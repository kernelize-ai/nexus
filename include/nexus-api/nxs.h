/*******************************************************************************
 ******************************************************************************/

#ifndef __NEXUSAPI_NXS_H
#define __NEXUSAPI_NXS_H

#include <nexus-api/nxs_platform.h>
#include <nexus-api/nxs_version.h>

/* clang-format off */

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/

typedef nxs_uint             nxs_bool;                     /* WARNING!  Unlike nxs_ types in nxs_platform.h, nxs_bool is not guaranteed to be the same size as the bool in kernels. */

/******************************************************************************/

/* Error Codes */
enum _nxs_status {
    NXS_Success                              = 0,
    NXS_DeviceNotFound                       = -1,
    NXS_DeviceNotAvailable                   = -2,
    NXS_CompilerNotAvailable                 = -3,
    NXS_MemObjectAllocationFailure           = -4,
    NXS_OutOfResources                       = -5,
    NXS_OutOfHostMemory                      = -6,
    NXS_ProfilingInfoNotAvailable            = -7,
    NXS_MemCopyOverlap                       = -8,
    NXS_ImageFormatMismatch                  = -9,
    NXS_ImageFormatNotSupported              = -10,
    NXS_BuildProgramFailure                  = -11,
    NXS_MapFailure                           = -12,
    NXS_MisalignedSubBufferOffset            = -13,
    NXS_ExecStatusErrorForEventsInWaitList   = -14,
    NXS_CompileProgramFailure                = -15,
    NXS_LinkerNotAvailable                   = -16,
    NXS_LinkProgramFailure                   = -17,
    NXS_DevicePartitionFailed                = -18,
    NXS_KernelArgInfoNotAvailable            = -19,
    NXS_InvalidValue                         = -30,
    NXS_InvalidDeviceType                    = -31,
    NXS_InvalidContext                       = -34,
    NXS_InvalidQueueProperties               = -35,
    NXS_InvalidCommandQueue                  = -36,
    NXS_InvalidHostPtr                       = -37,
    NXS_InvalidMemObject                     = -38,
    NXS_InvalidImageFormatDescriptor         = -39,
    NXS_InvalidImageSize                     = -40,
    NXS_InvalidSampler                       = -41,
    NXS_InvalidBinary                        = -42,
    NXS_InvalidBuildOptions                  = -43,
    NXS_InvalidProgram                       = -44,
    NXS_InvalidProgramExecutable             = -45,
    NXS_InvalidKernelName                    = -46,
    NXS_InvalidKernelDefinition              = -47,
    NXS_InvalidKernel                        = -48,
    NXS_InvalidArgIndex                      = -49,
    NXS_InvalidArgValue                      = -50,
    NXS_InvalidArgSize                       = -51,
    NXS_InvalidKernelArgs                    = -52,
    NXS_InvalidWorkDimension                 = -53,
    NXS_InvalidWorkGroupSize                 = -54,
    NXS_InvalidWorkItemSize                  = -55,
    NXS_InvalidGlobalOffset                  = -56,
    NXS_InvalidEventWaitList                 = -57,
    NXS_InvalidEvent                         = -58,
    NXS_InvalidOperation                     = -59,
    NXS_InvalidGlObject                      = -60,
    NXS_InvalidBufferSize                    = -61,
    NXS_InvalidMipLevel                      = -62,
    NXS_InvalidGlobalWorkSize                = -63,
    NXS_InvalidProperty                      = -64,
    NXS_InvalidImageDescriptor               = -65,
    NXS_InvalidCompilerOptions               = -66,
    NXS_InvalidDeviceQueue                   = -70,
    NXS_InvalidSpecId                        = -71,
    NXS_MaxSizeRestrictionExceeded           = -72,
    NXS_InvalidObject                        = -80,
    NXS_InvalidBuffer                        = -81,
    NXS_InvalidCommand                       = -82,
    NXS_InvalidDevice                        = -83,
    NXS_InvalidLibrary                       = -84,
    NXS_InvalidRuntime                       = -85,
    NXS_InvalidSchedule                      = -86,
    NXS_InvalidStream                        = -87,
    NXS_InvalidSystem                        = -88,

    NXS_STATUS_MIN                           = -88,
    NXS_STATUS_MAX                           = 0,
    NXS_STATUS_PREFIX_LEN                    = 4
};

typedef enum _nxs_status nxs_status;

/* ID test functions*/
inline nxs_bool nxs_success(nxs_int result) { return result >= 0; }
inline nxs_bool nxs_failed(nxs_int result) { return result < 0; }

inline nxs_bool nxs_valid_id(nxs_int id) { return id >= 0; }


/* ENUM nxs_event_type
 *
 * NXS_EventType_Shared:
 *   - Event is shared between multiple streams
 *   - Event is signaled when a signal command for this event is complete
 *   - Event is waited on by multiple streams for specific signal values
 * NXS_EventType_Signal:
 *   - Event is signaled when a signal command is complete
 *   - Event is waited on by a wait stream
 * NXS_EventType_Fence:
 *   - Event is signaled when a kernel command is complete
 *   - Event is waited on by a kernel command
 */
enum _nxs_event_type {
    NXS_EventType_Shared = 0,
    NXS_EventType_Signal = 1,
    NXS_EventType_Fence = 2,
};
typedef enum _nxs_event_type nxs_event_type;

enum _nxs_execution_settings {
    NXS_ExecutionSettings_Profiling = 1 << 0,
    NXS_ExecutionSettings_Timing = 1 << 1,
    NXS_ExecutionSettings_Capture = 1 << 2,
    NXS_ExecutionSettings_NonBlocking = 1 << 3,
};
typedef enum _nxs_execution_settings nxs_execution_settings;

/* ENUM nxs_event_status */
/*
 * NXS_EventStatus_Submitted:
 *   - Event is submitted to a command queue
 * NXS_EventStatus_Running:
 *   - Event is running
 * NXS_EventStatus_Complete:
 *   - Event is complete
 * NXS_EventStatus_Error:
 *   - Event has an error
 * NXS_EventStatus_Canceled:
 *   - Event was canceled
 */
enum _nxs_event_status {
    NXS_EventStatus_Submitted = 0,
    NXS_EventStatus_Running = 1,
    NXS_EventStatus_Complete = 2,
    NXS_EventStatus_Error = 3,
    NXS_EventStatus_Canceled = 4,
};
typedef enum _nxs_event_status nxs_event_status;

/* ENUM nxs_command_type */
/*
 * NXS_CommandType_Dispatch:
 *   - Command is a kernel dispatch
 * NXS_CommandType_Signal:
 *   - Command is a signal command
 * NXS_CommandType_Wait:
 *   - Command is a wait command
 */
enum _nxs_command_type {
    NXS_CommandType_Dispatch = 0,
    NXS_CommandType_Signal = 1,
    NXS_CommandType_Wait = 2,
};
typedef enum _nxs_command_type nxs_command_type;

/* ENUM nxs_command_arg_type */
/*
 * NXS_CommandArgType_User:
 *   - Argument specified by User from the original kernel source
 * NXS_CommandArgType_Launch:
 *   - Launch argument specified by runtime
 * NXS_CommandArgType_ProgramId:
 *   - Program ID passed by runtime
 * NXS_CommandArgType_Constant:
 *   - Compile time argument, needed for runtime launch setup
 * NXS_CommandArgType_Mask:
 *   - Mask argument specified by runtime
 * NXS_CommandArgType_NextBitOffset:
 *   - Next bit offset for the next argument type
 */
enum _nxs_command_arg_type {
    NXS_CommandArgType_User = 0,
    NXS_CommandArgType_Launch = 1,
    NXS_CommandArgType_Constant = 2,
    NXS_CommandArgType_Mask = 3,
    NXS_CommandArgType_NextBitOffset = 3
};
typedef enum _nxs_command_arg_type nxs_command_arg_type;

/* ENUM nxs_data_type */
/*
 * NXS_DataType_Undefined:
 *   - Undefined data type
 */
enum _nxs_data_type {
    NXS_DataType_Undefined = 0,
    NXS_DataType_F32 = 1 << NXS_CommandArgType_NextBitOffset,
    NXS_DataType_F16 = 2 << NXS_CommandArgType_NextBitOffset,
    NXS_DataType_BF16 = 3 << NXS_CommandArgType_NextBitOffset,
    NXS_DataType_F8 = 4 << NXS_CommandArgType_NextBitOffset,
    NXS_DataType_BF8 = 5 << NXS_CommandArgType_NextBitOffset,
    NXS_DataType_F4 = 6 << NXS_CommandArgType_NextBitOffset,
    NXS_DataType_BF4 = 7 << NXS_CommandArgType_NextBitOffset,
    NXS_DataType_I32 = 8 << NXS_CommandArgType_NextBitOffset,
    NXS_DataType_U32 = 9 << NXS_CommandArgType_NextBitOffset,
    NXS_DataType_I16 = 10 << NXS_CommandArgType_NextBitOffset,
    NXS_DataType_U16 = 11 << NXS_CommandArgType_NextBitOffset,
    NXS_DataType_I8 = 12 << NXS_CommandArgType_NextBitOffset,
    NXS_DataType_U8 = 13 << NXS_CommandArgType_NextBitOffset,
    NXS_DataType_I4 = 14 << NXS_CommandArgType_NextBitOffset,
    NXS_DataType_U4 = 15 << NXS_CommandArgType_NextBitOffset,
    NXS_DataType_F64 = 16 << NXS_CommandArgType_NextBitOffset,
    NXS_DataType_I64 = 17 << NXS_CommandArgType_NextBitOffset,
    NXS_DataType_U64 = 18 << NXS_CommandArgType_NextBitOffset,
    NXS_DataType_Bool = 19 << NXS_CommandArgType_NextBitOffset,
    NXS_DataType_Mask = 31 << NXS_CommandArgType_NextBitOffset,
    NXS_DataType_PREFIX_LEN = 13,
    /* Flags */
    NXS_DataType_Block = 32 << NXS_CommandArgType_NextBitOffset,
    NXS_DataType_Flags = 1 << (NXS_CommandArgType_NextBitOffset + 5),
    NXS_DataType_NextBitOffset = NXS_CommandArgType_NextBitOffset + 6
};
typedef enum _nxs_data_type nxs_data_type;

nxs_data_type nxsGetDataType(nxs_uint settings);
nxs_uint nxsGetDataTypeFlags(nxs_uint settings);
nxs_uint nxsGetDataTypeSizeBits(nxs_uint settings);
nxs_ulong nxsGetNumElements(nxs_shape shape);
const char *nxsGetDataTypeName(nxs_data_type type);

/* ENUM nxs_command_queue_properties
 *
 * NXS_CommandQueueProperty_OutOfOrderExecution:
 *   - Command queue supports out-of-order execution
 * NXS_CommandQueueProperty_Profiling:
 *   - Command queue supports profiling
 */
enum _nxs_stream_settings {
    NXS_StreamSettings_OutOfOrderExecution = 1 << 0,
    NXS_StreamSettings_Profiling = 1 << 1,
};
typedef enum _nxs_stream_settings nxs_stream_settings;

/* ENUM nxs_buffer_settings */
/*
 * NXS_BufferSettings_OnHost:
 *   - Buffer is on host
 * NXS_BufferSettings_OnDevice:
 *   - Buffer is on device
 * NXS_BufferSettings_Maintain:
 *   - Buffer is maintained by the runtime
 */
enum _nxs_buffer_settings {
    NXS_BufferSettings_OnHost = 1 << 0,
    NXS_BufferSettings_OnDevice = 1 << 1,
    NXS_BufferSettings_Maintain = 1 << 2,
};
typedef enum _nxs_buffer_settings nxs_buffer_settings;

/* ENUM _nxs_buffer_transfer */
/*
 * NXS_BufferDeviceToHost:
 *   - Copy buffer from device to host
 * NXS_BufferHostToDevice:
 *   - Copy buffer from host to device
 */
enum _nxs_buffer_transfer {
    NXS_BufferDeviceToHost = 0,
    NXS_BufferHostToDevice = 1,
};

/********************************************************************************************************/
/* Constants */

#define NXS_KERNEL_MAX_CONSTS                         64
#define NXS_KERNEL_MAX_ARGS                           64

/********************************************************************************************************/

#ifdef __cplusplus
}
#endif

#endif  /* __NEXUSAPI_NXS_H */
