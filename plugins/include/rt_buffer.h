#ifndef RT_BUFFER_H
#define RT_BUFFER_H

#include <nexus-api.h>

#include <cstring>

namespace nxs {
namespace rt {

class Buffer {
  char *buf;
  nxs_buffer_layout layout;
  size_t size_bytes;
  nxs_uint settings;

 public:
  Buffer(const nxs_buffer_layout &layout, void *data_ptr = nullptr,
         nxs_uint settings = 0)
      : buf((char *)data_ptr), layout(layout), size_bytes(0), settings(settings) {
    setSizeBytes();
    if (settings & NXS_BufferSettings_Maintain) {
      buf = (char *)malloc(getSizeBytes());
      if (data_ptr) std::memcpy((void *)buf, data_ptr, getSizeBytes());
    }
  }
  Buffer(size_t size=0, void *data_ptr = nullptr, nxs_uint settings = 0)
      : Buffer(nxs_buffer_layout{
                   (nxs_uint)NXS_DataType_Undefined,
                   (nxs_uint)(size == 0 ? 0 : 1),
                   {size},
                   {0}},
               data_ptr, settings) {}
  ~Buffer() { release(); }
  void release() {
    if (buf && settings & NXS_BufferSettings_Maintain)
      free(buf);
    buf = nullptr;
    layout = nxs_buffer_layout{(nxs_uint)NXS_DataType_Undefined, 0, {0}, {0}};
    size_bytes = 0;
  }
  char *data() const { return buf; }
  char *getData() const { return buf; }
  size_t getSizeBytes() const { return size_bytes; }
  void setSizeBytes() {
    size_bytes = getNumElements();
    if (auto element_size_bits = getElementSizeBits()) {
      size_bytes *= element_size_bits;
      size_bytes /= 8;
    }
  }
  void setSizeBytes(size_t new_size_bytes) {
    auto *buf_c = buf;
    if (settings & NXS_BufferSettings_Maintain) {
      buf = (char *)realloc(buf, new_size_bytes);
    } else {
      settings |= NXS_BufferSettings_Maintain;
      buf = (char *)malloc(new_size_bytes);
      if (buf_c) std::memcpy((void *)buf, buf_c, std::min(size_bytes, new_size_bytes));
    }
    layout = nxs_buffer_layout{(nxs_uint)NXS_DataType_Undefined, 1,
                               {new_size_bytes}, {0}};
    size_bytes = new_size_bytes;
  }
  const nxs_buffer_layout& getShape() const { return layout; }
  nxs_ulong getNumElements() const { return nxsGetNumElements(layout); }
  nxs_uint getElementSizeBits() const {
    return nxsGetDataTypeSizeBits(layout.data_type);
  }
  nxs_data_type getDataType() const {
    return (nxs_data_type)layout.data_type;
  }
  nxs_uint getSettings() const { return settings; }
  void setSettings(nxs_uint new_settings) {
    settings = new_settings;
  }
  template <typename T = void>
  T *get() {
    return reinterpret_cast<T *>(buf);
  }
};

}  // namespace rt
}  // namespace nxs

#endif  // RT_BUFFER_H
