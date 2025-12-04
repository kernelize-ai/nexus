#ifndef RT_TT_BUFFER_H
#define RT_TT_BUFFER_H

#include "tenstorrent.h"

#include <rt_buffer.h>

class TTBuffer : public nxs::rt::Buffer {
  typedef std::shared_ptr<ttmd::MeshBuffer> Buffer_sp;
  TTDevice device;
  Buffer_sp buffer;
  nxs_uint address;
  bool buffer_loaded;

 public:
  TTBuffer(TTDevice dev = TTDevice(), size_t size = 0,
           void *data_ptr = nullptr, nxs_uint settings = 0);

  ~TTBuffer() = default;

  nxs_uint *getAddress() { return &address; }

  Buffer_sp makeDeviceBuffer();

  nxs_status copyToHost(void *host_buf);

};

#endif  // RT_TT_BUFFER_H