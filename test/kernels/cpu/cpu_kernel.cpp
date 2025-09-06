//#include <stdio.h>

typedef long long int64;

template <typename T>
void add_vectors(T *a, T *b, T *out, int64 global_size[],
                 int64 local_size[], int64 dims[]) {
  //printf("DIMS: %lld\n", dims[0]);
  a += dims[0];
  b += dims[0];
  out += dims[0];
  for (int i = 0; i < local_size[0]; ++i) {
    out[i] = a[i] + b[i];
  }
}

int main() {
  add_vectors<int>(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
  return 0;
}
