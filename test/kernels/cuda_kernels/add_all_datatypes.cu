extern "C" __global__ void add_all_datatypes(
    float val_float,
    double val_double,
    int val_int,
    short val_short,
    char val_char,
    unsigned int val_uint,
    long long val_longlong,
    double* result
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (idx == 0) {  // Only thread 0 writes the result
        *result = (double)val_float + 
                  val_double + 
                  (double)val_int + 
                  (double)val_short + 
                  (double)val_char + 
                  (double)val_uint + 
                  (double)val_longlong;
    }
}