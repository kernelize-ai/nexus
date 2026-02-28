/*
 * Copyright (c) 2023 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * OpenCL is a trademark of Apple Inc. used under license by Khronos.
 */


#include <nexus-api.h>
#include <magic_enum/magic_enum.hpp>

const char *nxsGetFuncName(nxs_int funcEnum) {
    auto fenum = magic_enum::enum_cast<nxs_function>(funcEnum);
    if (fenum)
        return magic_enum::enum_name(*fenum).data() + NXS_FUNCTION_PREFIX_LEN;
    return "";
}

nxs_function nxsGetFuncEnum(const char *funcName) {
    std::string fname = std::string("NF_") + funcName;
    if (auto val = magic_enum::enum_cast<nxs_function>(fname))
        return *val;
    return NXS_FUNCTION_INVALID;
}


const char *nxsGetPropName(nxs_int propEnum) {
    auto penum = magic_enum::enum_cast<nxs_property>(propEnum);
    if (penum)
        return magic_enum::enum_name(*penum).data() + NXS_PROPERTY_PREFIX_LEN;
    return "";
}

nxs_property nxsGetPropEnum(const char *propName) {
    std::string pname = std::string("NP_") + propName;
    if (auto val = magic_enum::enum_cast<nxs_property>(pname))
        return *val;
    return NXS_PROPERTY_INVALID;
}

const char *nxsGetStatusName(nxs_int statusEnum) {
    auto senum = magic_enum::enum_cast<nxs_status>(statusEnum);
    if (senum)
        return magic_enum::enum_name(*senum).data() + NXS_STATUS_PREFIX_LEN;
    return "";
}

nxs_status nxsGetStatusEnum(const char *statusName) {
    std::string sname = std::string("NXS_") + statusName;
    return *magic_enum::enum_cast<nxs_status>(sname);
}

nxs_data_type nxsGetDataType(nxs_uint settings) {
    return (nxs_data_type)(settings & NXS_DataType_Mask);
}

nxs_uint nxsGetDataTypeFlags(nxs_uint settings) {
    return settings & NXS_DataType_Flags;
}

nxs_uint nxsGetDataTypeSizeBits(nxs_uint settings) {
    switch (nxsGetDataType(settings)) {
        case NXS_DataType_F32:
        case NXS_DataType_I32:
        case NXS_DataType_U32:
            return 32;
        case NXS_DataType_F16:
        case NXS_DataType_BF16:
        case NXS_DataType_I16:
        case NXS_DataType_U16:
            return 16;
        case NXS_DataType_F8:
        case NXS_DataType_BF8:
        case NXS_DataType_I8:
        case NXS_DataType_U8:
            return 8;
        case NXS_DataType_F4:
        case NXS_DataType_BF4:
        case NXS_DataType_I4:
        case NXS_DataType_U4:
            return 4;
        case NXS_DataType_F64:
        case NXS_DataType_I64:
        case NXS_DataType_U64:
            return 64;
        case NXS_DataType_Bool:
            return 8;
        default:
            break;
    }
    return 0;
}

nxs_ulong nxsGetNumElements(nxs_shape shape) {
    nxs_ulong num_elements = 1;
    for (nxs_uint i = 0; i < shape.rank; i++) {
        num_elements *= shape.dims[i];
    }
    return num_elements;
}

const char *nxsGetDataTypeName(nxs_data_type type) {
    auto fenum = magic_enum::enum_cast<nxs_data_type>(type);
    if (fenum)
        return magic_enum::enum_name(*fenum).data() + NXS_DataType_PREFIX_LEN;
    return "";
}
