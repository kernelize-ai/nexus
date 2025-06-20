#ifndef RUNTIME_KERNEL_H
#define RUNTIME_KERNEL_H

#include <string>
#include <vector>

#include <runtime_buffer.h>

class RuntimeKernel {

public:

    RuntimeKernel() = default;
    RuntimeKernel(const std::string &name, const std::string &sourceCode)
        : name(name), sourceCode(sourceCode) {}

    ~RuntimeKernel() = default;

    void setArgument(const RuntimeBuffer &arg, nxs_int bufferIndex) {
        std::cout << bufferIndex << std::endl;
        if (bufferIndex == arguments.size())
            arguments.push_back(arg);
        else
            arguments[bufferIndex] = arg;
    }

    std::string name; // The name of the kernel
    std::string sourceCode; // The source code of the kernel

    CUmodule module;
    CUfunction kernel;

    std::vector<RuntimeBuffer> arguments; // Arguments for the kernel
};

typedef std::vector<RuntimeKernel> Kernels;

#endif // RUNTIME_KERNEL_H