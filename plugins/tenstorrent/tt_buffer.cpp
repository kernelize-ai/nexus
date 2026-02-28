#include <tt_buffer.h>
#include <tt_device.h>
#include <tt_runtime.h>

#include <tt-metalium/tilize_utils.hpp>

TTBuffer::TTBuffer(TTDevice *dev, nxs_shape shape,
                   void *data_ptr, nxs_uint settings)
  : Buffer(shape, data_ptr, settings), device(dev) {
    if (device) {
      switch (nxsGetDataType(settings)) {
        case NXS_DataType_F32:
          makeDeviceBuffer<float>();
          break;
        case NXS_DataType_BF16:
          makeDeviceBuffer<bfloat16>();
          break;
        case NXS_DataType_U32:
          makeDeviceBuffer<uint32_t>();
          break;
        case NXS_DataType_U16:
          makeDeviceBuffer<uint16_t>();
          break;
        default:
          NXSAPI_LOG(nexus::NXS_LOG_ERROR, "TTBuffer: Unsupported data type: ",
             nxsGetDataTypeName(nxsGetDataType(settings)));
          break;
      }
    }
}



template <typename T>
TTBuffer::Buffer_sp TTBuffer::makeDeviceBuffer() {
  if (!(getSettings() & NXS_BufferSettings_OnDevice)) {
    auto shape = getShape();
    if (shape.rank == 0) {
      return nullptr;
    }
    assert(shape.rank >= 2 && "Shape must be at least 2D");
    setSettings(getSettings() | NXS_BufferSettings_OnDevice);

    constexpr nxs_ulong tileWidth = 32;

    // Pad up to the nearest tile size
    nxs_shape tilizedShape = shape;
    nxs_ulong rowCount = 1;
    for (nxs_uint i = 0; i < tilizedShape.rank; i++) {
      tilizedShape.dims[i] = (tilizedShape.dims[i] + tileWidth - 1) / tileWidth;
      if (i != 0) rowCount *= tilizedShape.dims[i];
    }
    nxs_ulong tilizedStride = tilizedShape.dims[0];

    nxs_ulong rowStride = shape.dims[0] * sizeof(T);
    nxs_ulong paddedSize = nxsGetNumElements(tilizedShape);
    std::vector<T> buf_v(paddedSize, 0);
    char *data_ptr = (char *)getData();
    for (nxs_ulong i = 0; i < rowCount; i++) {
      std::copy(data_ptr, data_ptr + rowStride, buf_v.begin() + i * tilizedStride);
      data_ptr += rowStride;
    }
    buf_v = tilize_nfaces(buf_v, tilizedShape.dims[0], rowCount);

    // Size of a tile in bytes
    nxs_ulong tileSize = tileWidth * tileWidth * sizeof(T);

    ttmd::DeviceLocalBufferConfig dram_config{
        .page_size = tileSize,  // Number of bytes when round-robin between banks. Usually this is the same
                                      // as the tile size for efficiency.
        .buffer_type = ttm::BufferType::DRAM};  // Type of buffer (DRAM or L1(SRAM))
    ttmd::ReplicatedBufferConfig distributed_buffer_config{
        .size = paddedSize * sizeof(T)  // Size of the buffer in bytes
    };  
    // Create 3 buffers in DRAM to hold the 2 input tiles and 1 output tile.
    TT_OBJ_CHECK(buffer, ttmd::MeshBuffer::create, distributed_buffer_config, dram_config, device->get().get());
    auto &cq = device->getCQ();
    TT_CHECK(ttmd::EnqueueWriteMeshBuffer, cq, buffer, buf_v, true);
    // TODO: change to non-blocking and remove the finish
    TT_CHECK(ttmd::Finish, cq);

    address = buffer->address(); // defer until cq finished
    NXSAPI_LOG(nexus::NXS_LOG_NOTE, "TTBuffer: makeDeviceBuffer: tile_size=", tileSize, " size=", paddedSize);
  } else {
    address = static_cast<nxs_uint>(reinterpret_cast<uintptr_t>(data()));
  }
  return buffer;
}

nxs_status TTBuffer::copyToHost(void *host_buf) {
  if (buffer) {
    auto tile_size = 1024 * getDataTypeSize(getSettings());
    auto pad_size = size();
    if (size() % tile_size != 0) {
      pad_size = size() + tile_size - (size() % tile_size);
    }
    std::vector<nxs_uchar> buf_v(pad_size);
    auto &cq = device->getCQ();
    TT_CHECK(ttmd::EnqueueReadMeshBuffer, cq, buf_v, buffer, true);
    TT_CHECK(ttmd::Finish, cq);
    std::copy(buf_v.begin(), buf_v.begin() + size(), (nxs_uchar *)host_buf);
    NXSAPI_LOG(nexus::NXS_LOG_NOTE, "TTBuffer: copyToHost: tile_size=", tile_size, " size=", size());
  }
  return NXS_Success;
}