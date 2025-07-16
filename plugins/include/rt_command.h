#ifndef RT_COMMAND_H
#define RT_COMMAND_H

#include <rt_library.h>
#include <rt_buffer.h>

namespace nxs {
namespace rt {

class Command : public rt::Object {

public:

    std::vector<Buffer> buffers;

    int gridSize = -1;
    int blockSize = -1;

    Command() = default;
    virtual ~Command() = default;

    nxs_status setArgument(nxs_int argument_index, Buffer buffer) {
      if (argument_index >= buffers.size())
        buffers.push_back(buffer);
      else
        buffers[argument_index] = buffer;

      return NXS_Success;
    }

    nxs_status finalize(nxs_int group_size, nxs_int grid_size) {
      gridSize = grid_size;
      blockSize = group_size;
      return NXS_Success;
    }

};

typedef std::vector<rt::Command *> Commands;

}  // namespace rt
}  // namespace nxs

#endif // RT_COMMAND_H