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
 * NXS_CommandArgType_CompileTime:
 *   - Compile time argument, needed for runtime launch setup
 */
enum _nxs_command_arg_type {
    NXS_CommandArgType_User = 0,
    NXS_CommandArgType_Launch = 1,
    NXS_CommandArgType_Constant = 3,
    NXS_CommandArgType_Mask = 3,
    NXS_CommandArgType_NextBitOffset = 3
};
typedef enum _nxs_command_arg_type nxs_command_arg_type;

enum _nxs_command_arg_data_type {
    NXS_CommandArgDataType_F32 = 1 << NXS_CommandArgType_NextBitOffset,
    NXS_CommandArgDataType_F16 = 2 << NXS_CommandArgType_NextBitOffset,
    NXS_CommandArgDataType_BF16 = 3 << NXS_CommandArgType_NextBitOffset,
    NXS_CommandArgDataType_F8 = 4 << NXS_CommandArgType_NextBitOffset,
    NXS_CommandArgDataType_BF8 = 5 << NXS_CommandArgType_NextBitOffset,
    NXS_CommandArgDataType_F4 = 6 << NXS_CommandArgType_NextBitOffset,
    NXS_CommandArgDataType_BF4 = 7 << NXS_CommandArgType_NextBitOffset,
    NXS_CommandArgDataType_I32 = 8 << NXS_CommandArgType_NextBitOffset,
    NXS_CommandArgDataType_U32 = 9 << NXS_CommandArgType_NextBitOffset,
    NXS_CommandArgDataType_I16 = 10 << NXS_CommandArgType_NextBitOffset,
    NXS_CommandArgDataType_U16 = 11 << NXS_CommandArgType_NextBitOffset,
    NXS_CommandArgDataType_I8 = 12 << NXS_CommandArgType_NextBitOffset,
    NXS_CommandArgDataType_U8 = 13 << NXS_CommandArgType_NextBitOffset,
    NXS_CommandArgDataType_I4 = 14 << NXS_CommandArgType_NextBitOffset,
    NXS_CommandArgDataType_U4 = 15 << NXS_CommandArgType_NextBitOffset,
    NXS_CommandArgDataType_Mask = 15 << NXS_CommandArgType_NextBitOffset,
    NXS_CommandArgDataType_NextBitOffset = NXS_CommandArgType_NextBitOffset + 4
};
typedef enum _nxs_command_arg_data_type nxs_command_arg_data_type;

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

enum _nxs_buffer_settings {
    NXS_BufferSettings_OnHost = 1 << 0,
    NXS_BufferSettings_OnDevice = 1 << 1,
    NXS_BufferSettings_Maintain = 1 << 2,
};
typedef enum _nxs_buffer_settings nxs_buffer_settings;


#define NXS_KERNEL_MAX_CONSTS                         64
#define NXS_KERNEL_MAX_ARGS                           64


/* nxs_bool */
#define NXS_FALSE                                    0
#define NXS_TRUE                                     1
#ifdef NXS_VERSION_1_2
#define NXS_BLOCKING                                 NXS_TRUE
#define NXS_NON_BLOCKING                             NXS_FALSE
#endif

/* nxs_platform_info */
#define NXS_RUNTIME_PROFILE                         0x0900
#define NXS_RUNTIME_VERSION                         0x0901
#define NXS_RUNTIME_NAME                            0x0902
#define NXS_RUNTIME_VENDOR                          0x0903
#define NXS_RUNTIME_EXTENSIONS                      0x0904
#ifdef NXS_VERSION_2_1
#define NXS_RUNTIME_HOST_TIMER_RESOLUTION           0x0905
#endif
#ifdef NXS_VERSION_3_0
#define NXS_RUNTIME_NUMERIC_VERSION                 0x0906
#define NXS_RUNTIME_EXTENSIONS_WITH_VERSION         0x0907
#endif

/* nxs_device_type - bitfield */
#define NXS_DEVICE_TYPE_DEFAULT                      (1 << 0)
#define NXS_DEVICE_TYPE_CPU                          (1 << 1)
#define NXS_DEVICE_TYPE_GPU                          (1 << 2)
#define NXS_DEVICE_TYPE_ACCELERATOR                  (1 << 3)
#ifdef NXS_VERSION_1_2
#define NXS_DEVICE_TYPE_CUSTOM                       (1 << 4)
#endif
#define NXS_DEVICE_TYPE_ALL                          0xFFFFFFFF

/* nxs_device_info */
#define NXS_DEVICE_TYPE                                   0x1000
#define NXS_DEVICE_VENDOR_ID                              0x1001
#define NXS_DEVICE_MAX_COMPUTE_UNITS                      0x1002
#define NXS_DEVICE_MAX_WORK_ITEM_DIMENSIONS               0x1003
#define NXS_DEVICE_MAX_WORK_GROUP_SIZE                    0x1004
#define NXS_DEVICE_MAX_WORK_ITEM_SIZES                    0x1005
#define NXS_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR            0x1006
#define NXS_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT           0x1007
#define NXS_DEVICE_PREFERRED_VECTOR_WIDTH_INT             0x1008
#define NXS_DEVICE_PREFERRED_VECTOR_WIDTH_LONG            0x1009
#define NXS_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT           0x100A
#define NXS_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE          0x100B
#define NXS_DEVICE_MAX_CLOCK_FREQUENCY                    0x100C
#define NXS_DEVICE_ADDRESS_BITS                           0x100D
#define NXS_DEVICE_MAX_READ_IMAGE_ARGS                    0x100E
#define NXS_DEVICE_MAX_WRITE_IMAGE_ARGS                   0x100F
#define NXS_DEVICE_MAX_MEM_ALLOC_SIZE                     0x1010
#define NXS_DEVICE_IMAGE2D_MAX_WIDTH                      0x1011
#define NXS_DEVICE_IMAGE2D_MAX_HEIGHT                     0x1012
#define NXS_DEVICE_IMAGE3D_MAX_WIDTH                      0x1013
#define NXS_DEVICE_IMAGE3D_MAX_HEIGHT                     0x1014
#define NXS_DEVICE_IMAGE3D_MAX_DEPTH                      0x1015
#define NXS_DEVICE_IMAGE_SUPPORT                          0x1016
#define NXS_DEVICE_MAX_PARAMETER_SIZE                     0x1017
#define NXS_DEVICE_MAX_SAMPLERS                           0x1018
#define NXS_DEVICE_MEM_BASE_ADDR_ALIGN                    0x1019
#define NXS_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE               0x101A
#define NXS_DEVICE_SINGLE_FP_CONFIG                       0x101B
#define NXS_DEVICE_GLOBAL_MEM_CACHE_TYPE                  0x101C
#define NXS_DEVICE_GLOBAL_MEM_CACHELINE_SIZE              0x101D
#define NXS_DEVICE_GLOBAL_MEM_CACHE_SIZE                  0x101E
#define NXS_DEVICE_GLOBAL_MEM_SIZE                        0x101F
#define NXS_DEVICE_MAX_CONSTANT_BUFFER_SIZE               0x1020
#define NXS_DEVICE_MAX_CONSTANT_ARGS                      0x1021
#define NXS_DEVICE_LOCAL_MEM_TYPE                         0x1022
#define NXS_DEVICE_LOCAL_MEM_SIZE                         0x1023
#define NXS_DEVICE_ERROR_CORRECTION_SUPPORT               0x1024
#define NXS_DEVICE_PROFILING_TIMER_RESOLUTION             0x1025
#define NXS_DEVICE_ENDIAN_LITTLE                          0x1026
#define NXS_DEVICE_AVAILABLE                              0x1027
#define NXS_DEVICE_COMPILER_AVAILABLE                     0x1028
#define NXS_DEVICE_EXECUTION_CAPABILITIES                 0x1029
#define NXS_DEVICE_QUEUE_PROPERTIES                       0x102A    /* deprecated */
#ifdef NXS_VERSION_2_0
#define NXS_DEVICE_QUEUE_ON_HOST_PROPERTIES               0x102A
#endif
#define NXS_DEVICE_NAME                                   0x102B
#define NXS_DEVICE_VENDOR                                 0x102C
#define NXS_DRIVER_VERSION                                0x102D
#define NXS_DEVICE_PROFILE                                0x102E
#define NXS_DEVICE_VERSION                                0x102F
#define NXS_DEVICE_EXTENSIONS                             0x1030
#define NXS_DEVICE_RUNTIME                               0x1031
#ifdef NXS_VERSION_1_2
#define NXS_DEVICE_DOUBLE_FP_CONFIG                       0x1032
#endif
/* 0x1033 reserved for NXS_DEVICE_HALF_FP_CONFIG which is already defined in "nxs_ext.h" */
#ifdef NXS_VERSION_1_1
#define NXS_DEVICE_PREFERRED_VECTOR_WIDTH_HALF            0x1034
#define NXS_DEVICE_HOST_UNIFIED_MEMORY                    0x1035   /* deprecated */
#define NXS_DEVICE_NATIVE_VECTOR_WIDTH_CHAR               0x1036
#define NXS_DEVICE_NATIVE_VECTOR_WIDTH_SHORT              0x1037
#define NXS_DEVICE_NATIVE_VECTOR_WIDTH_INT                0x1038
#define NXS_DEVICE_NATIVE_VECTOR_WIDTH_LONG               0x1039
#define NXS_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT              0x103A
#define NXS_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE             0x103B
#define NXS_DEVICE_NATIVE_VECTOR_WIDTH_HALF               0x103C
#define NXS_DEVICE_NEXUSAPI_C_VERSION                       0x103D
#endif
#ifdef NXS_VERSION_1_2
#define NXS_DEVICE_LINKER_AVAILABLE                       0x103E
#define NXS_DEVICE_BUILT_IN_KERNELS                       0x103F
#define NXS_DEVICE_IMAGE_MAX_BUFFER_SIZE                  0x1040
#define NXS_DEVICE_IMAGE_MAX_ARRAY_SIZE                   0x1041
#define NXS_DEVICE_PARENT_DEVICE                          0x1042
#define NXS_DEVICE_PARTITION_MAX_SUB_DEVICES              0x1043
#define NXS_DEVICE_PARTITION_PROPERTIES                   0x1044
#define NXS_DEVICE_PARTITION_AFFINITY_DOMAIN              0x1045
#define NXS_DEVICE_PARTITION_TYPE                         0x1046
#define NXS_DEVICE_REFERENCE_COUNT                        0x1047
#define NXS_DEVICE_PREFERRED_INTEROP_USER_SYNC            0x1048
#define NXS_DEVICE_PRINTF_BUFFER_SIZE                     0x1049
#endif
#ifdef NXS_VERSION_2_0
#define NXS_DEVICE_IMAGE_PITCH_ALIGNMENT                  0x104A
#define NXS_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT           0x104B
#define NXS_DEVICE_MAX_READ_WRITE_IMAGE_ARGS              0x104C
#define NXS_DEVICE_MAX_GLOBAL_VARIABLE_SIZE               0x104D
#define NXS_DEVICE_QUEUE_ON_DEVICE_PROPERTIES             0x104E
#define NXS_DEVICE_QUEUE_ON_DEVICE_PREFERRED_SIZE         0x104F
#define NXS_DEVICE_QUEUE_ON_DEVICE_MAX_SIZE               0x1050
#define NXS_DEVICE_MAX_ON_DEVICE_QUEUES                   0x1051
#define NXS_DEVICE_MAX_ON_DEVICE_EVENTS                   0x1052
#define NXS_DEVICE_SVM_CAPABILITIES                       0x1053
#define NXS_DEVICE_GLOBAL_VARIABLE_PREFERRED_TOTAL_SIZE   0x1054
#define NXS_DEVICE_MAX_PIPE_ARGS                          0x1055
#define NXS_DEVICE_PIPE_MAX_ACTIVE_RESERVATIONS           0x1056
#define NXS_DEVICE_PIPE_MAX_PACKET_SIZE                   0x1057
#define NXS_DEVICE_PREFERRED_RUNTIME_ATOMIC_ALIGNMENT    0x1058
#define NXS_DEVICE_PREFERRED_GLOBAL_ATOMIC_ALIGNMENT      0x1059
#define NXS_DEVICE_PREFERRED_LOCAL_ATOMIC_ALIGNMENT       0x105A
#endif
#ifdef NXS_VERSION_2_1
#define NXS_DEVICE_IL_VERSION                             0x105B
#define NXS_DEVICE_MAX_NUM_SUB_GROUPS                     0x105C
#define NXS_DEVICE_SUB_GROUP_INDEPENDENT_FORWARD_PROGRESS 0x105D
#endif
#ifdef NXS_VERSION_3_0
#define NXS_DEVICE_NUMERIC_VERSION                        0x105E
#define NXS_DEVICE_EXTENSIONS_WITH_VERSION                0x1060
#define NXS_DEVICE_ILS_WITH_VERSION                       0x1061
#define NXS_DEVICE_BUILT_IN_KERNELS_WITH_VERSION          0x1062
#define NXS_DEVICE_ATOMIC_MEMORY_CAPABILITIES             0x1063
#define NXS_DEVICE_ATOMIC_FENCE_CAPABILITIES              0x1064
#define NXS_DEVICE_NON_UNIFORM_WORK_GROUP_SUPPORT         0x1065
#define NXS_DEVICE_NEXUSAPI_C_ALL_VERSIONS                  0x1066
#define NXS_DEVICE_PREFERRED_WORK_GROUP_SIZE_MULTIPLE     0x1067
#define NXS_DEVICE_WORK_GROUP_COLLECTIVE_FUNCTIONS_SUPPORT 0x1068
#define NXS_DEVICE_GENERIC_ADDRESS_SPACE_SUPPORT          0x1069
/* 0x106A to 0x106E - Reserved for upcoming KHR extension */
#define NXS_DEVICE_NEXUSAPI_C_FEATURES                      0x106F
#define NXS_DEVICE_DEVICE_ENQUEUE_CAPABILITIES            0x1070
#define NXS_DEVICE_PIPE_SUPPORT                           0x1071
#define NXS_DEVICE_LATEST_CONFORMANCE_VERSION_PASSED      0x1072
#endif

/* nxs_device_fp_config - bitfield */
#define NXS_FP_DENORM                                (1 << 0)
#define NXS_FP_INF_NAN                               (1 << 1)
#define NXS_FP_ROUND_TO_NEAREST                      (1 << 2)
#define NXS_FP_ROUND_TO_ZERO                         (1 << 3)
#define NXS_FP_ROUND_TO_INF                          (1 << 4)
#define NXS_FP_FMA                                   (1 << 5)
#ifdef NXS_VERSION_1_1
#define NXS_FP_SOFT_FLOAT                            (1 << 6)
#endif
#ifdef NXS_VERSION_1_2
#define NXS_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT         (1 << 7)
#endif

/* nxs_device_mem_cache_type */
#define NXS_NONE                                     0x0
#define NXS_READ_ONLY_CACHE                          0x1
#define NXS_READ_WRITE_CACHE                         0x2

/* nxs_device_local_mem_type */
#define NXS_LOCAL                                    0x1
#define NXS_GLOBAL                                   0x2

/* nxs_device_exec_capabilities - bitfield */
#define NXS_EXEC_KERNEL                              (1 << 0)
#define NXS_EXEC_NATIVE_KERNEL                       (1 << 1)

/* nxs_command_queue_properties - bitfield */
#define NXS_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE      (1 << 0)
#define NXS_QUEUE_PROFILING_ENABLE                   (1 << 1)
#ifdef NXS_VERSION_2_0
#define NXS_QUEUE_ON_DEVICE                          (1 << 2)
#define NXS_QUEUE_ON_DEVICE_DEFAULT                  (1 << 3)
#endif

/* nxs_context_properties */
#define NXS_CONTEXT_RUNTIME                         0x1084
#ifdef NXS_VERSION_1_2
#define NXS_CONTEXT_INTEROP_USER_SYNC                0x1085
#endif

#ifdef NXS_VERSION_1_2

/* nxs_device_partition_property */
#define NXS_DEVICE_PARTITION_EQUALLY                 0x1086
#define NXS_DEVICE_PARTITION_BY_COUNTS               0x1087
#define NXS_DEVICE_PARTITION_BY_COUNTS_LIST_END      0x0
#define NXS_DEVICE_PARTITION_BY_AFFINITY_DOMAIN      0x1088

#endif

/* nxs_kernel_info */
#define NXS_KERNEL_FUNCTION_NAME                     0x1190
#define NXS_KERNEL_NUM_ARGS                          0x1191
#define NXS_KERNEL_REFERENCE_COUNT                   0x1192
#define NXS_KERNEL_CONTEXT                           0x1193
#define NXS_KERNEL_PROGRAM                           0x1194
#ifdef NXS_VERSION_1_2
#define NXS_KERNEL_ATTRIBUTES                        0x1195
#endif

#ifdef NXS_VERSION_1_2

/* nxs_kernel_arg_info */
#define NXS_KERNEL_ARG_ADDRESS_QUALIFIER             0x1196
#define NXS_KERNEL_ARG_ACCESS_QUALIFIER              0x1197
#define NXS_KERNEL_ARG_TYPE_NAME                     0x1198
#define NXS_KERNEL_ARG_TYPE_QUALIFIER                0x1199
#define NXS_KERNEL_ARG_NAME                          0x119A

#endif

#ifdef NXS_VERSION_1_2

/* nxs_kernel_arg_address_qualifier */
#define NXS_KERNEL_ARG_ADDRESS_GLOBAL                0x119B
#define NXS_KERNEL_ARG_ADDRESS_LOCAL                 0x119C
#define NXS_KERNEL_ARG_ADDRESS_CONSTANT              0x119D
#define NXS_KERNEL_ARG_ADDRESS_PRIVATE               0x119E

#endif

#ifdef NXS_VERSION_1_2

/* nxs_kernel_arg_access_qualifier */
#define NXS_KERNEL_ARG_ACCESS_READ_ONLY              0x11A0
#define NXS_KERNEL_ARG_ACCESS_WRITE_ONLY             0x11A1
#define NXS_KERNEL_ARG_ACCESS_READ_WRITE             0x11A2
#define NXS_KERNEL_ARG_ACCESS_NONE                   0x11A3

#endif

#ifdef NXS_VERSION_1_2

/* nxs_kernel_arg_type_qualifier */
#define NXS_KERNEL_ARG_TYPE_NONE                     0
#define NXS_KERNEL_ARG_TYPE_CONST                    (1 << 0)
#define NXS_KERNEL_ARG_TYPE_RESTRICT                 (1 << 1)
#define NXS_KERNEL_ARG_TYPE_VOLATILE                 (1 << 2)
#ifdef NXS_VERSION_2_0
#define NXS_KERNEL_ARG_TYPE_PIPE                     (1 << 3)
#endif

#endif

/* nxs_kernel_work_group_info */
#define NXS_KERNEL_WORK_GROUP_SIZE                   0x11B0
#define NXS_KERNEL_COMPILE_WORK_GROUP_SIZE           0x11B1
#define NXS_KERNEL_LOCAL_MEM_SIZE                    0x11B2
#define NXS_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE 0x11B3
#define NXS_KERNEL_PRIVATE_MEM_SIZE                  0x11B4
#ifdef NXS_VERSION_1_2
#define NXS_KERNEL_GLOBAL_WORK_SIZE                  0x11B5
#endif

#ifdef NXS_VERSION_2_1

/* nxs_kernel_sub_group_info */
#define NXS_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE    0x2033
#define NXS_KERNEL_SUB_GROUP_COUNT_FOR_NDRANGE       0x2034
#define NXS_KERNEL_LOCAL_SIZE_FOR_SUB_GROUP_COUNT    0x11B8
#define NXS_KERNEL_MAX_NUM_SUB_GROUPS                0x11B9
#define NXS_KERNEL_COMPILE_NUM_SUB_GROUPS            0x11BA

#endif

#ifdef NXS_VERSION_2_0

/* nxs_kernel_exec_info */
#define NXS_KERNEL_EXEC_INFO_SVM_PTRS                0x11B6
#define NXS_KERNEL_EXEC_INFO_SVM_FINE_GRAIN_SYSTEM   0x11B7

#endif

/* nxs_event_info */
#define NXS_EVENT_COMMAND_QUEUE                      0x11D0
#define NXS_EVENT_COMMAND_TYPE                       0x11D1
#define NXS_EVENT_REFERENCE_COUNT                    0x11D2
#define NXS_EVENT_COMMAND_EXECUTION_STATUS           0x11D3
#ifdef NXS_VERSION_1_1
#define NXS_EVENT_CONTEXT                            0x11D4
#endif

/* nxs_command_type */
#define NXS_COMMAND_NDRANGE_KERNEL                   0x11F0
#define NXS_COMMAND_TASK                             0x11F1
#define NXS_COMMAND_NATIVE_KERNEL                    0x11F2
#define NXS_COMMAND_READ_BUFFER                      0x11F3
#define NXS_COMMAND_WRITE_BUFFER                     0x11F4
#define NXS_COMMAND_COPY_BUFFER                      0x11F5
#define NXS_COMMAND_READ_IMAGE                       0x11F6
#define NXS_COMMAND_WRITE_IMAGE                      0x11F7
#define NXS_COMMAND_COPY_IMAGE                       0x11F8
#define NXS_COMMAND_COPY_IMAGE_TO_BUFFER             0x11F9
#define NXS_COMMAND_COPY_BUFFER_TO_IMAGE             0x11FA
#define NXS_COMMAND_MAP_BUFFER                       0x11FB
#define NXS_COMMAND_MAP_IMAGE                        0x11FC
#define NXS_COMMAND_UNMAP_MEM_OBJECT                 0x11FD
#define NXS_COMMAND_MARKER                           0x11FE
#define NXS_COMMAND_ACQUIRE_GL_OBJECTS               0x11FF
#define NXS_COMMAND_RELEASE_GL_OBJECTS               0x1200
#ifdef NXS_VERSION_1_1
#define NXS_COMMAND_READ_BUFFER_RECT                 0x1201
#define NXS_COMMAND_WRITE_BUFFER_RECT                0x1202
#define NXS_COMMAND_COPY_BUFFER_RECT                 0x1203
#define NXS_COMMAND_USER                             0x1204
#endif
#ifdef NXS_VERSION_1_2
#define NXS_COMMAND_BARRIER                          0x1205
#define NXS_COMMAND_MIGRATE_MEM_OBJECTS              0x1206
#define NXS_COMMAND_FILL_BUFFER                      0x1207
#define NXS_COMMAND_FILL_IMAGE                       0x1208
#endif
#ifdef NXS_VERSION_2_0
#define NXS_COMMAND_SVM_FREE                         0x1209
#define NXS_COMMAND_SVM_MEMCPY                       0x120A
#define NXS_COMMAND_SVM_MEMFILL                      0x120B
#define NXS_COMMAND_SVM_MAP                          0x120C
#define NXS_COMMAND_SVM_UNMAP                        0x120D
#endif
#ifdef NXS_VERSION_3_0
#define NXS_COMMAND_SVM_MIGRATE_MEM                  0x120E
#endif

/* command execution status */
#define NXS_COMPLETE                                 0x0
#define NXS_RUNNING                                  0x1
#define NXS_SUBMITTED                                0x2
#define NXS_QUEUED                                   0x3

/* nxs_buffer_create_type */
#ifdef NXS_VERSION_1_1
#define NXS_BUFFER_CREATE_TYPE_REGION                0x1220
#endif

/* nxs_profiling_info */
#define NXS_PROFILING_COMMAND_QUEUED                 0x1280
#define NXS_PROFILING_COMMAND_SUBMIT                 0x1281
#define NXS_PROFILING_COMMAND_START                  0x1282
#define NXS_PROFILING_COMMAND_END                    0x1283
#ifdef NXS_VERSION_2_0
#define NXS_PROFILING_COMMAND_COMPLETE               0x1284
#endif

/* nxs_device_atomic_capabilities - bitfield */
#ifdef NXS_VERSION_3_0
#define NXS_DEVICE_ATOMIC_ORDER_RELAXED          (1 << 0)
#define NXS_DEVICE_ATOMIC_ORDER_ACQ_REL          (1 << 1)
#define NXS_DEVICE_ATOMIC_ORDER_SEQ_CST          (1 << 2)
#define NXS_DEVICE_ATOMIC_SCOPE_WORK_ITEM        (1 << 3)
#define NXS_DEVICE_ATOMIC_SCOPE_WORK_GROUP       (1 << 4)
#define NXS_DEVICE_ATOMIC_SCOPE_DEVICE           (1 << 5)
#define NXS_DEVICE_ATOMIC_SCOPE_ALL_DEVICES      (1 << 6)
#endif

/* nxs_device_device_enqueue_capabilities - bitfield */
#ifdef NXS_VERSION_3_0
#define NXS_DEVICE_QUEUE_SUPPORTED               (1 << 0)
#define NXS_DEVICE_QUEUE_REPLACEABLE_DEFAULT     (1 << 1)
#endif

/* nxs_khronos_vendor_id */
#define NXS_KHRONOS_VENDOR_ID_CODEPLAY               0x10004

/* nxs_version */
#define NXS_VERSION_MAJOR_BITS (10)
#define NXS_VERSION_MINOR_BITS (10)
#define NXS_VERSION_PATCH_BITS (12)

#define NXS_VERSION_MAJOR_MASK ((1 << NXS_VERSION_MAJOR_BITS) - 1)
#define NXS_VERSION_MINOR_MASK ((1 << NXS_VERSION_MINOR_BITS) - 1)
#define NXS_VERSION_PATCH_MASK ((1 << NXS_VERSION_PATCH_BITS) - 1)

#define NXS_VERSION_MAJOR(version) \
  ((version) >> (NXS_VERSION_MINOR_BITS + NXS_VERSION_PATCH_BITS))

#define NXS_VERSION_MINOR(version) \
  (((version) >> NXS_VERSION_PATCH_BITS) & NXS_VERSION_MINOR_MASK)

#define NXS_VERSION_PATCH(version) ((version) & NXS_VERSION_PATCH_MASK)

#define NXS_MAKE_VERSION(major, minor, patch)                      \
  ((((major) & NXS_VERSION_MAJOR_MASK)                             \
       << (NXS_VERSION_MINOR_BITS + NXS_VERSION_PATCH_BITS)) |      \
   (((minor) & NXS_VERSION_MINOR_MASK) << NXS_VERSION_PATCH_BITS) | \
   ((patch) & NXS_VERSION_PATCH_MASK))

/********************************************************************************************************/

#ifdef __cplusplus
}
#endif

#endif  /* __NEXUSAPI_NXS_H */
