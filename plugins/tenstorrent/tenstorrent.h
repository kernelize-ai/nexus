#ifndef RT_TT_H
#define RT_TT_H

#include <nexus-api.h>

#include <tt-metalium/host_api.hpp>
#include <tt-metalium/distributed.hpp>

namespace ttm = tt::tt_metal;
namespace ttmd = tt::tt_metal::distributed;

typedef std::shared_ptr<ttmd::MeshDevice> TTDevice;

inline tt::DataFormat getDataFormat(nxs_uint settings) {
    auto nxsFormat = settings & NXS_CommandArgDataType_Mask;
    switch (nxsFormat) {
        case NXS_CommandArgDataType_F32:
            return tt::DataFormat::Float32;
        case NXS_CommandArgDataType_F16:
            return tt::DataFormat::Float16;
        case NXS_CommandArgDataType_BF16:
            return tt::DataFormat::Float16_b;
        case NXS_CommandArgDataType_F8:
            return tt::DataFormat::Bfp8;
        case NXS_CommandArgDataType_BF8:
            return tt::DataFormat::Bfp8_b;
        case NXS_CommandArgDataType_I32:
            return tt::DataFormat::Int32;
        case NXS_CommandArgDataType_U32:
            return tt::DataFormat::UInt32;
        case NXS_CommandArgDataType_I16:
        //    return tt::DataFormat::Int16;
        case NXS_CommandArgDataType_U16:
            return tt::DataFormat::UInt16;
        case NXS_CommandArgDataType_I8:
            return tt::DataFormat::Int8;
        case NXS_CommandArgDataType_U8:
            return tt::DataFormat::UInt8;
        default:
            break;
    }
    return tt::DataFormat::Float32;
}

inline size_t getDataTypeSize(nxs_uint settings) {
    auto nxsFormat = settings & NXS_CommandArgDataType_Mask;
    switch (nxsFormat) {
        case NXS_CommandArgDataType_F32:
        case NXS_CommandArgDataType_I32:
        case NXS_CommandArgDataType_U32:
            return 4;
        case NXS_CommandArgDataType_F16:
        case NXS_CommandArgDataType_BF16:
        case NXS_CommandArgDataType_I16:
        case NXS_CommandArgDataType_U16:
            return 2;
        case NXS_CommandArgDataType_F8:
        case NXS_CommandArgDataType_BF8:
        case NXS_CommandArgDataType_I8:
        case NXS_CommandArgDataType_U8:
            return 1;
        default:
            break;
    }
    return 1;
}

#endif // RT_TT_H