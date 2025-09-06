
template <typename T>
__global__ void add_vectors(T* a, T* b, T* c) {
  int idx = blockIdx.x * blockDim.x + threadIdx.x;
  c[idx] = a[idx] + b[idx];
}


void test() {
  add_vectors<long long><<<1,1>>>(nullptr, nullptr, nullptr);
}
