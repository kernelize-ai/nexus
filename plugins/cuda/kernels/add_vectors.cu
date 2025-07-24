
extern "C" __global__ void add_vectors(float* a, float* b, float* c) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    c[idx] = a[idx] + b[idx];
}