#ifndef RT_TT_H
#define RT_TT_H

#include <nexus-api.h>
#include <nexus-api/nxs_log.h>

#define NXSAPI_LOG_MODULE "tt_runtime"

#include <rt_utilities.h>

#include <tt-metalium/host_api.hpp>
#include <tt-metalium/distributed.hpp>
#include <tt-metalium/work_split.hpp>

namespace ttm = tt::tt_metal;
namespace ttmd = tt::tt_metal::distributed;

inline tt::DataFormat getDataFormat(nxs_uint settings) {
    auto nxsFormat = settings & NXS_DataType_Mask;
    switch (nxsFormat) {
        case NXS_DataType_F32:
            return tt::DataFormat::Float32;
        case NXS_DataType_F16:
            return tt::DataFormat::Float16;
        case NXS_DataType_BF16:
            return tt::DataFormat::Float16_b;
        case NXS_DataType_F8:
            return tt::DataFormat::Bfp8;
        case NXS_DataType_BF8:
            return tt::DataFormat::Bfp8_b;
        case NXS_DataType_I32:
            return tt::DataFormat::Int32;
        case NXS_DataType_U32:
            return tt::DataFormat::UInt32;
        case NXS_DataType_I16:
        //    return tt::DataFormat::Int16;
        case NXS_DataType_U16:
            return tt::DataFormat::UInt16;
        case NXS_DataType_I8:
            return tt::DataFormat::Int8;
        case NXS_DataType_U8:
            return tt::DataFormat::UInt8;
        default:
            break;
    }
    return tt::DataFormat::Float32;
}

inline size_t getDataTypeSize(nxs_uint settings) {
    auto nxsFormat = settings & NXS_DataType_Mask;
    switch (nxsFormat) {
        case NXS_DataType_F32:
        case NXS_DataType_I32:
        case NXS_DataType_U32:
            return 4;
        case NXS_DataType_F16:
        case NXS_DataType_BF16:
        case NXS_DataType_I16:
        case NXS_DataType_U16:
            return 2;
        case NXS_DataType_F8:
        case NXS_DataType_BF8:
        case NXS_DataType_I8:
        case NXS_DataType_U8:
            return 1;
        default:
            break;
    }
    return 1;
}

// "TT_CHECK ", #call, nxs::rt::print_value(__VA_ARGS__));

#define TT_NOBJ_CHECK(obj, call, ...)                                      \
    NXSAPI_LOG(nexus::NXS_LOG_NOTE,                                           \
               "TT_CHECK ", #call, "(", #__VA_ARGS__, ")"); \
    auto obj = call(__VA_ARGS__);                                    


#define TT_OBJ_CHECK(obj, call, ...)                                      \
    NXSAPI_LOG(nexus::NXS_LOG_NOTE,                                           \
               "TT_CHECK ", #call, "(", #__VA_ARGS__, ")"); \
    obj = call(__VA_ARGS__);                                     \
    if (!obj) {                                                \
      NXSAPI_LOG(nexus::NXS_LOG_ERROR,                                          \
                 "TT error: ");                 \
    }                                                                        

#define TT_COND_CHECK(cond, call, ...)                                      \
    NXSAPI_LOG(nexus::NXS_LOG_NOTE,                                           \
               "TT_CHECK ", #call, "(", #__VA_ARGS__, ")"); \
    call(__VA_ARGS__);                                     \
    if (cond) {                                                \
      NXSAPI_LOG(nexus::NXS_LOG_ERROR,                                          \
                 "TT error: ", #cond);                 \
    }                                                                        

#define TT_CHECK(call, ...)                                      \
    NXSAPI_LOG(nexus::NXS_LOG_NOTE,                                           \
               "TT_CHECK ", #call, "(", #__VA_ARGS__, ")"); \
    call(__VA_ARGS__);                                    

#endif // RT_TT_H