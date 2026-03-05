#include <tt_buffer.h>
#include <tt_device.h>
#include <tt_runtime.h>

#include <tt-metalium/tilize_utils.hpp>

static bool compareShapes(nxs_buffer_layout shape1, nxs_buffer_layout shape2) {
  if (shape1.rank != shape2.rank) {
    return false;
  }
  for (nxs_uint i = 0; i < shape1.rank; i++) {
    if (shape1.dim[i] != shape2.dim[i]) {
      return false;
    }
  }
  return true;
}

TTBuffer::TTBuffer(TTDevice *dev, nxs_buffer_layout shape,
                   void *data_ptr, nxs_uint settings)
  : Buffer(shape, data_ptr, settings), device(dev) {
    if (shape.rank != 0) {
      assert(shape.rank >= 2 && "Shape must be at least 2D");

      // Pad up to the nearest tile size
      rowCount = 1;
      paddedSize = 1;
      for (nxs_uint i = 0; i < shape.rank; i++) {
        tilizedShape.dim[i] = ((shape.dim[i] + tileWidth - 1) / tileWidth) * tileWidth;
        paddedSize *= tilizedShape.dim[i];
        if (i != 0) rowCount *= tilizedShape.dim[i];
      }

      // Size of a tile in bytes
      elementSize = getElementSizeBits() / 8;
      nxs_ulong tileSizeBytes = tileWidth * tileWidth * elementSize;

      // Create buffer in DRAM.
      NXSAPI_LOG(nexus::NXS_LOG_NOTE, "TTBuffer: tile_size=", tileSizeBytes, " size=", paddedSize);
      ttmd::DeviceLocalBufferConfig dram_config{
          .page_size = tileSizeBytes,  // Number of bytes when round-robin between banks. Usually this is the same
                                        // as the tile size for efficiency.
          .buffer_type = ttm::BufferType::DRAM};  // Type of buffer (DRAM or L1(SRAM))
      ttmd::ReplicatedBufferConfig distributed_buffer_config{
          .size = paddedSize * elementSize  // Size of the buffer in bytes
      };
      TT_OBJ_CHECK(buffer, ttmd::MeshBuffer::create, distributed_buffer_config, dram_config, device->get().get());

      if (!(getSettings() & NXS_BufferSettings_OnDevice) && data_ptr != nullptr) {
        setSettings(getSettings() | NXS_BufferSettings_OnDevice);
        copyToDevice(getData(), false);
      } else {
        address = static_cast<nxs_uint>(reinterpret_cast<uintptr_t>(getData()));
      }
    }
}

template <typename T>
nxs_status TTBuffer::tilizeAndCopyToDevice(T *data_ptr, bool blocking) {
  nxs_ulong tilizedStride = tilizedShape.dim[0];

  auto shape = getShape();
  nxs_ulong rowStride = shape.dim[0];
  std::vector<T> buf_v(paddedSize, 0);
  for (nxs_ulong i = 0; i < rowCount; i++) {
    std::copy(data_ptr, data_ptr + rowStride, buf_v.begin() + i * tilizedStride);
    data_ptr += rowStride;
  }
  buf_v = tilize_nfaces(buf_v, tilizedShape.dim[0], rowCount);

  auto &cq = device->getCQ();
  TT_CHECK(ttmd::EnqueueWriteMeshBuffer, cq, buffer, buf_v, blocking);
  if (blocking) {
    TT_CHECK(ttmd::Finish, cq);
  }

  address = buffer->address(); // defer until cq finished
  NXSAPI_LOG(nexus::NXS_LOG_NOTE, "TTBuffer: tilizeAndCopyToDevice: address=", address, " size=", paddedSize);
  return NXS_Success;
}

nxs_status TTBuffer::copyToDevice(void *host_buf, bool blocking) {
  switch (getDataType()) {
    case NXS_DataType_F32:
      return tilizeAndCopyToDevice<float>(reinterpret_cast<float *>(getData()), blocking);
      break;
    case NXS_DataType_BF16:
      return tilizeAndCopyToDevice<bfloat16>(reinterpret_cast<bfloat16 *>(getData()), blocking);
      break;
    case NXS_DataType_U32:
      return tilizeAndCopyToDevice<uint32_t>(reinterpret_cast<uint32_t *>(getData()), blocking);
      break;
    case NXS_DataType_U16:
      return tilizeAndCopyToDevice<uint16_t>(reinterpret_cast<uint16_t *>(getData()), blocking);
      break;
    default:
      NXSAPI_LOG(nexus::NXS_LOG_ERROR, "TTBuffer: Unsupported data type: ",
        nxsGetDataTypeName(getDataType()));
      break;
  }
  return NXS_Success;
}

template <typename T>
nxs_status TTBuffer::copyToHostUntilize(T *data_ptr) {
  NXSAPI_LOG(nexus::NXS_LOG_NOTE, "TTBuffer: copyToHostUntilize: address=", address, " size=", paddedSize);
  std::vector<T> buf_v(paddedSize);
  auto &cq = device->getCQ();
  TT_CHECK(ttmd::EnqueueReadMeshBuffer, cq, buf_v, buffer, true);
  TT_CHECK(ttmd::Finish, cq);

  nxs_ulong tilizedStride = tilizedShape.dim[0];
  buf_v = untilize_nfaces(buf_v, tilizedStride, rowCount);

  T *tbuf_ptr = reinterpret_cast<T *>(buf_v.data());
  nxs_ulong rowStride = getShape().dim[0] * elementSize;
  nxs_ulong tilizedRowStride = tilizedStride * elementSize;
  for (nxs_ulong i = 0; i < rowCount; i++) {
    std::copy(tbuf_ptr, tbuf_ptr + rowStride, data_ptr);
    data_ptr += rowStride;
    tbuf_ptr += tilizedRowStride;
  }

  return NXS_Success;
}

nxs_status TTBuffer::copyToHost(void *host_buf) {
  if (buffer) {
    switch (getDataType()) {
      case NXS_DataType_F32:
        return copyToHostUntilize<float>(reinterpret_cast<float *>(host_buf));
        break;
      case NXS_DataType_BF16:
        return copyToHostUntilize<bfloat16>(reinterpret_cast<bfloat16 *>(host_buf));
        break;
      case NXS_DataType_U32:
        return copyToHostUntilize<uint32_t>(reinterpret_cast<uint32_t *>(host_buf));
        break;
      case NXS_DataType_U16:
        return copyToHostUntilize<uint16_t>(reinterpret_cast<uint16_t *>(host_buf));
        break;
      default:
        NXSAPI_LOG(nexus::NXS_LOG_ERROR, "TTBuffer: Unsupported data type: ",
          nxsGetDataTypeName(getDataType()));
        break;
    }
  }
  return NXS_Success;
}

