extern "C" __global__ void add_vectors_shared_memory(float* a, float* b, float* c) {
    extern __shared__ float sh_mem[];

    float* s_a = sh_mem;
    float* s_b = &s_a[blockDim.x];

    int tid = threadIdx.x;
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    s_a[tid] = a[idx];
    s_b[tid] = b[idx];

    __syncthreads();

    float sum = s_a[tid] + s_b[tid];

    c[idx] = sum;
}

