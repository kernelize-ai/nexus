// ========================================
// File: matmul_kernels.hip
// ========================================
#include <hip/hip_runtime.h>

extern "C" {

// Matrix multiplication kernel
__global__ void add_vectors(float* A, float* B, float* C) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    C[idx] = A[idx] + B[idx];
}


// Matrix multiplication kernel
__global__ void matmul_kernel(float* A, float* B, float* C, int M, int N, int K) {
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (row < M && col < N) {
        float sum = 0.0f;
        for (int k = 0; k < K; k++) {
            sum += A[row * K + k] * B[k * N + col];
        }
        C[row * N + col] = sum;
    }
}

// Optimized matrix multiplication kernel using shared memory
__global__ void matmul_shared_kernel(float* A, float* B, float* C, int M, int N, int K) {
    const int TILE_SIZE = 16;
    __shared__ float As[TILE_SIZE][TILE_SIZE];
    __shared__ float Bs[TILE_SIZE][TILE_SIZE];
    
    int bx = blockIdx.x, by = blockIdx.y;
    int tx = threadIdx.x, ty = threadIdx.y;
    int row = by * TILE_SIZE + ty;
    int col = bx * TILE_SIZE + tx;
    
    float sum = 0.0f;
    
    for (int tile = 0; tile < (K + TILE_SIZE - 1) / TILE_SIZE; tile++) {
        // Load tiles into shared memory
        if (row < M && tile * TILE_SIZE + tx < K) {
            As[ty][tx] = A[row * K + tile * TILE_SIZE + tx];
        } else {
            As[ty][tx] = 0.0f;
        }
        
        if (col < N && tile * TILE_SIZE + ty < K) {
            Bs[ty][tx] = B[(tile * TILE_SIZE + ty) * N + col];
        } else {
            Bs[ty][tx] = 0.0f;
        }
        
        __syncthreads();
        
        // Compute partial sum
        for (int k = 0; k < TILE_SIZE; k++) {
            sum += As[ty][k] * Bs[k][tx];
        }
        
        __syncthreads();
    }
    
    if (row < M && col < N) {
        C[row * N + col] = sum;
    }
}

// Block-wise matrix multiplication with larger tiles
__global__ void matmul_block_kernel(float* A, float* B, float* C, int M, int N, int K) {
    const int TILE_SIZE = 32;
    __shared__ float As[TILE_SIZE][TILE_SIZE];
    __shared__ float Bs[TILE_SIZE][TILE_SIZE];
    
    int bx = blockIdx.x, by = blockIdx.y;
    int tx = threadIdx.x, ty = threadIdx.y;
    int row = by * TILE_SIZE + ty;
    int col = bx * TILE_SIZE + tx;
    
    float sum = 0.0f;
    
    for (int tile = 0; tile < (K + TILE_SIZE - 1) / TILE_SIZE; tile++) {
        // Load tiles into shared memory with bounds checking
        if (row < M && tile * TILE_SIZE + tx < K) {
            As[ty][tx] = A[row * K + tile * TILE_SIZE + tx];
        } else {
            As[ty][tx] = 0.0f;
        }
        
        if (col < N && tile * TILE_SIZE + ty < K) {
            Bs[ty][tx] = B[(tile * TILE_SIZE + ty) * N + col];
        } else {
            Bs[ty][tx] = 0.0f;
        }
        
        __syncthreads();
        
        // Compute partial sum with unrolling
        #pragma unroll
        for (int k = 0; k < TILE_SIZE; k++) {
            sum += As[ty][k] * Bs[k][tx];
        }
        
        __syncthreads();
    }
    
    if (row < M && col < N) {
        C[row * N + col] = sum;
    }
}

} // extern "C"

