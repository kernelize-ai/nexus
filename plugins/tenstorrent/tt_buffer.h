#ifndef RT_TT_BUFFER_H
#define RT_TT_BUFFER_H

#include "tenstorrent.h"

#include <rt_buffer.h>

class TTDevice;

class TTBuffer : public nxs::rt::Buffer {
  typedef std::shared_ptr<ttmd::MeshBuffer> Buffer_sp;
  TTDevice *device;
  nxs_uint address;
  Buffer_sp buffer;

  nxs_buffer_layout tilizedShape;
  nxs_ulong rowCount;
  nxs_ulong paddedSize;
  nxs_uint elementSize;

 public:
  TTBuffer(TTDevice *dev = nullptr,
           nxs_buffer_layout shape = nxs_buffer_layout{(nxs_uint)NXS_DataType_Undefined, 0, {0}, {0}},
           void *data_ptr = nullptr, nxs_uint settings = 0);

  ~TTBuffer() = default;

  nxs_ulong size() const { return getSizeBytes(); }
  nxs_uint *getAddress() { return &address; }

  template <typename T>
  nxs_status tilizeAndCopyToDevice(T *data_ptr, bool blocking);

  nxs_status copyToDevice(void *host_buf, bool blocking);

  template <typename T>
  nxs_status copyToHostUntilize(T *host_buf);

  nxs_status copyToHost(void *host_buf);

  constexpr static nxs_ulong tileWidth = 32;

};

#endif  // RT_TT_BUFFER_H