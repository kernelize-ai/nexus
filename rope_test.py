#!/usr/bin/env python3
"""
Nexus implementation of rotary embedding function, equivalent to vLLM's rotary_embedding.
"""

import numpy as np
import sys
import subprocess

# Install torch if needed
try:
    import torch
except ImportError:
    subprocess.check_call([sys.executable, "-m", "pip", "install", "torch"])
    import torch

import nexus
from typing import Optional, Union

def rotary_embedding(
    positions: Union[np.ndarray, torch.Tensor],  # [batch_size, seq_len] or [num_tokens]
    query: Union[np.ndarray, torch.Tensor],      # [batch_size, seq_len, num_heads * head_size] or
                           # [num_tokens, num_heads * head_size] or
                           # [batch_size, seq_len, num_heads, head_size] or
                           # [num_tokens, num_heads, head_size]
    key: Optional[Union[np.ndarray, torch.Tensor]] = None,  # Same shape options as query, or None
    head_size: int = 64,
    cos_sin_cache: Optional[Union[np.ndarray, torch.Tensor]] = None,  # [max_position, rot_dim]
    is_neox: bool = False,
    kernel_file: str = "build.local/cuda_kernels/pos_encoding_kernels.ptx",  # Path to your PTX kernel file
    kernel_name: str = "_ZN4vllm23rotary_embedding_kernelIfLb0EEEvPKlPT_S4_PKS3_illliii",
    runtime_name: str = "cuda"  # Your runtime name
) -> tuple[Union[np.ndarray, torch.Tensor], Optional[Union[np.ndarray, torch.Tensor]]]:
    """
    Apply rotary position embedding to query and key tensors using Nexus runtime.
    
    Args:
        positions: Position indices for each token
        query: Query tensor to apply rotary embedding to
        key: Optional key tensor to apply rotary embedding to
        head_size: Size of each attention head
        cos_sin_cache: Precomputed cos/sin cache, will be generated if None
        is_neox: Whether to use NeoX-style rotary embedding
        kernel_file: Path to CUDA kernel file
        kernel_name: Name of the kernel function
        runtime_name: Name of the runtime to use
        
    Returns:
        Tuple of (rotated_query, rotated_key)
    """
    
    # Convert inputs to torch tensors if they're numpy arrays
    if isinstance(positions, np.ndarray):
        positions = torch.from_numpy(positions.astype(np.int64))
    if isinstance(query, np.ndarray):
        query = torch.from_numpy(query.astype(np.float32))
    if key is not None and isinstance(key, np.ndarray):
        key = torch.from_numpy(key.astype(np.float32))
    
    # Ensure correct dtypes
    positions = positions.to(torch.int64)
    query = query.to(torch.float32)
    if key is not None:
        key = key.to(torch.float32)
    
    positions_ndim = positions.dim()
    num_tokens = positions.numel()
    
    # Validate position dimensions
    if positions_ndim not in [1, 2]:
        raise ValueError("positions must have shape [num_tokens] or [batch_size, seq_len]")
    
    # Validate tensor shapes match positions
    if positions_ndim == 1:
        # For 1D positions, first dim of query/key should match positions OR be num_tokens
        expected_tokens = positions.shape[0]
        if query.dim() >= 2 and query.shape[0] * query.shape[1] == expected_tokens:
            # Handle case: positions=[seq_len], query=[batch_size, seq_len, ...]
            pass  # This is valid
        elif query.shape[0] != expected_tokens:
            raise ValueError(f"query first dimension ({query.shape[0]}) must match positions length ({expected_tokens}) or total tokens")
        
        if key is not None:
            if key.dim() >= 2 and key.shape[0] * key.shape[1] == expected_tokens:
                pass  # This is valid
            elif key.shape[0] != expected_tokens:
                raise ValueError(f"key first dimension ({key.shape[0]}) must match positions length ({expected_tokens}) or total tokens")
    elif positions_ndim == 2:
        if (query.shape[0] != positions.shape[0] or 
            query.shape[1] != positions.shape[1]):
            raise ValueError("query and positions must have the same batch_size and seq_len")
        if (key is not None and 
            (key.shape[0] != positions.shape[0] or key.shape[1] != positions.shape[1])):
            raise ValueError("key and positions must have the same batch_size and seq_len")
    
    # Calculate tensor properties
    query_hidden_size = query.numel() // num_tokens
    key_hidden_size = key.numel() // num_tokens if key is not None else 0
    
    if query_hidden_size % head_size != 0:
        raise ValueError(f"query_hidden_size ({query_hidden_size}) must be divisible by head_size ({head_size})")
    if key is not None and key_hidden_size % head_size != 0:
        raise ValueError(f"key_hidden_size ({key_hidden_size}) must be divisible by head_size ({head_size})")
    
    num_heads = query_hidden_size // head_size
    num_kv_heads = key_hidden_size // head_size if key is not None else num_heads
    
    if num_heads % num_kv_heads != 0:
        raise ValueError(f"num_heads ({num_heads}) must be divisible by num_kv_heads ({num_kv_heads})")
    
    # Generate cos_sin_cache if not provided
    if cos_sin_cache is None:
        max_position = int(positions.max()) + 1
        rot_dim = head_size  # Assuming rot_dim equals head_size
        cos_sin_cache_np = create_cos_sin_cache(max_position, rot_dim)
        cos_sin_cache = torch.from_numpy(cos_sin_cache_np)
    else:
        if isinstance(cos_sin_cache, np.ndarray):
            cos_sin_cache = torch.from_numpy(cos_sin_cache.astype(np.float32))
        cos_sin_cache = cos_sin_cache.to(torch.float32)
        rot_dim = cos_sin_cache.shape[1]
    
    # Calculate strides exactly like vLLM
    seq_dim_idx = positions_ndim - 1
    query_stride = query.stride()[seq_dim_idx]  # This is the key fix!
    key_stride = key.stride()[seq_dim_idx] if key is not None else 0
    
    # Determine head stride: for [*, heads, head_size] use stride of last dim;
    # for flat [*, heads*head_size], heads blocks are contiguous of size head_size
    query_ndim = query.dim()
    if query_ndim == positions_ndim + 2:
        head_stride = query.stride()[-2]  # stride(-2) in PyTorch
    else:
        head_stride = head_size
    
    # Ensure tensors are contiguous and on CPU before creating buffers
    positions = positions.contiguous()
    query = query.contiguous() 
    if key is not None:
        key = key.contiguous()
    cos_sin_cache = cos_sin_cache.contiguous()
    
    # Get Nexus runtime and device (following the improved pattern)
    rt = None
    dev = None
    
    rt = nexus.get_runtime(runtime_name)
    dev = rt.get_devices()[0] if rt and rt.get_devices().size() > 0 else None
    
    if not rt or not dev:
        raise RuntimeError(f"No GPU runtime/device found")
    
    # Create buffers
    nb_positions = dev.create_buffer(positions)
    nb_query = dev.create_buffer(query)
    nb_cos_sin_cache = dev.create_buffer(cos_sin_cache)
    
    # Handle optional key buffer - Nexus might not handle None/nullptr well
    if key is not None:
        nb_key = dev.create_buffer(key)
    else:
        # Create a minimal dummy buffer since Nexus may not handle None gracefully
        # The kernel should ignore this when key_stride is 0
        dummy_key = torch.zeros(1, dtype=torch.float32)
        nb_key = dev.create_buffer(dummy_key)
    
    # Load library and get kernel
    lib = dev.load_library_file(kernel_file)
    if not lib:
        raise RuntimeError(f"Failed to load library: {kernel_file}")
    
    kern = lib.get_kernel(kernel_name)
    if not kern:
        raise RuntimeError(f"Failed to get kernel: {kernel_name}")
    
    # Create schedule and command
    sched = dev.create_schedule()
    cmd = sched.create_command(kern)
    
    # Set kernel arguments exactly matching vLLM kernel signature
    cmd.set_arg(0, nb_positions)     # positions.data_ptr<int64_t>()
    cmd.set_arg(1, nb_query)         # query.data_ptr<scalar_t>()
    cmd.set_arg(2, nb_key)           # key buffer (dummy if None)
    cmd.set_arg(3, nb_cos_sin_cache) # cos_sin_cache.data_ptr<scalar_t>()
    cmd.set_arg(4, rot_dim)          # rot_dim (int)
    cmd.set_arg(5, query_stride)     # query_stride (int64_t)
    cmd.set_arg(6, key_stride)       # key_stride (int64_t) - will be 0 if no key
    cmd.set_arg(7, head_stride)      # head_stride (int64_t)
    cmd.set_arg(8, num_heads)        # num_heads (int)
    cmd.set_arg(9, num_kv_heads)     # num_kv_heads (int)
    cmd.set_arg(10, head_size)       # head_size (int)
    
    # Configure grid and block dimensions 
    # Based on vLLM: grid = num_tokens, block = min(num_heads * rot_dim / 2, 512)
    # But your C++ test uses smaller values that work: grid=32, block=32
    grid_size = num_tokens
    
    # Try the vLLM calculation first, but with a more conservative approach
    vllm_block_size = min(num_heads * rot_dim // 2, 512)
    
    # If the vLLM block size is too large, fall back to a smaller size
    # The C++ test uses 32, so let's try that as a fallback
    if vllm_block_size > 64:  # Conservative threshold
        block_size = 32  # Match your working C++ test
    else:
        block_size = max(vllm_block_size, 32)  # Ensure minimum of 32
    
    # Ensure block_size is a valid CUDA block size (power of 2, <= 1024)
    if block_size <= 0:
        block_size = 32
        
    print(f"Debug: grid_size={grid_size}, block_size={block_size} (vllm_would_be={vllm_block_size})")
    print(f"Debug: num_tokens={num_tokens}, num_heads={num_heads}, rot_dim={rot_dim}")
    print(f"Debug: query_stride={query_stride}, key_stride={key_stride}, head_stride={head_stride}")
    
    cmd.finalize(grid_size, block_size)
    
    # Execute kernel
    sched.run()
    
    # Create result tensors and copy back
    result_query = torch.zeros_like(query)
    nb_query.copy(result_query)
    
    result_key = None
    if key is not None:
        result_key = torch.zeros_like(key)
        nb_key.copy(result_key)
    
    return result_query, result_key


def create_cos_sin_cache(max_position: int, rot_dim: int) -> np.ndarray:
    """
    Create cosine/sine cache for rotary embeddings.
    
    Args:
        max_position: Maximum position to cache
        rot_dim: Rotation dimension (typically head_size)
        
    Returns:
        Cache array of shape [max_position, rot_dim]
    """
    cache = np.zeros((max_position, rot_dim), dtype=np.float32)
    
    for pos in range(max_position):
        for dim in range(rot_dim // 2):
            angle = pos / (10000.0 ** (2.0 * dim / rot_dim))
            cache[pos, dim] = np.cos(angle)                    # cos part
            cache[pos, rot_dim // 2 + dim] = np.sin(angle)     # sin part
    
    return cache


# Example usage and test function
def test_rotary_embedding():
    """Test the rotary embedding implementation."""
    # Test parameters
    batch_size = 2
    seq_len = 4
    num_heads = 8
    head_size = 64
    num_tokens = batch_size * seq_len
    
    # Create test data using torch tensors
    positions = torch.arange(num_tokens, dtype=torch.int64)  # [0, 1, 2, 3, 4, 5, 6, 7]
    
    # Flatten the query/key to match [num_tokens, num_heads * head_size]
    query = torch.randn(num_tokens, num_heads * head_size, dtype=torch.float32)
    key = torch.randn(num_tokens, num_heads * head_size, dtype=torch.float32)
    
    # Apply rotary embedding
    try:
        rotated_query, rotated_key = rotary_embedding(
            positions=positions,
            query=query,
            key=key,
            head_size=head_size,
            kernel_file="build.local/cuda_kernels/pos_encoding_kernels.ptx",
            kernel_name="_ZN4vllm23rotary_embedding_kernelIfLb0EEEvPKlPT_S4_PKS3_illliii"
        )
        
        print("Rotary embedding test passed!")
        print(f"Original query shape: {query.shape}")
        print(f"Rotated query shape: {rotated_query.shape}")
        print(f"Query changed: {not torch.allclose(query, rotated_query)}")
        
        return True
        
    except Exception as e:
        print(f"Test failed: {e}")
        return False


if __name__ == "__main__":
    test_rotary_embedding()