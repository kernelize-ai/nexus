#!/usr/bin/env python3
import argparse
import math
import sys
import numpy as np
import torch

import nexus

# -----------------------------
# CPU reference implementation
# -----------------------------
def rope_cpu_ref(query_in, positions, rot_dim, num_heads, head_size,
                 cache, is_neox=False):
    """
    Recompute rotary embeddings (float32) on CPU.

    query_in:  (N*H*S,) float32
    positions: (N,) int64
    cache:     (max_pos*rot_dim,) float32 laid out per pos: [cos[0:rot_dim/2], sin[0:rot_dim/2]]
    """
    assert rot_dim % 2 == 0
    num_tokens = positions.shape[0]
    embed_dim = rot_dim // 2

    q = query_in.reshape(num_tokens, num_heads, head_size).copy()
    for t in range(num_tokens):
        pos = int(positions[t])
        base = pos * rot_dim
        cos = cache[base:base+embed_dim]
        sin = cache[base+embed_dim:base+2*embed_dim]

        for h in range(num_heads):
            arr = q[t, h]  # shape [head_size]
            if is_neox:
                # NeoX pairs: (i, i+embed_dim)
                x = arr[0:embed_dim]
                y = arr[embed_dim:2*embed_dim]
                xo = x * cos - y * sin
                yo = y * cos + x * sin
                arr[0:embed_dim] = xo
                arr[embed_dim:2*embed_dim] = yo
            else:
                # GPT-J pairs: (2*i, 2*i+1)
                x = arr[0:2*embed_dim:2]
                y = arr[1:2*embed_dim:2]
                xo = x * cos - y * sin
                yo = y * cos + x * sin
                arr[0:2*embed_dim:2] = xo
                arr[1:2*embed_dim:2] = yo

    return q.reshape(-1)


def validate_rope(query_orig, query_gpu, positions, rot_dim, num_heads, head_size,
                  cache, is_neox=False, atol=5e-6, rtol=5e-6):
    cpu = rope_cpu_ref(query_orig, positions, rot_dim, num_heads, head_size, cache, is_neox)
    diff = np.abs(cpu - query_gpu)
    max_err  = float(diff.max())
    mean_err = float(diff.mean())
    p99      = float(np.percentile(diff, 99))
    ok = np.allclose(cpu, query_gpu, atol=atol, rtol=rtol)

    # Norm preservation check on token 1 pairs (like your C++ prints)
    max_len_err = 0.0
    if positions.shape[0] >= 2:
        base = num_heads * head_size  # token index 1 offset
        if not is_neox:
            for i in range(0, rot_dim, 2):
                xa, ya = query_orig[base+i],   query_orig[base+i+1]
                xb, yb = query_gpu [base+i],   query_gpu [base+i+1]
                max_len_err = max(max_len_err, abs((xa*xa+ya*ya) - (xb*xb+yb*yb)))
        else:
            half = rot_dim // 2
            for i in range(half):
                xa, ya = query_orig[base+i],   query_orig[base+half+i]
                xb, yb = query_gpu [base+i],   query_gpu [base+half+i]
                max_len_err = max(max_len_err, abs((xa*xa+ya*ya) - (xb*xb+yb*yb)))

    return ok, max_err, max_len_err, cpu

# -----------------------------
# Main
# -----------------------------
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("runtime", help="runtime name (e.g., cuda)")
    ap.add_argument("ptx_path", help="path to PTX (e.g., build.local/cuda_kernels/pos_encoding_kernels.ptx)")
    ap.add_argument("kernel", help="kernel symbol (e.g., mangled rotary kernel)")
    ap.add_argument("--num_tokens", type=int, default=4)
    ap.add_argument("--num_heads",  type=int, default=8)
    ap.add_argument("--num_kv_heads", type=int, default=8)
    ap.add_argument("--head_size",  type=int, default=64)
    ap.add_argument("--max_position", type=int, default=100)
    ap.add_argument("--rot_dim", type=int, default=64, help="rotary dimension (pairs*2)")
    ap.add_argument("--neox", type=int, default=0, help="1 for NeoX pairing, 0 for GPT-J pairing")
    args = ap.parse_args()

    runtime_name   = args.runtime
    kernel_file    = args.ptx_path
    kernel_name    = args.kernel
    num_tokens     = args.num_tokens
    num_heads      = args.num_heads
    num_kv_heads   = args.num_kv_heads
    head_size      = args.head_size
    max_position   = args.max_position
    rot_dim        = args.rot_dim
    is_neox        = (args.neox != 0)

    # -----------------------------
    # Build inputs on CPU (torch) so nexus can memcpy to device
    # -----------------------------
    torch.manual_seed(17)
    positions_t = torch.arange(num_tokens, dtype=torch.long)  # int64 [0..num_tokens-1]

    q_sz = num_tokens * num_heads * head_size
    k_sz = num_tokens * num_kv_heads * head_size
    c_sz = max_position * rot_dim

    query_in_t = torch.empty(q_sz, dtype=torch.float32)
    key_in_t   = torch.empty(k_sz, dtype=torch.float32)
    torch.nn.init.uniform_(query_in_t, a=-1.0, b=1.0)
    torch.nn.init.uniform_(key_in_t,   a=-1.0, b=1.0)

    # cos/sin cache layout: per position: [cos[0:embed_dim], sin[0:embed_dim]]
    embed_dim = rot_dim // 2
    cos_sin_cache_t = torch.empty(c_sz, dtype=torch.float32)
    for pos in range(max_position):
        for dim in range(embed_dim):
            angle = pos / (10000.0 ** (2.0 * dim / rot_dim))
            cos_sin_cache_t[pos*rot_dim + dim]           = math.cos(angle)
            cos_sin_cache_t[pos*rot_dim + embed_dim+dim] = math.sin(angle)

    # Keep host copies for CPU reference
    positions = positions_t.numpy().astype(np.int64)
    query_in  = query_in_t.numpy().astype(np.float32).copy()
    key_in    = key_in_t.numpy().astype(np.float32).copy()
    cache     = cos_sin_cache_t.numpy().astype(np.float32)

    # -----------------------------
    # Nexus: pick GPU device, load kernel, set args
    # -----------------------------
    rt = nexus.get_runtime(runtime_name)
    if not rt:
        print("No runtimes found")
        return 1

    dev = None
    for d in rt.get_devices():
        if d.get_property_str(nexus.property.Type) == "gpu":
            dev = d
            break
    if dev is None:
        print("No GPU device found")
        return 1

    # Device buffers: (host tensors → device copies)
    b_pos   = dev.create_buffer(positions_t)
    b_query = dev.create_buffer(query_in_t)
    b_key   = dev.create_buffer(key_in_t)
    b_cache = dev.create_buffer(cos_sin_cache_t)

    lib  = dev.load_library_file(kernel_file)
    kern = lib.get_kernel(kernel_name)
    if not kern:
        print("Failed to get kernel:", kernel_name)
        return 1

    sched = dev.create_schedule()
    cmd   = sched.create_command(kern)

    # Strides for flat layout (like C++ test)
    query_stride = num_heads * head_size
    key_stride   = num_kv_heads * head_size
    head_stride  = head_size

    # Set kernel args (match widths!)
    cmd.set_arg(0, b_pos)                 # positions
    cmd.set_arg(1, b_query)               # query
    cmd.set_arg(2, b_key)                 # key
    cmd.set_arg(3, b_cache)               # cos_sin_cache

    cmd.set_arg(4,  rot_dim)             # rot_dim (int)
    cmd.set_arg(5,  query_stride, True)        # query_stride (int64_t)
    cmd.set_arg(6,  key_stride, True)          # key_stride (int64_t)
    cmd.set_arg(7,  head_stride, True)         # head_stride (int64_t)
    cmd.set_arg(8,  num_heads)           # num_heads (int)
    cmd.set_arg(9,  num_kv_heads)        # num_kv_heads (int)
    cmd.set_arg(10, head_size)           # head_size (int)

    grid  = int(num_tokens)
    block = int(min(num_heads * (rot_dim // 2), 512))
    cmd.finalize(grid, block)

    stream = dev.create_stream()
    sched.run(stream, False)

    # Copy result back
    query_out_t = torch.empty_like(query_in_t)
    key_out_t   = torch.empty_like(key_in_t)
    b_query.copy(query_out_t)
    b_key.copy(key_out_t)

    query_out = query_out_t.numpy().astype(np.float32)

    print("Rotary embedding kernel completed successfully!")
    print("First few query values after rotation:", *query_out[:5])
    if num_tokens >= 2:
        base = num_heads * head_size
        print("Position[1]:", positions[1])
        print(f"Original query[{base}]: {query_in[base]:.6f}")
        print(f"Result query[{base}]: {query_out[base]:.6f}")
    print(f"Cache[{rot_dim}]: {cache[rot_dim]:.6f}")  # pos=1, first cos


    ok, max_err, max_len_err, cpu_ref = validate_rope(
        query_in, query_out, positions, rot_dim,
        num_heads, head_size, cache, is_neox=is_neox
    )

    changed_idx = None
    for i in range(query_out.shape[0]):
        if abs(query_out[i] - query_in[i]) > 1e-6:
            changed_idx = i
            break
    if changed_idx is not None:
        print(f"Changed at index {changed_idx}: {query_in[changed_idx]} -> {query_out[changed_idx]}")

    print(f"pair_ok={int(ok)} max_len_err={max_len_err:.9g}")
    if not ok:
        print(f"FAIL: CPU/GPU mismatch, max|diff|={max_err:.9g}")
        return 1

    print("\nTest PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
