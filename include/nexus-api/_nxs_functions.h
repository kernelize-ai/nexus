/*
 */

/* clang-format off */

#if defined(NEXUS_API_GENERATE_FUNC_DECL)
/************************************************************************
 * Generate the Function declarations
 ***********************************************************************/
/* Generate the Function extern */
#define NEXUS_API_FUNC(RETURN_TYPE, NAME, ...) \
    extern NXS_API_EXTERN_C NXS_API_ENTRY RETURN_TYPE NXS_API_CALL nxs##NAME(__VA_ARGS__);

#else
#if defined(NEXUS_API_GENERATE_FUNC_ENUM)
/************************************************************************
 * Generate the Function Enum
 ***********************************************************************/
/* Generate the Enum name */
#define NEXUS_API_FUNC(RETURN_TYPE, NAME, ...) \
        NF_nxs##NAME,
    
/* Declare the Enumeration */
enum _nxs_function {

#else
#if defined(NEXUS_API_GENERATE_FUNC_TYPE)
/************************************************************************
 * Generate the Function typedefs
 ***********************************************************************/
 /* Generate enum lookup of Function type */
#ifdef __cplusplus
template <nxs_function Tfn>
struct nxsFunctionType { typedef void *type; };
#endif


 /* Generate the Function typedefs */
#define _NEXUS_API_FUNC(RETURN_TYPE, NAME, ...) \
    typedef RETURN_TYPE NXS_API_CALL NXS_CONCAT(nxs##NAME, _t)(__VA_ARGS__); \
    typedef NXS_CONCAT(nxs##NAME, _t) * NXS_CONCAT(nxs##NAME, _fn);

#ifdef __cplusplus
#define NEXUS_API_FUNC(RETURN_TYPE, NAME, ...) \
    _NEXUS_API_FUNC(RETURN_TYPE, NAME, __VA_ARGS__) \
    template <> struct nxsFunctionType<NF_nxs##NAME> { typedef NXS_CONCAT(nxs##NAME, _fn) type; };
#else
#define NEXUS_API_FUNC(RETURN_TYPE, NAME, ...) \
    _NEXUS_API_FUNC(RETURN_TYPE, NAME, __VA_ARGS__)
#endif

#endif
#endif
#endif


/************************************************************************
 * Define API Functions
 ***********************************************************************/

/************************************************************************
 * @def GetRuntimeProperty
 * @brief Return Runtime properties 
 ***********************************************************************/
NEXUS_API_FUNC(nxs_status, GetRuntimeProperty,
    nxs_uint runtime_property_id,
    void *property_value,
    size_t* property_value_size
)

/************************************************************************
 * @def GetDeviceProperty
 * @brief Return Device properties
 ***********************************************************************/
NEXUS_API_FUNC(nxs_status, GetDeviceProperty,
    nxs_int device_id,
    nxs_uint property_id,
    void *property_value,
    size_t* property_value_size
)

/************************************************************************
 * @def CreateBuffer
 * @brief Create buffer on the device
  * @return Negative value is an error status.
  *         Non-negative is the bufferId.
***********************************************************************/
NEXUS_API_FUNC(nxs_int, CreateBuffer,
    nxs_int device_id,
    size_t size,
    void* host_ptr,
    nxs_uint buffer_settings
)
/************************************************************************
 * @def GetBufferProperty
 * @brief Return Buffer properties 
 ***********************************************************************/
NEXUS_API_FUNC(nxs_status, GetBufferProperty,
    nxs_int buffer_id,
    nxs_uint buffer_property_id,
    void *property_value,
    size_t* property_value_size
)
/************************************************************************
 * @def CreateBuffer
 * @brief Create buffer on the device
  * @return Negative value is an error status.
  *         Non-negative is the bufferId.
***********************************************************************/
NEXUS_API_FUNC(nxs_status, CopyBuffer,
    nxs_int buffer_id,
    void* host_ptr,
    nxs_uint buffer_settings
)
/************************************************************************
 * @def ReleaseBuffer
 * @brief Release the buffer on the device
  * @return Error status or Succes.
***********************************************************************/
NEXUS_API_FUNC(nxs_status, ReleaseBuffer,
    nxs_int buffer_id
)


/************************************************************************
 * @def CreateLibrary
 * @brief Create command buffer on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
NEXUS_API_FUNC(nxs_int, CreateLibrary,
    nxs_int device_id,
    void *library_data,
    nxs_uint data_size,
    nxs_uint library_settings
)
/************************************************************************
 * @def CreateLibrary
 * @brief Create command buffer on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
NEXUS_API_FUNC(nxs_int, CreateLibraryFromFile,
    nxs_int device_id,
    const char *library_data,
    nxs_uint library_settings
)
/************************************************************************
 * @def GetLibraryProperty
 * @brief Return Library properties 
 ***********************************************************************/
NEXUS_API_FUNC(nxs_status, GetLibraryProperty,
    nxs_int library_id,
    nxs_uint library_property_id,
    void *property_value,
    size_t* property_value_size
)
/************************************************************************
 * @def ReleaseLibrary
 * @brief Release the buffer on the device
  * @return Error status or Succes.
***********************************************************************/
NEXUS_API_FUNC(nxs_status, ReleaseLibrary,
    nxs_int library_id
)

/************************************************************************
 * @def CreateLibrary
 * @brief Create command buffer on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
NEXUS_API_FUNC(nxs_int, GetKernel,
    nxs_int library_id,
    const char *kernel_name
)
/************************************************************************
 * @def GetKernelProperty
 * @brief Return Kernel properties 
 ***********************************************************************/
NEXUS_API_FUNC(nxs_status, GetKernelProperty,
    nxs_int kernel_id,
    nxs_uint kernel_property_id,
    void *property_value,
    size_t* property_value_size
)

/************************************************************************
 * @def CreateEvent
 * @brief Create event on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
NEXUS_API_FUNC(nxs_int, CreateEvent,
    nxs_int device_id,
    nxs_event_type event_type,
    nxs_uint event_settings
)
/************************************************************************
 * @def GetEventProperty
 * @brief Return Event properties 
 ***********************************************************************/
NEXUS_API_FUNC(nxs_status, GetEventProperty,
    nxs_int event_id,
    nxs_uint event_property_id,
    void *property_value,
    size_t* property_value_size
)
/************************************************************************
 * @def SignalEvent
 * @brief Signal the event on the device
 ***********************************************************************/
NEXUS_API_FUNC(nxs_status, SignalEvent,
    nxs_int event_id,
    nxs_int value
)
/************************************************************************
 * @def WaitEvent
 * @brief Wait for the event on the device
 ***********************************************************************/
NEXUS_API_FUNC(nxs_status, WaitEvent,
    nxs_int event_id,
    nxs_int value
)
/************************************************************************
 * @def ReleaseEvent
 * @brief Release the event on the device
  * @return Error status or Succes.
***********************************************************************/
NEXUS_API_FUNC(nxs_status, ReleaseEvent,
    nxs_int event_id
)

/************************************************************************
 * @def CreateStream
 * @brief Create stream on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
NEXUS_API_FUNC(nxs_int, CreateStream,
    nxs_int device_id,
    nxs_uint stream_settings
)
/************************************************************************
 * @def GetStreamProperty
 * @brief Return Stream properties 
 ***********************************************************************/
NEXUS_API_FUNC(nxs_status, GetStreamProperty,
    nxs_int stream_id,
    nxs_uint stream_property_id,
    void *property_value,
    size_t* property_value_size
)
/************************************************************************
 * @def ReleaseStream
 * @brief Release the buffer on the device
  * @return Error status or Succes.
***********************************************************************/
NEXUS_API_FUNC(nxs_status, ReleaseStream,
    nxs_int stream_id
)

/************************************************************************
 * @def CreateSchedule
 * @brief Create schedule on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
NEXUS_API_FUNC(nxs_int, CreateSchedule,
    nxs_int device_id,
    nxs_uint schedule_settings
)
/************************************************************************
 * @def GetScheduleProperty
 * @brief Return Schedule properties 
 ***********************************************************************/
NEXUS_API_FUNC(nxs_status, GetScheduleProperty,
    nxs_int schedule_id,
    nxs_uint schedule_property_id,
    void *property_value,
    size_t* property_value_size
)
/************************************************************************
 * @def ReleaseCommandList
 * @brief Release the buffer on the device
  * @return Error status or Succes.
***********************************************************************/
NEXUS_API_FUNC(nxs_status, RunSchedule,
    nxs_int schedule_id,
    nxs_int stream_id,
    nxs_uint run_settings
)
/************************************************************************
 * @def ReleaseCommandList
 * @brief Release the buffer on the device
  * @return Error status or Succes.
***********************************************************************/
NEXUS_API_FUNC(nxs_status, ReleaseSchedule,
    nxs_int schedule_id
)

/************************************************************************
 * @def CreateCommand
 * @brief Create command to launch a kernel on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
NEXUS_API_FUNC(nxs_int, CreateCommand,
    nxs_int schedule_id,
    nxs_int kernel_id,
    nxs_uint command_settings
)
/************************************************************************
 * @def CreateSignalCommand
 * @brief Create command to signal an event on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
NEXUS_API_FUNC(nxs_int, CreateSignalCommand,
    nxs_int schedule_id,
    nxs_int event_id,
    nxs_int signal_value,
    nxs_uint command_settings
)
/************************************************************************
 * @def CreateWaitCommand
 * @brief Create command to wait on an event on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
NEXUS_API_FUNC(nxs_int, CreateWaitCommand,
    nxs_int schedule_id,
    nxs_int event_id,
    nxs_int wait_value,
    nxs_uint command_settings
)
/************************************************************************
 * @def GetCommandProperty
 * @brief Return Command properties 
 ***********************************************************************/
NEXUS_API_FUNC(nxs_status, GetCommandProperty,
    nxs_int command_id,
    nxs_uint command_property_id,
    void *property_value,
    size_t* property_value_size
)
/************************************************************************
 * @def SetCommandArgument
 * @brief Set command argument
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
 NEXUS_API_FUNC(nxs_status, SetCommandArgument,
    nxs_int command_id,
    nxs_int argument_index,
    nxs_int buffer_id
)
/************************************************************************
 * @def SetCommandArgument
 * @brief Set command argument
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
 NEXUS_API_FUNC(nxs_status, SetCommandScalar,
    nxs_int command_id,
    nxs_int argument_index,
    void *value
)
/************************************************************************
 * @def FinalizeCommand
 * @brief Finalize command buffer on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
NEXUS_API_FUNC(nxs_status, FinalizeCommand,
    nxs_int command_id,
    nxs_dim3 grid_size,
    nxs_dim3 block_size,
    nxs_uint shared_mem_size
)


#ifdef NEXUS_API_GENERATE_FUNC_ENUM
    NXS_FUNCTION_CNT,
    NXS_FUNCTION_PREFIX_LEN = 3,
    
    NXS_FUNCTION_INVALID = -1
}; /* close _nxs_function */

typedef enum _nxs_function nxs_function;

const char *nxsGetFuncName(nxs_int funcEnum);
nxs_function nxsGetFuncEnum(const char *funcName);

#endif

/* clang-format on */

#undef NEXUS_API_GENERATE_FUNC_DECL
#undef NEXUS_API_GENERATE_FUNC_ENUM
#undef NEXUS_API_GENERATE_FUNC_TYPE

#undef NEXUS_API_FUNC
