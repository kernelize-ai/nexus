#!/usr/bin/env python3
import math, numpy as np, torch, nexus

# ===== Hardcoded config =====
runtime_name = "cuda"
ptx_path     = "build.local/cuda_kernels/pos_encoding_kernels.ptx"
kernel_name  = "_ZN4vllm23rotary_embedding_kernelIN3c108BFloat16ELb1EEEvPKlPT_S6_PKS5_illliii"

num_tokens   = 4
num_heads    = 8
num_kv_heads = 8
head_size    = 64
rot_dim      = 64
max_position = 100

assert rot_dim % 2 == 0 and rot_dim <= head_size
embed = rot_dim // 2

# ===== Helpers =====
def to_bf16_like_np(arr: np.ndarray) -> np.ndarray:
    return torch.from_numpy(arr).to(torch.bfloat16).to(torch.float32).cpu().numpy()

def cpu_neox_split_rotate_ptx_exact(x_head_f32: np.ndarray, pos: int, cache_bf16_flat: torch.Tensor) -> np.ndarray:
    """NeoX pairing, SPLIT cache, BF16 per-product rounding to mirror kernel."""
    x = x_head_f32.copy()
    base = pos * rot_dim
    cos = cache_bf16_flat[base : base+embed].to(torch.float32).cpu().numpy()
    sin = cache_bf16_flat[base+embed : base+rot_dim].to(torch.float32).cpu().numpy()
    a = x[:embed].copy()
    b = x[embed:2*embed].copy()
    # PTX-like rounding at each step
    xo = to_bf16_like_np(to_bf16_like_np(a * cos) - to_bf16_like_np(b * sin))
    yo = to_bf16_like_np(to_bf16_like_np(b * cos) + to_bf16_like_np(a * sin))
    x[:embed]        = xo
    x[embed:2*embed] = yo
    return x

# ===== Inputs (BF16) =====
torch.manual_seed(17)
positions_t = torch.arange(num_tokens, dtype=torch.long)  # [0,1,2,3]
q_in_t  = torch.empty(num_tokens * num_heads    * head_size, dtype=torch.bfloat16)
k_in_t  = torch.empty(num_tokens * num_kv_heads * head_size, dtype=torch.bfloat16)
torch.nn.init.uniform_(q_in_t, a=-1.0, b=1.0)
torch.nn.init.uniform_(k_in_t, a=-1.0, b=1.0)

# ===== Build BF16 SPLIT cache (float32 math, then cast to BF16) =====
# angle(pos, d) = pos * 10000^(-2*d/rot_dim)
inv_freq = torch.pow(torch.tensor(10000.0, dtype=torch.float32),
                     -2.0 * torch.arange(embed, dtype=torch.float32) / float(rot_dim))
cos_sin = torch.empty(max_position, rot_dim, dtype=torch.float32)
for pos in range(max_position):
    angle = pos * inv_freq  # [embed], float32
    cos_sin[pos, 0:embed]       = torch.cos(angle)
    cos_sin[pos, embed:rot_dim] = torch.sin(angle)
cache_t = cos_sin.reshape(-1).to(torch.bfloat16).contiguous()  # [max_position*rot_dim], BF16

# ===== Element strides (matches .cu) =====
# query/key pointers are scalar_t*; the kernel adds element offsets and
# multiplies by element size internally when doing ld/st.
q_stride_elems = (num_heads    * head_size)  # elements per token
k_stride_elems = (num_kv_heads * head_size)  # elements per token
h_stride_elems = head_size                   # elements per head

# ===== Nexus setup =====
rt  = nexus.get_runtime(runtime_name)
dev = next(d for d in rt.get_devices() if d.get_property_str(nexus.property.Type) == "gpu")

b_pos   = dev.create_buffer(positions_t)
b_q     = dev.create_buffer(q_in_t)
b_k     = dev.create_buffer(k_in_t)
b_cache = dev.create_buffer(cache_t)

lib  = dev.load_library_file(ptx_path)
kern = lib.get_kernel(kernel_name)
sched = dev.create_schedule()
cmd   = sched.create_command(kern)

# Arg order per PTX/.cu:
# 0: positions (u64), 1: query (u64), 2: key (u64), 3: cache (u64)
# 4: rot_dim (u32)
# 5: query_stride (u64 ELEMENTS), 6: key_stride (u64 ELEMENTS), 7: head_stride (u64 ELEMENTS)
# 8: num_heads (u32), 9: num_kv_heads (u32), 10: head_size (u32)
cmd.set_arg(0,  b_pos)
cmd.set_arg(1,  b_q)
cmd.set_arg(2,  b_k)
cmd.set_arg(3,  b_cache)
cmd.set_arg(4,  rot_dim)                 # u32
cmd.set_arg(5,  int(q_stride_elems), True)  # u64 (ELEMENTS)
cmd.set_arg(6,  int(k_stride_elems), True)  # u64 (ELEMENTS)
cmd.set_arg(7,  int(h_stride_elems), True)  # u64 (ELEMENTS)
cmd.set_arg(8,  num_heads)              # u32
cmd.set_arg(9,  num_kv_heads)           # u32
cmd.set_arg(10, head_size)              # u32

# ===== Launch: 1 token per block; threads = num_heads * (rot_dim/2) =====
threads_x = min(num_heads * (rot_dim // 2), 512)
cmd.finalize(int(num_tokens), int(threads_x))

stream = dev.create_stream()
sched.run(stream, True)  # block until finished

# ===== Copy back =====
q_out_t = torch.empty_like(q_in_t)
b_q.copy(q_out_t)
q_in  = q_in_t.to(torch.float32).cpu().numpy()
q_out = q_out_t.to(torch.float32).cpu().numpy()

print("NeoX BF16 kernel run complete.")
print("First 8 in :", q_in[:8])
print("First 8 out:", q_out[:8])

# Quick sanity: ensure something changed
changed = float(np.max(np.abs(q_out - q_in)))
print(f"max|q_out - q_in| over all elems: {changed:.6f}")

# ===== Compare token=1, head=0 (first rot_dim only) =====
stride_e = num_heads * head_size
t, h = min(1, num_tokens-1), 0
base = t * stride_e + h * head_size

before = q_in [base:base+head_size]
after  = q_out[base:base+head_size]
pred   = cpu_neox_split_rotate_ptx_exact(before.copy(), pos=t, cache_bf16_flat=cache_t)

a = after[:rot_dim]
p = pred [:rot_dim]
diff = np.abs(a - p)

mae = float(diff.mean())
mxe = float(diff.max())
nb  = float(np.linalg.norm(before[:rot_dim]))
na  = float(np.linalg.norm(a))
npb = float(np.linalg.norm(p))

print(f"\nToken={t}, Head={h} (NeoX + SPLIT cache, PTX-accurate BF16 math):")
print("GPU out:", a)
print("CPU ref:", p)
print(f"MAE={mae:.6f}  max|e|={mxe:.6f}")
print(f"norm in={nb:.6f}  norm GPU={na:.6f}  norm CPUref={npb:.6f}")

# Robust gates (BF16-friendly)
strict_thr  = 0.30
relaxed_thr = 0.65
bad_strict  = int((diff > strict_thr).sum())
bad_relaxed = int((diff > relaxed_thr).sum())

print(f"\nError stats over first rot_dim: >{strict_thr:.2f}: {bad_strict}, >{relaxed_thr:.2f}: {bad_relaxed}")
ok_mae   = mae <= 0.06
ok_norm  = abs(na - npb) / max(npb,1e-9) <= 0.02
ok_shape = bad_relaxed <= 1 and bad_strict <= 3
print(f"PASS CHECKS → MAE<=0.06: {ok_mae}, norms within 2%: {ok_norm}, outliers ok: {ok_shape}")

if not (ok_mae and ok_norm and ok_shape):
    import sys
    sys.exit(1)
