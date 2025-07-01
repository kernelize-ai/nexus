
typedef long long int64;

void add_vectors(float *a, float *b, float *out, int64 global_size[],
                 int64 local_size[], int64 dims[]) {
  a += dims[0];
  b += dims[0];
  out += dims[0];
  for (int i = 0; i < local_size[0]; ++i) {
    out[i] = a[i] + b[i];
  }
}
