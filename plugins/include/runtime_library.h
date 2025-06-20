#ifndef RUNTIME_LIBRARY_H
#define RUNTIME_LIBRARY_H

#include <string>
#include <vector>

#include <nexus-api.h>

#include <runtime_kernel.h>

class RuntimeLibrary {

public:

    RuntimeLibrary() = default;
    RuntimeLibrary(void *library_data, nxs_uint data_size) {
        kernel = RuntimeKernel("add_vectors", std::string(static_cast<const char*>(library_data), data_size));
        kernelSize = data_size;
    }
    ~RuntimeLibrary() = default;

    CUmodule module;
    RuntimeKernel kernel;
    size_t kernelSize;

};

typedef std::vector<RuntimeLibrary> Libraries;

#endif // RUNTIME_LIBRARY_H