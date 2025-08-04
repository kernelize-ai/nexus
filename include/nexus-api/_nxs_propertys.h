/*
 */

/* clang-format off */

#if defined(NEXUS_API_GENERATE_PROP_ENUM)
/************************************************************************
 * Generate the Function declarations
 ***********************************************************************/
// Generate the Function extern

#define NEXUS_API_PROP(NAME, TYPE, DESC) \
    NP_##NAME,

enum _nxs_property {

#else
#if defined(NEXUS_API_GENERATE_PROP_MAP)
/************************************************************************
 * Generate the Property type trait lookup
 ***********************************************************************/
enum _nxs_property_type {
    NPT_INT,
    NPT_FLT,
    NPT_STR,
    NPT_INT_VEC,
    NPT_FLT_VEC,
    NPT_STR_VEC,
    NPT_OBJ_VEC,
    NPT_UNK
};

typedef enum _nxs_property_type nxs_property_type;

#define _prop_int NPT_INT
#define _prop_flt NPT_FLT
#define _prop_str NPT_STR
#define _prop_int_vec NPT_INT_VEC
#define _prop_flt_vec NPT_FLT_VEC
#define _prop_str_vec NPT_STR_VEC
#define _prop_obj_vec NPT_OBJ_VEC

static nxs_property_type nxs_property_type_map[] = {

#define NEXUS_API_PROP(NAME, TYPE, DESC) \
    TYPE,

#else
#if defined(NEXUS_API_GENERATE_PROP_TYPE)
/************************************************************************
 * Generate the Property type trait lookup
 ***********************************************************************/
typedef nxs_long _prop_int;
typedef nxs_double _prop_flt;

#ifdef __cplusplus

typedef std::string _prop_str;
typedef std::vector<nxs_long> _prop_int_vec;
typedef std::vector<nxs_double> _prop_flt_vec;
typedef std::vector<std::string> _prop_str_vec;
typedef std::vector<void *> _prop_obj_vec;

template <nxs_property Tnp>
struct nxsPropertyType { typedef void *type; };

#else
typedef char * _prop_str;
typedef void * _prop_int_vec;
typedef void * _prop_flt_vec;
typedef void * _prop_str_vec;
typedef void * _prop_obj_vec;
#endif

// Generate the Property typedefs
#ifdef __cplusplus
#define NEXUS_API_PROP(NAME, TYPE, DESC) \
    template <> struct nxsPropertyType<NP_##NAME> { typedef TYPE type; };
#else
#define NEXUS_API_PROP(RETURN_TYPE, NAME, ...)
#endif


#endif
#endif
#endif

/************************************************************************
 * Define API Properties
 ***********************************************************************/

/* THIS SHOULD BE GENERATED FROM THE SCHEMA */

/************************************************************************
 * @def Name
 * @brief Object Name 
 ***********************************************************************/
NEXUS_API_PROP(Name,                  _prop_str,        "Unit Name")
NEXUS_API_PROP(Type,                  _prop_str,        "Unit Type")
NEXUS_API_PROP(Value,                 _prop_int,        "Unit Value")
NEXUS_API_PROP(ID,                    _prop_int,        "Unit ID")
NEXUS_API_PROP(Description,           _prop_str,        "Unit Description")

/* Property Hierarchy */
NEXUS_API_PROP(Count,                 _prop_int,        "Number of Units")
NEXUS_API_PROP(Size,                  _prop_int,        "Number of Sub-Units")
NEXUS_API_PROP(SubUnits,              _prop_str_vec,    "Sub-Unit Vector")
NEXUS_API_PROP(SubUnitType,           _prop_str,        "Sub-Unit Type")

NEXUS_API_PROP(Keys,                  _prop_int_vec,    "Node Keys")

/* Device Properties */
NEXUS_API_PROP(Vendor,                _prop_str,        "Vendor Name")
NEXUS_API_PROP(Architecture,          _prop_str,        "Architecture Designation")
NEXUS_API_PROP(Version,               _prop_str,        "Version String")
NEXUS_API_PROP(MajorVersion,          _prop_int,        "Major Version")
NEXUS_API_PROP(MinorVersion,          _prop_int,        "Minor Version")

NEXUS_API_PROP(CoreSubsystem,         _prop_obj_vec,    "Core Subsystem Hierarchy")
NEXUS_API_PROP(MemorySubsystem,       _prop_obj_vec,    "Memory Subsystem Hierarchy")

NEXUS_API_PROP(Limits,                _prop_obj_vec,    "System limits")
NEXUS_API_PROP(Features,              _prop_str,        "System features")

NEXUS_API_PROP(GlobalMemorySize,      _prop_int,        "Global memory size (bytes)")
NEXUS_API_PROP(CoreMemorySize,        _prop_int,        "Core Memory size (bytes)")
NEXUS_API_PROP(CoreRegisterSize,      _prop_int,        "Core Register size (bytes)")

NEXUS_API_PROP(SIMDSize,              _prop_int,        "SIMD thread count")

NEXUS_API_PROP(CoreClockRate,         _prop_int,        "Core clock rate (MHz)")
NEXUS_API_PROP(MemoryClockRate,       _prop_int,        "Memory clock rate (MHz)")
NEXUS_API_PROP(MemoryBusWidth,        _prop_int,        "Memory bus width (bits)")

/* Kernel Properties */
NEXUS_API_PROP(MaxThreadsPerBlock,    _prop_int,        "Max threads per block")
NEXUS_API_PROP(TimeStamp,             _prop_int,        "Time stamp (cycles)")
NEXUS_API_PROP(ElapsedTime,           _prop_flt,        "Elapsed time (ms)")

/* Threadgroup Properties */
NEXUS_API_PROP(MaxThreadsPerThreadgroup, _prop_int,     "Max threads per threadgroup")
NEXUS_API_PROP(MaxThreadgroupsPerCore,   _prop_int,     "Max threadgroups per Core")
NEXUS_API_PROP(MaxThreadgroupMemorySize, _prop_int,     "Max threadgroup memory size (bytes)")

NEXUS_API_PROP(Location,              _prop_str,        "Location")
NEXUS_API_PROP(MaxTransferRate,       _prop_int,        "Max transfer rate (bytes/sec)")
NEXUS_API_PROP(UnifiedMemory,         _prop_int,        "Unified Memory present")
NEXUS_API_PROP(MaxBufferSize,         _prop_int,        "Max buffer size (bytes)")

NEXUS_API_PROP(DataTypes,             _prop_str_vec,    "Data Types")
NEXUS_API_PROP(ClockModes,            _prop_str_vec,    "Clock Modes")
NEXUS_API_PROP(BaseClock,             _prop_int,        "Base Clock (MHz)")
NEXUS_API_PROP(PowerModes,            _prop_str_vec,    "Power Modes")
NEXUS_API_PROP(MaxPower,              _prop_flt,        "Max Power")

/************************************************************************
 * Cleanup
 ***********************************************************************/
#ifdef NEXUS_API_GENERATE_PROP_ENUM
    NXS_PROPERTY_CNT,
    NXS_PROPERTY_PREFIX_LEN        = 3,

    NXS_PROPERTY_INVALID = -1
}; /* close _nxs_property */

typedef enum _nxs_property nxs_property;

/* Translation functions */
nxs_int nxsGetPropCount();
const char *nxsGetPropName(nxs_int propEnum);
nxs_property nxsGetPropEnum(const char *propName);

const char *nxsGetStatusName(nxs_int statusEnum);
nxs_status nxsGetStatusEnum(const char *statusName);
#else
#if defined(NEXUS_API_GENERATE_PROP_MAP)

}; /* close nxs_property_type_map */

#undef _prop_int
#undef _prop_flt
#undef _prop_str
#undef _prop_int_vec
#undef _prop_flt_vec
#undef _prop_str_vec
#undef _prop_obj_vec

#endif
#endif

/* clang-format off */

#undef NEXUS_API_GENERATE_PROP_ENUM
#undef NEXUS_API_GENERATE_PROP_MAP
#undef NEXUS_API_GENERATE_PROP_TYPE

#undef _NEXUS_API_PROP
#undef NEXUS_API_PROP
