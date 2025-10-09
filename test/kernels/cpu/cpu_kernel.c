#include <stdio.h>

typedef unsigned int uint32;

void add_vectors(float *a, float *b, float *out, int launch_size[],
                 int launch_id[], void *cpu_barrier) {
  const uint32 stride = 32;
  a += launch_id[0] * stride;
  b += launch_id[0] * stride;
  out += launch_id[0] * stride;
  out[launch_id[3]] = a[launch_id[3]] + b[launch_id[3]];
}
