#include <metal_stdlib>
using namespace metal;

kernel void add_vectors(device const float* inA [[buffer(0)]],
                        device const float* inB [[buffer(1)]],
                        device float* result [[buffer(2)]],
                        uint id [[thread_position_in_grid]])
{
    result[id] = inA[id] + inB[id];
}

