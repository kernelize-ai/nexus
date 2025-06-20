#ifndef RUNTIME_BUFFER_H
#define RUNTIME_BUFFER_H

#include <string>
#include <vector>

class RuntimeBuffer {

public:

    RuntimeBuffer(void *ptr = nullptr): ptr(ptr) {}

    ~RuntimeBuffer() = default;

    void *ptr; // Pointer to the buffer data
};

typedef std::vector<RuntimeBuffer> Buffers;

#endif // RUNTIME_BUFFER_H