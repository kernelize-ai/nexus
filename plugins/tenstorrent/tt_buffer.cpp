#include <tt_buffer.h>
#include <tt_device.h>
#include <tt_runtime.h>

TTBuffer::TTBuffer(TTDevice *dev, size_t size,
                   void *data_ptr, nxs_uint settings)
  : Buffer(size, data_ptr, settings), device(dev) {
    if (device)
      makeDeviceBuffer();
}


TTBuffer::Buffer_sp TTBuffer::makeDeviceBuffer() {
  if (!(getSettings() & NXS_BufferSettings_OnDevice)) {
    setSettings(getSettings() | NXS_BufferSettings_OnDevice);

    size_t tile_size = 1024 * getDataTypeSize(getSettings());
    //assert(getSize() % tile_size == 0);
    auto pad_size = size();
    if (size() % tile_size != 0) {
      pad_size = size() + tile_size - (size() % tile_size);
    }
    std::vector<nxs_uchar> buf_v(pad_size, 0);
    std::copy(data(), data() + size(), buf_v.begin());
    ttmd::DeviceLocalBufferConfig dram_config{
        .page_size = tile_size,  // Number of bytes when round-robin between banks. Usually this is the same
                                      // as the tile size for efficiency.
        .buffer_type = ttm::BufferType::DRAM};  // Type of buffer (DRAM or L1(SRAM))
    ttmd::ReplicatedBufferConfig distributed_buffer_config{
        .size = pad_size  // Size of the buffer in bytes
    };  
    // Create 3 buffers in DRAM to hold the 2 input tiles and 1 output tile.
    TT_OBJ_CHECK(buffer, ttmd::MeshBuffer::create, distributed_buffer_config, dram_config, device->get().get());
    auto &cq = device->getCQ();
    TT_CHECK(ttmd::EnqueueWriteMeshBuffer, cq, buffer, buf_v, true); // TODO: change to non-blocking and remove the finish
    TT_CHECK(ttmd::Finish, cq);

    address = buffer->address(); // defer until cq finished
    NXSAPI_LOG(nexus::NXS_LOG_NOTE, "TTBuffer: makeDeviceBuffer: tile_size=", tile_size, " size=", size());
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