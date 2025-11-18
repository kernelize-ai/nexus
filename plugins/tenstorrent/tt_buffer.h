#ifndef RT_TT_BUFFER_H
#define RT_TT_BUFFER_H

#include <rt_buffer.h>

#include "tenstorrent.h"

class TTBuffer : public nxs::rt::Buffer {
  typedef std::shared_ptr<ttmd::MeshBuffer> Buffer_sp;
  Buffer_sp buffer;
  nxs_uint address;
  bool buffer_loaded;

 public:
  TTBuffer(TTDevice device = TTDevice(), size_t size = 0,
           void *data_ptr = nullptr, nxs_uint settings = 0);

  ~TTBuffer() = default;

  nxs_uint *getAddress() { return &address; }

  Buffer_sp makeDeviceBuffer(TTDevice device);
};

#endif  // RT_TT_BUFFER_H