#include <tt_buffer.h>
#include <tt_runtime.h>

TTBuffer::TTBuffer(TTDevice dev, size_t size,
                   void *data_ptr, nxs_uint settings)
  : Buffer(size, data_ptr, settings), device(dev) {
    if (device)
      makeDeviceBuffer();
}


TTBuffer::Buffer_sp TTBuffer::makeDeviceBuffer() {
  if (!(getSettings() & NXS_BufferSettings_OnDevice)) {
    //setSetting(NXS_BufferSettings_OnDevice);

    size_t tile_size = 1024 * getDataTypeSize(getSettings());
    assert(getSize() % tile_size == 0);
    ttmd::DeviceLocalBufferConfig dram_config{
        .page_size = tile_size,  // Number of bytes when round-robin between banks. Usually this is the same
                                      // as the tile size for efficiency.
        .buffer_type = ttm::BufferType::DRAM};  // Type of buffer (DRAM or L1(SRAM))
    ttmd::ReplicatedBufferConfig distributed_buffer_config{
        .size = getSize()  // Size of the buffer in bytes
    };  
    // Create 3 buffers in DRAM to hold the 2 input tiles and 1 output tile.
    TT_OBJ_CHECK(buffer, ttmd::MeshBuffer::create, distributed_buffer_config, dram_config, device.get());
    std::vector<nxs_uchar> buf_v(data(), data() + size());
    TT_NOBJ_CHECK(&cq, device->mesh_command_queue);
    TT_CHECK(ttmd::EnqueueWriteMeshBuffer, cq, buffer, buf_v, true); // TODO: change to non-blocking and remove the finish
    TT_CHECK(ttmd::Finish, cq);

    address = buffer->address();
    NXSAPI_LOG(nexus::NXS_LOG_NOTE, "TTBuffer: tile_size=", tile_size, " address=", address);
  }
  return buffer;
}

nxs_status TTBuffer::copyToHost(void *host_buf) {
  if (address != -1) {
    getDataTypeSize(getSettings());
    TT_NOBJ_CHECK(&cq, device->mesh_command_queue);
    TT_CHECK(cq.enqueue_read_mesh_buffer, host_buf, buffer, true);
    TT_CHECK(ttmd::Finish, cq);
  }
  return NXS_Success;
}