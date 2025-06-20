#ifndef RUNTIME_COMMAND_H
#define RUNTIME_COMMAND_H

#include <runtime_library.h>
#include <runtime_buffer.h>

class RuntimeCommand {

public:

    RuntimeCommand() = delete;
    RuntimeCommand(RuntimeLibrary &library) : library(library) {}
    ~RuntimeCommand() = default;

    nxs_int setArgument(nxs_int argument_index, RuntimeBuffer buffer) {
      if (argument_index >= buffers.size())
        buffers.push_back(buffer);
      else
        buffers[argument_index] = buffer;

      return 0;
    }

    nxs_int kernel;
    RuntimeLibrary library;
    Buffers buffers;

    int gridSize = -1;
    int blockSize = -1;

};

typedef std::vector<RuntimeCommand> Commands;

#endif // RUNTIME_COMMAND_H