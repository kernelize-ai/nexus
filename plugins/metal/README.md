
## Example to generate kernel.so

Kernel source:
```
#include <metal_stdlib>
using namespace metal;

kernel void add_vectors(device const float* inA [[buffer(0)]],
                        device const float* inB [[buffer(1)]],
                        device float* result [[buffer(2)]],
                        uint id [[thread_position_in_grid]])
{
    result[id] = inA[id] + inB[id];
}
```

To generate LLVM IR:
```script
xcrun -sdk macosx metal -c kernel.metal -o kernel.ir
llvm-dis kernel.ir -o kernel.ll
```

To generate SO library binary:
```script
xcrun -sdk macosx metal -c kernel.metal -o kernel.ir
xcrun -sdk macosx metallib -o kernel.so kernel.ir
```
