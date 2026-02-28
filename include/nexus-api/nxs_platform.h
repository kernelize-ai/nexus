/*******************************************************************************
 ******************************************************************************/

#ifndef __NXS_RUNTIME_H
#define __NXS_RUNTIME_H

#include <nexus-api/nxs_version.h>

/* clang-format off */

#if defined(_WIN32)
    #if !defined(NXS_API_ENTRY)
        #define NXS_API_ENTRY
    #endif
    #if !defined(NXS_API_CALL)
        #define NXS_API_CALL     __stdcall
    #endif
    #if !defined(NXS_CALLBACK)
        #define NXS_CALLBACK     __stdcall
    #endif
#else
    #if !defined(NXS_API_ENTRY)
        #define NXS_API_ENTRY
    #endif
    #if !defined(NXS_API_CALL)
        #define NXS_API_CALL
    #endif
    #if !defined(NXS_CALLBACK)
        #define NXS_CALLBACK
    #endif
#endif

#ifdef __cplusplus
#define NXS_API_EXTERN_C "C"
#else
#define NXS_API_EXTERN_C
#endif
/*
 * Deprecation flags refer to the last version of the header in which the
 * feature was not deprecated.
 *
 * E.g. VERSION_1_1_DEPRECATED means the feature is present in 1.1 without
 * deprecation but is deprecated in versions later than 1.1.
 */

#ifndef NXS_API_SUFFIX_USER
#define NXS_API_SUFFIX_USER
#endif

#ifndef NXS_API_PREFIX_USER
#define NXS_API_PREFIX_USER
#endif

#define NXS_API_SUFFIX_COMMON NXS_API_SUFFIX_USER
#define NXS_API_PREFIX_COMMON NXS_API_PREFIX_USER

#define NXS_API_SUFFIX__VERSION_1_0 NXS_API_SUFFIX_COMMON
#define NXS_API_SUFFIX__VERSION_1_1 NXS_API_SUFFIX_COMMON
#define NXS_API_SUFFIX__VERSION_1_2 NXS_API_SUFFIX_COMMON
#define NXS_API_SUFFIX__VERSION_2_0 NXS_API_SUFFIX_COMMON
#define NXS_API_SUFFIX__VERSION_2_1 NXS_API_SUFFIX_COMMON
#define NXS_API_SUFFIX__VERSION_2_2 NXS_API_SUFFIX_COMMON
#define NXS_API_SUFFIX__VERSION_3_0 NXS_API_SUFFIX_COMMON
#define NXS_API_SUFFIX__EXPERIMENTAL NXS_API_SUFFIX_COMMON


#ifdef __GNUC__
  #define NXS_API_SUFFIX_DEPRECATED __attribute__((deprecated))
  #define NXS_API_PREFIX_DEPRECATED
#elif defined(_MSC_VER) && !defined(__clang__)
  #define NXS_API_SUFFIX_DEPRECATED
  #define NXS_API_PREFIX_DEPRECATED __declspec(deprecated)
#else
  #define NXS_API_SUFFIX_DEPRECATED
  #define NXS_API_PREFIX_DEPRECATED
#endif

#ifdef NXS_USE_DEPRECATED_NEXUSAPI_1_0_APIS
    #define NXS_API_SUFFIX__VERSION_1_0_DEPRECATED NXS_API_SUFFIX_COMMON
    #define NXS_API_PREFIX__VERSION_1_0_DEPRECATED NXS_API_PREFIX_COMMON
#else
    #define NXS_API_SUFFIX__VERSION_1_0_DEPRECATED NXS_API_SUFFIX_COMMON NXS_API_SUFFIX_DEPRECATED
    #define NXS_API_PREFIX__VERSION_1_0_DEPRECATED NXS_API_PREFIX_COMMON NXS_API_PREFIX_DEPRECATED
#endif

#ifdef NXS_USE_DEPRECATED_NEXUSAPI_1_1_APIS
    #define NXS_API_SUFFIX__VERSION_1_1_DEPRECATED NXS_API_SUFFIX_COMMON
    #define NXS_API_PREFIX__VERSION_1_1_DEPRECATED NXS_API_PREFIX_COMMON
#else
    #define NXS_API_SUFFIX__VERSION_1_1_DEPRECATED NXS_API_SUFFIX_COMMON NXS_API_SUFFIX_DEPRECATED
    #define NXS_API_PREFIX__VERSION_1_1_DEPRECATED NXS_API_PREFIX_COMMON NXS_API_PREFIX_DEPRECATED
#endif

#ifdef NXS_USE_DEPRECATED_NEXUSAPI_1_2_APIS
    #define NXS_API_SUFFIX__VERSION_1_2_DEPRECATED NXS_API_SUFFIX_COMMON
    #define NXS_API_PREFIX__VERSION_1_2_DEPRECATED NXS_API_PREFIX_COMMON
#else
    #define NXS_API_SUFFIX__VERSION_1_2_DEPRECATED NXS_API_SUFFIX_COMMON NXS_API_SUFFIX_DEPRECATED
    #define NXS_API_PREFIX__VERSION_1_2_DEPRECATED NXS_API_PREFIX_COMMON NXS_API_PREFIX_DEPRECATED
 #endif

#ifdef NXS_USE_DEPRECATED_NEXUSAPI_2_0_APIS
    #define NXS_API_SUFFIX__VERSION_2_0_DEPRECATED NXS_API_SUFFIX_COMMON
    #define NXS_API_PREFIX__VERSION_2_0_DEPRECATED NXS_API_PREFIX_COMMON
#else
    #define NXS_API_SUFFIX__VERSION_2_0_DEPRECATED NXS_API_SUFFIX_COMMON NXS_API_SUFFIX_DEPRECATED
    #define NXS_API_PREFIX__VERSION_2_0_DEPRECATED NXS_API_PREFIX_COMMON NXS_API_PREFIX_DEPRECATED
#endif

#ifdef NXS_USE_DEPRECATED_NEXUSAPI_2_1_APIS
    #define NXS_API_SUFFIX__VERSION_2_1_DEPRECATED NXS_API_SUFFIX_COMMON
    #define NXS_API_PREFIX__VERSION_2_1_DEPRECATED NXS_API_PREFIX_COMMON
#else
    #define NXS_API_SUFFIX__VERSION_2_1_DEPRECATED NXS_API_SUFFIX_COMMON NXS_API_SUFFIX_DEPRECATED
    #define NXS_API_PREFIX__VERSION_2_1_DEPRECATED NXS_API_PREFIX_COMMON NXS_API_PREFIX_DEPRECATED
#endif

#ifdef NXS_USE_DEPRECATED_NEXUSAPI_2_2_APIS
    #define NXS_API_SUFFIX__VERSION_2_2_DEPRECATED NXS_API_SUFFIX_COMMON
    #define NXS_API_PREFIX__VERSION_2_2_DEPRECATED NXS_API_PREFIX_COMMON
#else
    #define NXS_API_SUFFIX__VERSION_2_2_DEPRECATED NXS_API_SUFFIX_COMMON NXS_API_SUFFIX_DEPRECATED
    #define NXS_API_PREFIX__VERSION_2_2_DEPRECATED NXS_API_PREFIX_COMMON NXS_API_PREFIX_DEPRECATED
#endif

#define NXS_MAX_DIMS 8

#if (defined (_WIN32) && defined(_MSC_VER))

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
#endif

/* intptr_t is used in cl.h and provided by stddef.h in Visual C++, but not in clang */
/* stdint.h was missing before Visual Studio 2010, include it for later versions and for clang */
#if defined(__clang__) || _MSC_VER >= 1600
    #include <stdint.h>
#endif

/* scalar types  */
typedef signed   __int8         nxs_char;
typedef unsigned __int8         nxs_uchar;
typedef signed   __int16        nxs_short;
typedef unsigned __int16        nxs_ushort;
typedef signed   __int32        nxs_int;
typedef unsigned __int32        nxs_uint;
typedef signed   __int64        nxs_long;
typedef unsigned __int64        nxs_ulong;

typedef unsigned __int16        nxs_half;
typedef float                   nxs_float;
typedef double                  nxs_double;

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

/* Macro names and corresponding values defined by OpenCL */
#define NXS_CHAR_BIT         8
#define NXS_SCHAR_MAX        127
#define NXS_SCHAR_MIN        (-127-1)
#define NXS_CHAR_MAX         NXS_SCHAR_MAX
#define NXS_CHAR_MIN         NXS_SCHAR_MIN
#define NXS_UCHAR_MAX        255
#define NXS_SHRT_MAX         32767
#define NXS_SHRT_MIN         (-32767-1)
#define NXS_USHRT_MAX        65535
#define NXS_INT_MAX          2147483647
#define NXS_INT_MIN          (-2147483647-1)
#define NXS_UINT_MAX         0xffffffffU
#define NXS_LONG_MAX         ((nxs_long) 0x7FFFFFFFFFFFFFFFLL)
#define NXS_LONG_MIN         ((nxs_long) -0x7FFFFFFFFFFFFFFFLL - 1LL)
#define NXS_ULONG_MAX        ((nxs_ulong) 0xFFFFFFFFFFFFFFFFULL)

#define NXS_FLT_DIG          6
#define NXS_FLT_MANT_DIG     24
#define NXS_FLT_MAX_10_EXP   +38
#define NXS_FLT_MAX_EXP      +128
#define NXS_FLT_MIN_10_EXP   -37
#define NXS_FLT_MIN_EXP      -125
#define NXS_FLT_RADIX        2
#define NXS_FLT_MAX          340282346638528859811704183484516925440.0f
#define NXS_FLT_MIN          1.175494350822287507969e-38f
#define NXS_FLT_EPSILON      1.1920928955078125e-7f

#define NXS_HALF_DIG          3
#define NXS_HALF_MANT_DIG     11
#define NXS_HALF_MAX_10_EXP   +4
#define NXS_HALF_MAX_EXP      +16
#define NXS_HALF_MIN_10_EXP   -4
#define NXS_HALF_MIN_EXP      -13
#define NXS_HALF_RADIX        2
#define NXS_HALF_MAX          65504.0f
#define NXS_HALF_MIN          6.103515625e-05f
#define NXS_HALF_EPSILON      9.765625e-04f

#define NXS_DBL_DIG          15
#define NXS_DBL_MANT_DIG     53
#define NXS_DBL_MAX_10_EXP   +308
#define NXS_DBL_MAX_EXP      +1024
#define NXS_DBL_MIN_10_EXP   -307
#define NXS_DBL_MIN_EXP      -1021
#define NXS_DBL_RADIX        2
#define NXS_DBL_MAX          1.7976931348623158e+308
#define NXS_DBL_MIN          2.225073858507201383090e-308
#define NXS_DBL_EPSILON      2.220446049250313080847e-16

#define NXS_M_E              2.7182818284590452354
#define NXS_M_LOG2E          1.4426950408889634074
#define NXS_M_LOG10E         0.43429448190325182765
#define NXS_M_LN2            0.69314718055994530942
#define NXS_M_LN10           2.30258509299404568402
#define NXS_M_PI             3.14159265358979323846
#define NXS_M_PI_2           1.57079632679489661923
#define NXS_M_PI_4           0.78539816339744830962
#define NXS_M_1_PI           0.31830988618379067154
#define NXS_M_2_PI           0.63661977236758134308
#define NXS_M_2_SQRTPI       1.12837916709551257390
#define NXS_M_SQRT2          1.41421356237309504880
#define NXS_M_SQRT1_2        0.70710678118654752440

#define NXS_M_E_F            2.718281828f
#define NXS_M_LOG2E_F        1.442695041f
#define NXS_M_LOG10E_F       0.434294482f
#define NXS_M_LN2_F          0.693147181f
#define NXS_M_LN10_F         2.302585093f
#define NXS_M_PI_F           3.141592654f
#define NXS_M_PI_2_F         1.570796327f
#define NXS_M_PI_4_F         0.785398163f
#define NXS_M_1_PI_F         0.318309886f
#define NXS_M_2_PI_F         0.636619772f
#define NXS_M_2_SQRTPI_F     1.128379167f
#define NXS_M_SQRT2_F        1.414213562f
#define NXS_M_SQRT1_2_F      0.707106781f

#define NXS_NAN              (NXS_INFINITY - NXS_INFINITY)
#define NXS_HUGE_VALF        ((nxs_float) 1e50)
#define NXS_HUGE_VAL         ((nxs_double) 1e500)
#define NXS_MAXFLOAT         NXS_FLT_MAX
#define NXS_INFINITY         NXS_HUGE_VALF

#else

#include <stdint.h>

/* scalar types  */
typedef int8_t          nxs_char;
typedef uint8_t         nxs_uchar;
typedef int16_t         nxs_short;
typedef uint16_t        nxs_ushort;
typedef int32_t         nxs_int;
typedef uint32_t        nxs_uint;
typedef int64_t         nxs_long;
typedef uint64_t        nxs_ulong;

typedef uint16_t        nxs_half;
typedef float           nxs_float;
typedef double          nxs_double;

struct nxs_dim3 {
    nxs_uint x,y,z;
};

struct nxs_shape {
    nxs_ulong dims[NXS_MAX_DIMS];
    nxs_uint rank;
};

/* Macro names and corresponding values defined by OpenCL */
#define NXS_CHAR_BIT         8
#define NXS_SCHAR_MAX        127
#define NXS_SCHAR_MIN        (-127-1)
#define NXS_CHAR_MAX         NXS_SCHAR_MAX
#define NXS_CHAR_MIN         NXS_SCHAR_MIN
#define NXS_UCHAR_MAX        255
#define NXS_SHRT_MAX         32767
#define NXS_SHRT_MIN         (-32767-1)
#define NXS_USHRT_MAX        65535
#define NXS_INT_MAX          2147483647
#define NXS_INT_MIN          (-2147483647-1)
#define NXS_UINT_MAX         0xffffffffU
#define NXS_LONG_MAX         ((nxs_long) 0x7FFFFFFFFFFFFFFFLL)
#define NXS_LONG_MIN         ((nxs_long) -0x7FFFFFFFFFFFFFFFLL - 1LL)
#define NXS_ULONG_MAX        ((nxs_ulong) 0xFFFFFFFFFFFFFFFFULL)

#define NXS_FLT_DIG          6
#define NXS_FLT_MANT_DIG     24
#define NXS_FLT_MAX_10_EXP   +38
#define NXS_FLT_MAX_EXP      +128
#define NXS_FLT_MIN_10_EXP   -37
#define NXS_FLT_MIN_EXP      -125
#define NXS_FLT_RADIX        2
#define NXS_FLT_MAX          340282346638528859811704183484516925440.0f
#define NXS_FLT_MIN          1.175494350822287507969e-38f
#define NXS_FLT_EPSILON      1.1920928955078125e-7f

#define NXS_HALF_DIG          3
#define NXS_HALF_MANT_DIG     11
#define NXS_HALF_MAX_10_EXP   +4
#define NXS_HALF_MAX_EXP      +16
#define NXS_HALF_MIN_10_EXP   -4
#define NXS_HALF_MIN_EXP      -13
#define NXS_HALF_RADIX        2
#define NXS_HALF_MAX          65504.0f
#define NXS_HALF_MIN          6.103515625e-05f
#define NXS_HALF_EPSILON      9.765625e-04f

#define NXS_DBL_DIG          15
#define NXS_DBL_MANT_DIG     53
#define NXS_DBL_MAX_10_EXP   +308
#define NXS_DBL_MAX_EXP      +1024
#define NXS_DBL_MIN_10_EXP   -307
#define NXS_DBL_MIN_EXP      -1021
#define NXS_DBL_RADIX        2
#define NXS_DBL_MAX          179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.0
#define NXS_DBL_MIN          2.225073858507201383090e-308
#define NXS_DBL_EPSILON      2.220446049250313080847e-16

#define NXS_M_E              2.7182818284590452354
#define NXS_M_LOG2E          1.4426950408889634074
#define NXS_M_LOG10E         0.43429448190325182765
#define NXS_M_LN2            0.69314718055994530942
#define NXS_M_LN10           2.30258509299404568402
#define NXS_M_PI             3.14159265358979323846
#define NXS_M_PI_2           1.57079632679489661923
#define NXS_M_PI_4           0.78539816339744830962
#define NXS_M_1_PI           0.31830988618379067154
#define NXS_M_2_PI           0.63661977236758134308
#define NXS_M_2_SQRTPI       1.12837916709551257390
#define NXS_M_SQRT2          1.41421356237309504880
#define NXS_M_SQRT1_2        0.70710678118654752440

#define NXS_M_E_F            2.718281828f
#define NXS_M_LOG2E_F        1.442695041f
#define NXS_M_LOG10E_F       0.434294482f
#define NXS_M_LN2_F          0.693147181f
#define NXS_M_LN10_F         2.302585093f
#define NXS_M_PI_F           3.141592654f
#define NXS_M_PI_2_F         1.570796327f
#define NXS_M_PI_4_F         0.785398163f
#define NXS_M_1_PI_F         0.318309886f
#define NXS_M_2_PI_F         0.636619772f
#define NXS_M_2_SQRTPI_F     1.128379167f
#define NXS_M_SQRT2_F        1.414213562f
#define NXS_M_SQRT1_2_F      0.707106781f

#if defined( __GNUC__ )
   #define NXS_HUGE_VALF     __builtin_huge_valf()
   #define NXS_HUGE_VAL      __builtin_huge_val()
   #define NXS_NAN           __builtin_nanf( "" )
#else
   #define NXS_HUGE_VALF     ((nxs_float) 1e50)
   #define NXS_HUGE_VAL      ((nxs_double) 1e500)
   float nanf( const char * );
   #define NXS_NAN           nanf( "" )
#endif
#define NXS_MAXFLOAT         NXS_FLT_MAX
#define NXS_INFINITY         NXS_HUGE_VALF

#endif

#include <stddef.h>

/*
 * Vector types
 *
 *  Note:   OpenCL requires that all types be naturally aligned.
 *          This means that vector types must be naturally aligned.
 *          For example, a vector of four floats must be aligned to
 *          a 16 byte boundary (calculated as 4 * the natural 4-byte
 *          alignment of the float).  The alignment qualifiers here
 *          will only function properly if your compiler supports them
 *          and if you don't actively work to defeat them.  For example,
 *          in order for a nxs_float4 to be 16 byte aligned in a struct,
 *          the start of the struct must itself be 16-byte aligned.
 *
 *          Maintaining proper alignment is the user's responsibility.
 */

/* Define basic vector types */
#if defined( __VEC__ )
  #if !defined(__clang__)
     #include <altivec.h>   /* may be omitted depending on compiler. AltiVec spec provides no way to detect whether the header is required. */
  #endif
   typedef __vector unsigned char     __nxs_uchar16;
   typedef __vector signed char       __nxs_char16;
   typedef __vector unsigned short    __nxs_ushort8;
   typedef __vector signed short      __nxs_short8;
   typedef __vector unsigned int      __nxs_uint4;
   typedef __vector signed int        __nxs_int4;
   typedef __vector float             __nxs_float4;
   #define  __NXS_UCHAR16__  1
   #define  __NXS_CHAR16__   1
   #define  __NXS_USHORT8__  1
   #define  __NXS_SHORT8__   1
   #define  __NXS_UINT4__    1
   #define  __NXS_INT4__     1
   #define  __NXS_FLOAT4__   1
#endif

#if defined( __SSE__ )
    #if defined( __MINGW64__ )
        #include <intrin.h>
    #else
        #include <xmmintrin.h>
    #endif
    #if defined( __GNUC__ )
        typedef float __nxs_float4   __attribute__((vector_size(16)));
    #else
        typedef __m128 __nxs_float4;
    #endif
    #define __NXS_FLOAT4__   1
#endif

#if defined( __SSE2__ )
    #if defined( __MINGW64__ )
        #include <intrin.h>
    #else
        #include <emmintrin.h>
    #endif
    #if defined( __GNUC__ )
        typedef nxs_uchar    __nxs_uchar16    __attribute__((vector_size(16)));
        typedef nxs_char     __nxs_char16     __attribute__((vector_size(16)));
        typedef nxs_ushort   __nxs_ushort8    __attribute__((vector_size(16)));
        typedef nxs_short    __nxs_short8     __attribute__((vector_size(16)));
        typedef nxs_uint     __nxs_uint4      __attribute__((vector_size(16)));
        typedef nxs_int      __nxs_int4       __attribute__((vector_size(16)));
        typedef nxs_ulong    __nxs_ulong2     __attribute__((vector_size(16)));
        typedef nxs_long     __nxs_long2      __attribute__((vector_size(16)));
        typedef nxs_double   __nxs_double2    __attribute__((vector_size(16)));
    #else
        typedef __m128i __nxs_uchar16;
        typedef __m128i __nxs_char16;
        typedef __m128i __nxs_ushort8;
        typedef __m128i __nxs_short8;
        typedef __m128i __nxs_uint4;
        typedef __m128i __nxs_int4;
        typedef __m128i __nxs_ulong2;
        typedef __m128i __nxs_long2;
        typedef __m128d __nxs_double2;
    #endif
    #define __NXS_UCHAR16__  1
    #define __NXS_CHAR16__   1
    #define __NXS_USHORT8__  1
    #define __NXS_SHORT8__   1
    #define __NXS_INT4__     1
    #define __NXS_UINT4__    1
    #define __NXS_ULONG2__   1
    #define __NXS_LONG2__    1
    #define __NXS_DOUBLE2__  1
#endif

#if defined( __MMX__ )
    #include <mmintrin.h>
    #if defined( __GNUC__ )
        typedef nxs_uchar    __nxs_uchar8     __attribute__((vector_size(8)));
        typedef nxs_char     __nxs_char8      __attribute__((vector_size(8)));
        typedef nxs_ushort   __nxs_ushort4    __attribute__((vector_size(8)));
        typedef nxs_short    __nxs_short4     __attribute__((vector_size(8)));
        typedef nxs_uint     __nxs_uint2      __attribute__((vector_size(8)));
        typedef nxs_int      __nxs_int2       __attribute__((vector_size(8)));
        typedef nxs_ulong    __nxs_ulong1     __attribute__((vector_size(8)));
        typedef nxs_long     __nxs_long1      __attribute__((vector_size(8)));
        typedef nxs_float    __nxs_float2     __attribute__((vector_size(8)));
    #else
        typedef __m64       __nxs_uchar8;
        typedef __m64       __nxs_char8;
        typedef __m64       __nxs_ushort4;
        typedef __m64       __nxs_short4;
        typedef __m64       __nxs_uint2;
        typedef __m64       __nxs_int2;
        typedef __m64       __nxs_ulong1;
        typedef __m64       __nxs_long1;
        typedef __m64       __nxs_float2;
    #endif
    #define __NXS_UCHAR8__   1
    #define __NXS_CHAR8__    1
    #define __NXS_USHORT4__  1
    #define __NXS_SHORT4__   1
    #define __NXS_INT2__     1
    #define __NXS_UINT2__    1
    #define __NXS_ULONG1__   1
    #define __NXS_LONG1__    1
    #define __NXS_FLOAT2__   1
#endif

#if defined( __AVX__ )
    #if defined( __MINGW64__ )
        #include <intrin.h>
    #else
        #include <immintrin.h>
    #endif
    #if defined( __GNUC__ )
        typedef nxs_float    __nxs_float8     __attribute__((vector_size(32)));
        typedef nxs_double   __nxs_double4    __attribute__((vector_size(32)));
    #else
        typedef __m256      __nxs_float8;
        typedef __m256d     __nxs_double4;
    #endif
    #define __NXS_FLOAT8__   1
    #define __NXS_DOUBLE4__  1
#endif

/* Define capabilities for anonymous struct members. */
#if !defined(__cplusplus) && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define  __NXS_HAS_ANON_STRUCT__ 1
#define  __NXS_ANON_STRUCT__
#elif defined(_WIN32) && defined(_MSC_VER) && !defined(__STDC__)
#define  __NXS_HAS_ANON_STRUCT__ 1
#define  __NXS_ANON_STRUCT__
#elif defined(__GNUC__) && ! defined(__STRICT_ANSI__)
#define  __NXS_HAS_ANON_STRUCT__ 1
#define  __NXS_ANON_STRUCT__ __extension__
#elif defined(__clang__)
#define  __NXS_HAS_ANON_STRUCT__ 1
#define  __NXS_ANON_STRUCT__ __extension__
#else
#define  __NXS_HAS_ANON_STRUCT__ 0
#define  __NXS_ANON_STRUCT__
#endif

#if defined(_WIN32) && defined(_MSC_VER) && __NXS_HAS_ANON_STRUCT__
   /* Disable warning C4201: nonstandard extension used : nameless struct/union */
    #pragma warning( push )
    #pragma warning( disable : 4201 )
#endif

/* Define alignment keys */
#if defined( __GNUC__ ) || defined(__INTEGRITY)
    #define NXS_ALIGNED(_x)          __attribute__ ((aligned(_x)))
#elif defined( _WIN32) && (_MSC_VER)
    /* Alignment keys neutered on windows because MSVC can't swallow function arguments with alignment requirements     */
    /* http://msdn.microsoft.com/en-us/library/373ak2y1%28VS.71%29.aspx                                                 */
    /* #include <crtdefs.h>                                                                                             */
    /* #define NXS_ALIGNED(_x)          _CRT_ALIGN(_x)                                                                   */
    #define NXS_ALIGNED(_x)
#else
   #warning  Need to implement some method to align data here
   #define  NXS_ALIGNED(_x)
#endif

/* Indicate whether .xyzw, .s0123 and .hi.lo are supported */
#if __NXS_HAS_ANON_STRUCT__
    /* .xyzw and .s0123...{f|F} are supported */
    #define NXS_HAS_NAMED_VECTOR_FIELDS 1
    /* .hi and .lo are supported */
    #define NXS_HAS_HI_LO_VECTOR_FIELDS 1
#endif

/* Define nxs_vector types */

/* ---- nxs_charn ---- */
typedef union
{
    nxs_char  NXS_ALIGNED(2) s[2];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_char  x, y; };
   __NXS_ANON_STRUCT__ struct{ nxs_char  s0, s1; };
   __NXS_ANON_STRUCT__ struct{ nxs_char  lo, hi; };
#endif
#if defined( __NXS_CHAR2__)
    __nxs_char2     v2;
#endif
}nxs_char2;

typedef union
{
    nxs_char  NXS_ALIGNED(4) s[4];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_char  x, y, z, w; };
   __NXS_ANON_STRUCT__ struct{ nxs_char  s0, s1, s2, s3; };
   __NXS_ANON_STRUCT__ struct{ nxs_char2 lo, hi; };
#endif
#if defined( __NXS_CHAR2__)
    __nxs_char2     v2[2];
#endif
#if defined( __NXS_CHAR4__)
    __nxs_char4     v4;
#endif
}nxs_char4;

/* nxs_char3 is identical in size, alignment and behavior to nxs_char4. See section 6.1.5. */
typedef  nxs_char4  nxs_char3;

typedef union
{
    nxs_char   NXS_ALIGNED(8) s[8];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_char  x, y, z, w; };
   __NXS_ANON_STRUCT__ struct{ nxs_char  s0, s1, s2, s3, s4, s5, s6, s7; };
   __NXS_ANON_STRUCT__ struct{ nxs_char4 lo, hi; };
#endif
#if defined( __NXS_CHAR2__)
    __nxs_char2     v2[4];
#endif
#if defined( __NXS_CHAR4__)
    __nxs_char4     v4[2];
#endif
#if defined( __NXS_CHAR8__ )
    __nxs_char8     v8;
#endif
}nxs_char8;

typedef union
{
    nxs_char  NXS_ALIGNED(16) s[16];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_char  x, y, z, w, __spacer4, __spacer5, __spacer6, __spacer7, __spacer8, __spacer9, sa, sb, sc, sd, se, sf; };
   __NXS_ANON_STRUCT__ struct{ nxs_char  s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, sA, sB, sC, sD, sE, sF; };
   __NXS_ANON_STRUCT__ struct{ nxs_char8 lo, hi; };
#endif
#if defined( __NXS_CHAR2__)
    __nxs_char2     v2[8];
#endif
#if defined( __NXS_CHAR4__)
    __nxs_char4     v4[4];
#endif
#if defined( __NXS_CHAR8__ )
    __nxs_char8     v8[2];
#endif
#if defined( __NXS_CHAR16__ )
    __nxs_char16    v16;
#endif
}nxs_char16;


/* ---- nxs_ucharn ---- */
typedef union
{
    nxs_uchar  NXS_ALIGNED(2) s[2];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_uchar  x, y; };
   __NXS_ANON_STRUCT__ struct{ nxs_uchar  s0, s1; };
   __NXS_ANON_STRUCT__ struct{ nxs_uchar  lo, hi; };
#endif
#if defined( __nxs_uchar2__)
    __nxs_uchar2     v2;
#endif
}nxs_uchar2;

typedef union
{
    nxs_uchar  NXS_ALIGNED(4) s[4];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_uchar  x, y, z, w; };
   __NXS_ANON_STRUCT__ struct{ nxs_uchar  s0, s1, s2, s3; };
   __NXS_ANON_STRUCT__ struct{ nxs_uchar2 lo, hi; };
#endif
#if defined( __NXS_UCHAR2__)
    __nxs_uchar2     v2[2];
#endif
#if defined( __NXS_UCHAR4__)
    __nxs_uchar4     v4;
#endif
}nxs_uchar4;

/* nxs_uchar3 is identical in size, alignment and behavior to nxs_uchar4. See section 6.1.5. */
typedef  nxs_uchar4  nxs_uchar3;

typedef union
{
    nxs_uchar   NXS_ALIGNED(8) s[8];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_uchar  x, y, z, w; };
   __NXS_ANON_STRUCT__ struct{ nxs_uchar  s0, s1, s2, s3, s4, s5, s6, s7; };
   __NXS_ANON_STRUCT__ struct{ nxs_uchar4 lo, hi; };
#endif
#if defined( __NXS_UCHAR2__)
    __nxs_uchar2     v2[4];
#endif
#if defined( __NXS_UCHAR4__)
    __nxs_uchar4     v4[2];
#endif
#if defined( __NXS_UCHAR8__ )
    __nxs_uchar8     v8;
#endif
}nxs_uchar8;

typedef union
{
    nxs_uchar  NXS_ALIGNED(16) s[16];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_uchar  x, y, z, w, __spacer4, __spacer5, __spacer6, __spacer7, __spacer8, __spacer9, sa, sb, sc, sd, se, sf; };
   __NXS_ANON_STRUCT__ struct{ nxs_uchar  s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, sA, sB, sC, sD, sE, sF; };
   __NXS_ANON_STRUCT__ struct{ nxs_uchar8 lo, hi; };
#endif
#if defined( __NXS_UCHAR2__)
    __nxs_uchar2     v2[8];
#endif
#if defined( __NXS_UCHAR4__)
    __nxs_uchar4     v4[4];
#endif
#if defined( __NXS_UCHAR8__ )
    __nxs_uchar8     v8[2];
#endif
#if defined( __NXS_UCHAR16__ )
    __nxs_uchar16    v16;
#endif
}nxs_uchar16;


/* ---- nxs_shortn ---- */
typedef union
{
    nxs_short  NXS_ALIGNED(4) s[2];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_short  x, y; };
   __NXS_ANON_STRUCT__ struct{ nxs_short  s0, s1; };
   __NXS_ANON_STRUCT__ struct{ nxs_short  lo, hi; };
#endif
#if defined( __NXS_SHORT2__)
    __nxs_short2     v2;
#endif
}nxs_short2;

typedef union
{
    nxs_short  NXS_ALIGNED(8) s[4];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_short  x, y, z, w; };
   __NXS_ANON_STRUCT__ struct{ nxs_short  s0, s1, s2, s3; };
   __NXS_ANON_STRUCT__ struct{ nxs_short2 lo, hi; };
#endif
#if defined( __NXS_SHORT2__)
    __nxs_short2     v2[2];
#endif
#if defined( __NXS_SHORT4__)
    __nxs_short4     v4;
#endif
}nxs_short4;

/* nxs_short3 is identical in size, alignment and behavior to nxs_short4. See section 6.1.5. */
typedef  nxs_short4  nxs_short3;

typedef union
{
    nxs_short   NXS_ALIGNED(16) s[8];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_short  x, y, z, w; };
   __NXS_ANON_STRUCT__ struct{ nxs_short  s0, s1, s2, s3, s4, s5, s6, s7; };
   __NXS_ANON_STRUCT__ struct{ nxs_short4 lo, hi; };
#endif
#if defined( __NXS_SHORT2__)
    __nxs_short2     v2[4];
#endif
#if defined( __NXS_SHORT4__)
    __nxs_short4     v4[2];
#endif
#if defined( __NXS_SHORT8__ )
    __nxs_short8     v8;
#endif
}nxs_short8;

typedef union
{
    nxs_short  NXS_ALIGNED(32) s[16];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_short  x, y, z, w, __spacer4, __spacer5, __spacer6, __spacer7, __spacer8, __spacer9, sa, sb, sc, sd, se, sf; };
   __NXS_ANON_STRUCT__ struct{ nxs_short  s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, sA, sB, sC, sD, sE, sF; };
   __NXS_ANON_STRUCT__ struct{ nxs_short8 lo, hi; };
#endif
#if defined( __NXS_SHORT2__)
    __nxs_short2     v2[8];
#endif
#if defined( __NXS_SHORT4__)
    __nxs_short4     v4[4];
#endif
#if defined( __NXS_SHORT8__ )
    __nxs_short8     v8[2];
#endif
#if defined( __NXS_SHORT16__ )
    __nxs_short16    v16;
#endif
}nxs_short16;


/* ---- nxs_ushortn ---- */
typedef union
{
    nxs_ushort  NXS_ALIGNED(4) s[2];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_ushort  x, y; };
   __NXS_ANON_STRUCT__ struct{ nxs_ushort  s0, s1; };
   __NXS_ANON_STRUCT__ struct{ nxs_ushort  lo, hi; };
#endif
#if defined( __NXS_USHORT2__)
    __nxs_ushort2     v2;
#endif
}nxs_ushort2;

typedef union
{
    nxs_ushort  NXS_ALIGNED(8) s[4];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_ushort  x, y, z, w; };
   __NXS_ANON_STRUCT__ struct{ nxs_ushort  s0, s1, s2, s3; };
   __NXS_ANON_STRUCT__ struct{ nxs_ushort2 lo, hi; };
#endif
#if defined( __NXS_USHORT2__)
    __nxs_ushort2     v2[2];
#endif
#if defined( __NXS_USHORT4__)
    __nxs_ushort4     v4;
#endif
}nxs_ushort4;

/* nxs_ushort3 is identical in size, alignment and behavior to nxs_ushort4. See section 6.1.5. */
typedef  nxs_ushort4  nxs_ushort3;

typedef union
{
    nxs_ushort   NXS_ALIGNED(16) s[8];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_ushort  x, y, z, w; };
   __NXS_ANON_STRUCT__ struct{ nxs_ushort  s0, s1, s2, s3, s4, s5, s6, s7; };
   __NXS_ANON_STRUCT__ struct{ nxs_ushort4 lo, hi; };
#endif
#if defined( __NXS_USHORT2__)
    __nxs_ushort2     v2[4];
#endif
#if defined( __NXS_USHORT4__)
    __nxs_ushort4     v4[2];
#endif
#if defined( __NXS_USHORT8__ )
    __nxs_ushort8     v8;
#endif
}nxs_ushort8;

typedef union
{
    nxs_ushort  NXS_ALIGNED(32) s[16];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_ushort  x, y, z, w, __spacer4, __spacer5, __spacer6, __spacer7, __spacer8, __spacer9, sa, sb, sc, sd, se, sf; };
   __NXS_ANON_STRUCT__ struct{ nxs_ushort  s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, sA, sB, sC, sD, sE, sF; };
   __NXS_ANON_STRUCT__ struct{ nxs_ushort8 lo, hi; };
#endif
#if defined( __NXS_USHORT2__)
    __nxs_ushort2     v2[8];
#endif
#if defined( __NXS_USHORT4__)
    __nxs_ushort4     v4[4];
#endif
#if defined( __NXS_USHORT8__ )
    __nxs_ushort8     v8[2];
#endif
#if defined( __NXS_USHORT16__ )
    __nxs_ushort16    v16;
#endif
}nxs_ushort16;


/* ---- nxs_halfn ---- */
typedef union
{
    nxs_half  NXS_ALIGNED(4) s[2];
#if __NXS_HAS_ANON_STRUCT__
    __NXS_ANON_STRUCT__ struct{ nxs_half  x, y; };
    __NXS_ANON_STRUCT__ struct{ nxs_half  s0, s1; };
    __NXS_ANON_STRUCT__ struct{ nxs_half  lo, hi; };
#endif
#if defined( __NXS_HALF2__)
    __nxs_half2     v2;
#endif
}nxs_half2;

typedef union
{
    nxs_half  NXS_ALIGNED(8) s[4];
#if __NXS_HAS_ANON_STRUCT__
    __NXS_ANON_STRUCT__ struct{ nxs_half  x, y, z, w; };
    __NXS_ANON_STRUCT__ struct{ nxs_half  s0, s1, s2, s3; };
    __NXS_ANON_STRUCT__ struct{ nxs_half2 lo, hi; };
#endif
#if defined( __NXS_HALF2__)
    __nxs_half2     v2[2];
#endif
#if defined( __NXS_HALF4__)
    __nxs_half4     v4;
#endif
}nxs_half4;

/* nxs_half3 is identical in size, alignment and behavior to nxs_half4. See section 6.1.5. */
typedef  nxs_half4  nxs_half3;

typedef union
{
    nxs_half   NXS_ALIGNED(16) s[8];
#if __NXS_HAS_ANON_STRUCT__
    __NXS_ANON_STRUCT__ struct{ nxs_half  x, y, z, w; };
    __NXS_ANON_STRUCT__ struct{ nxs_half  s0, s1, s2, s3, s4, s5, s6, s7; };
    __NXS_ANON_STRUCT__ struct{ nxs_half4 lo, hi; };
#endif
#if defined( __NXS_HALF2__)
    __nxs_half2     v2[4];
#endif
#if defined( __NXS_HALF4__)
    __nxs_half4     v4[2];
#endif
#if defined( __NXS_HALF8__ )
    __nxs_half8     v8;
#endif
}nxs_half8;

typedef union
{
    nxs_half  NXS_ALIGNED(32) s[16];
#if __NXS_HAS_ANON_STRUCT__
    __NXS_ANON_STRUCT__ struct{ nxs_half  x, y, z, w, __spacer4, __spacer5, __spacer6, __spacer7, __spacer8, __spacer9, sa, sb, sc, sd, se, sf; };
    __NXS_ANON_STRUCT__ struct{ nxs_half  s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, sA, sB, sC, sD, sE, sF; };
    __NXS_ANON_STRUCT__ struct{ nxs_half8 lo, hi; };
#endif
#if defined( __NXS_HALF2__)
    __nxs_half2     v2[8];
#endif
#if defined( __NXS_HALF4__)
    __nxs_half4     v4[4];
#endif
#if defined( __NXS_HALF8__ )
    __nxs_half8     v8[2];
#endif
#if defined( __NXS_HALF16__ )
    __nxs_half16    v16;
#endif
}nxs_half16;

/* ---- nxs_intn ---- */
typedef union
{
    nxs_int  NXS_ALIGNED(8) s[2];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_int  x, y; };
   __NXS_ANON_STRUCT__ struct{ nxs_int  s0, s1; };
   __NXS_ANON_STRUCT__ struct{ nxs_int  lo, hi; };
#endif
#if defined( __NXS_INT2__)
    __nxs_int2     v2;
#endif
}nxs_int2;

typedef union
{
    nxs_int  NXS_ALIGNED(16) s[4];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_int  x, y, z, w; };
   __NXS_ANON_STRUCT__ struct{ nxs_int  s0, s1, s2, s3; };
   __NXS_ANON_STRUCT__ struct{ nxs_int2 lo, hi; };
#endif
#if defined( __NXS_INT2__)
    __nxs_int2     v2[2];
#endif
#if defined( __NXS_INT4__)
    __nxs_int4     v4;
#endif
}nxs_int4;

/* nxs_int3 is identical in size, alignment and behavior to nxs_int4. See section 6.1.5. */
typedef  nxs_int4  nxs_int3;

typedef union
{
    nxs_int   NXS_ALIGNED(32) s[8];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_int  x, y, z, w; };
   __NXS_ANON_STRUCT__ struct{ nxs_int  s0, s1, s2, s3, s4, s5, s6, s7; };
   __NXS_ANON_STRUCT__ struct{ nxs_int4 lo, hi; };
#endif
#if defined( __NXS_INT2__)
    __nxs_int2     v2[4];
#endif
#if defined( __NXS_INT4__)
    __nxs_int4     v4[2];
#endif
#if defined( __NXS_INT8__ )
    __nxs_int8     v8;
#endif
}nxs_int8;

typedef union
{
    nxs_int  NXS_ALIGNED(64) s[16];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_int  x, y, z, w, __spacer4, __spacer5, __spacer6, __spacer7, __spacer8, __spacer9, sa, sb, sc, sd, se, sf; };
   __NXS_ANON_STRUCT__ struct{ nxs_int  s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, sA, sB, sC, sD, sE, sF; };
   __NXS_ANON_STRUCT__ struct{ nxs_int8 lo, hi; };
#endif
#if defined( __NXS_INT2__)
    __nxs_int2     v2[8];
#endif
#if defined( __NXS_INT4__)
    __nxs_int4     v4[4];
#endif
#if defined( __NXS_INT8__ )
    __nxs_int8     v8[2];
#endif
#if defined( __NXS_INT16__ )
    __nxs_int16    v16;
#endif
}nxs_int16;


/* ---- nxs_uintn ---- */
typedef union
{
    nxs_uint  NXS_ALIGNED(8) s[2];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_uint  x, y; };
   __NXS_ANON_STRUCT__ struct{ nxs_uint  s0, s1; };
   __NXS_ANON_STRUCT__ struct{ nxs_uint  lo, hi; };
#endif
#if defined( __NXS_UINT2__)
    __nxs_uint2     v2;
#endif
}nxs_uint2;

typedef union
{
    nxs_uint  NXS_ALIGNED(16) s[4];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_uint  x, y, z, w; };
   __NXS_ANON_STRUCT__ struct{ nxs_uint  s0, s1, s2, s3; };
   __NXS_ANON_STRUCT__ struct{ nxs_uint2 lo, hi; };
#endif
#if defined( __NXS_UINT2__)
    __nxs_uint2     v2[2];
#endif
#if defined( __NXS_UINT4__)
    __nxs_uint4     v4;
#endif
}nxs_uint4;

/* nxs_uint3 is identical in size, alignment and behavior to nxs_uint4. See section 6.1.5. */
typedef  nxs_uint4  nxs_uint3;

typedef union
{
    nxs_uint   NXS_ALIGNED(32) s[8];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_uint  x, y, z, w; };
   __NXS_ANON_STRUCT__ struct{ nxs_uint  s0, s1, s2, s3, s4, s5, s6, s7; };
   __NXS_ANON_STRUCT__ struct{ nxs_uint4 lo, hi; };
#endif
#if defined( __NXS_UINT2__)
    __nxs_uint2     v2[4];
#endif
#if defined( __NXS_UINT4__)
    __nxs_uint4     v4[2];
#endif
#if defined( __NXS_UINT8__ )
    __nxs_uint8     v8;
#endif
}nxs_uint8;

typedef union
{
    nxs_uint  NXS_ALIGNED(64) s[16];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_uint  x, y, z, w, __spacer4, __spacer5, __spacer6, __spacer7, __spacer8, __spacer9, sa, sb, sc, sd, se, sf; };
   __NXS_ANON_STRUCT__ struct{ nxs_uint  s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, sA, sB, sC, sD, sE, sF; };
   __NXS_ANON_STRUCT__ struct{ nxs_uint8 lo, hi; };
#endif
#if defined( __NXS_UINT2__)
    __nxs_uint2     v2[8];
#endif
#if defined( __NXS_UINT4__)
    __nxs_uint4     v4[4];
#endif
#if defined( __NXS_UINT8__ )
    __nxs_uint8     v8[2];
#endif
#if defined( __NXS_UINT16__ )
    __nxs_uint16    v16;
#endif
}nxs_uint16;

/* ---- nxs_longn ---- */
typedef union
{
    nxs_long  NXS_ALIGNED(16) s[2];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_long  x, y; };
   __NXS_ANON_STRUCT__ struct{ nxs_long  s0, s1; };
   __NXS_ANON_STRUCT__ struct{ nxs_long  lo, hi; };
#endif
#if defined( __NXS_LONG2__)
    __nxs_long2     v2;
#endif
}nxs_long2;

typedef union
{
    nxs_long  NXS_ALIGNED(32) s[4];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_long  x, y, z, w; };
   __NXS_ANON_STRUCT__ struct{ nxs_long  s0, s1, s2, s3; };
   __NXS_ANON_STRUCT__ struct{ nxs_long2 lo, hi; };
#endif
#if defined( __NXS_LONG2__)
    __nxs_long2     v2[2];
#endif
#if defined( __NXS_LONG4__)
    __nxs_long4     v4;
#endif
}nxs_long4;

/* nxs_long3 is identical in size, alignment and behavior to nxs_long4. See section 6.1.5. */
typedef  nxs_long4  nxs_long3;

typedef union
{
    nxs_long   NXS_ALIGNED(64) s[8];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_long  x, y, z, w; };
   __NXS_ANON_STRUCT__ struct{ nxs_long  s0, s1, s2, s3, s4, s5, s6, s7; };
   __NXS_ANON_STRUCT__ struct{ nxs_long4 lo, hi; };
#endif
#if defined( __NXS_LONG2__)
    __nxs_long2     v2[4];
#endif
#if defined( __NXS_LONG4__)
    __nxs_long4     v4[2];
#endif
#if defined( __NXS_LONG8__ )
    __nxs_long8     v8;
#endif
}nxs_long8;

typedef union
{
    nxs_long  NXS_ALIGNED(128) s[16];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_long  x, y, z, w, __spacer4, __spacer5, __spacer6, __spacer7, __spacer8, __spacer9, sa, sb, sc, sd, se, sf; };
   __NXS_ANON_STRUCT__ struct{ nxs_long  s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, sA, sB, sC, sD, sE, sF; };
   __NXS_ANON_STRUCT__ struct{ nxs_long8 lo, hi; };
#endif
#if defined( __NXS_LONG2__)
    __nxs_long2     v2[8];
#endif
#if defined( __NXS_LONG4__)
    __nxs_long4     v4[4];
#endif
#if defined( __NXS_LONG8__ )
    __nxs_long8     v8[2];
#endif
#if defined( __NXS_LONG16__ )
    __nxs_long16    v16;
#endif
}nxs_long16;


/* ---- nxs_ulongn ---- */
typedef union
{
    nxs_ulong  NXS_ALIGNED(16) s[2];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_ulong  x, y; };
   __NXS_ANON_STRUCT__ struct{ nxs_ulong  s0, s1; };
   __NXS_ANON_STRUCT__ struct{ nxs_ulong  lo, hi; };
#endif
#if defined( __NXS_ULONG2__)
    __nxs_ulong2     v2;
#endif
}nxs_ulong2;

typedef union
{
    nxs_ulong  NXS_ALIGNED(32) s[4];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_ulong  x, y, z, w; };
   __NXS_ANON_STRUCT__ struct{ nxs_ulong  s0, s1, s2, s3; };
   __NXS_ANON_STRUCT__ struct{ nxs_ulong2 lo, hi; };
#endif
#if defined( __NXS_ULONG2__)
    __nxs_ulong2     v2[2];
#endif
#if defined( __NXS_ULONG4__)
    __nxs_ulong4     v4;
#endif
}nxs_ulong4;

/* nxs_ulong3 is identical in size, alignment and behavior to nxs_ulong4. See section 6.1.5. */
typedef  nxs_ulong4  nxs_ulong3;

typedef union
{
    nxs_ulong   NXS_ALIGNED(64) s[8];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_ulong  x, y, z, w; };
   __NXS_ANON_STRUCT__ struct{ nxs_ulong  s0, s1, s2, s3, s4, s5, s6, s7; };
   __NXS_ANON_STRUCT__ struct{ nxs_ulong4 lo, hi; };
#endif
#if defined( __NXS_ULONG2__)
    __nxs_ulong2     v2[4];
#endif
#if defined( __NXS_ULONG4__)
    __nxs_ulong4     v4[2];
#endif
#if defined( __NXS_ULONG8__ )
    __nxs_ulong8     v8;
#endif
}nxs_ulong8;

typedef union
{
    nxs_ulong  NXS_ALIGNED(128) s[16];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_ulong  x, y, z, w, __spacer4, __spacer5, __spacer6, __spacer7, __spacer8, __spacer9, sa, sb, sc, sd, se, sf; };
   __NXS_ANON_STRUCT__ struct{ nxs_ulong  s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, sA, sB, sC, sD, sE, sF; };
   __NXS_ANON_STRUCT__ struct{ nxs_ulong8 lo, hi; };
#endif
#if defined( __NXS_ULONG2__)
    __nxs_ulong2     v2[8];
#endif
#if defined( __NXS_ULONG4__)
    __nxs_ulong4     v4[4];
#endif
#if defined( __NXS_ULONG8__ )
    __nxs_ulong8     v8[2];
#endif
#if defined( __NXS_ULONG16__ )
    __nxs_ulong16    v16;
#endif
}nxs_ulong16;


/* --- nxs_floatn ---- */

typedef union
{
    nxs_float  NXS_ALIGNED(8) s[2];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_float  x, y; };
   __NXS_ANON_STRUCT__ struct{ nxs_float  s0, s1; };
   __NXS_ANON_STRUCT__ struct{ nxs_float  lo, hi; };
#endif
#if defined( __NXS_FLOAT2__)
    __nxs_float2     v2;
#endif
}nxs_float2;

typedef union
{
    nxs_float  NXS_ALIGNED(16) s[4];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_float   x, y, z, w; };
   __NXS_ANON_STRUCT__ struct{ nxs_float   s0, s1, s2, s3; };
   __NXS_ANON_STRUCT__ struct{ nxs_float2  lo, hi; };
#endif
#if defined( __NXS_FLOAT2__)
    __nxs_float2     v2[2];
#endif
#if defined( __NXS_FLOAT4__)
    __nxs_float4     v4;
#endif
}nxs_float4;

/* nxs_float3 is identical in size, alignment and behavior to nxs_float4. See section 6.1.5. */
typedef  nxs_float4  nxs_float3;

typedef union
{
    nxs_float   NXS_ALIGNED(32) s[8];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_float   x, y, z, w; };
   __NXS_ANON_STRUCT__ struct{ nxs_float   s0, s1, s2, s3, s4, s5, s6, s7; };
   __NXS_ANON_STRUCT__ struct{ nxs_float4  lo, hi; };
#endif
#if defined( __NXS_FLOAT2__)
    __nxs_float2     v2[4];
#endif
#if defined( __NXS_FLOAT4__)
    __nxs_float4     v4[2];
#endif
#if defined( __NXS_FLOAT8__ )
    __nxs_float8     v8;
#endif
}nxs_float8;

typedef union
{
    nxs_float  NXS_ALIGNED(64) s[16];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_float  x, y, z, w, __spacer4, __spacer5, __spacer6, __spacer7, __spacer8, __spacer9, sa, sb, sc, sd, se, sf; };
   __NXS_ANON_STRUCT__ struct{ nxs_float  s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, sA, sB, sC, sD, sE, sF; };
   __NXS_ANON_STRUCT__ struct{ nxs_float8 lo, hi; };
#endif
#if defined( __NXS_FLOAT2__)
    __nxs_float2     v2[8];
#endif
#if defined( __NXS_FLOAT4__)
    __nxs_float4     v4[4];
#endif
#if defined( __NXS_FLOAT8__ )
    __nxs_float8     v8[2];
#endif
#if defined( __NXS_FLOAT16__ )
    __nxs_float16    v16;
#endif
}nxs_float16;

/* --- nxs_doublen ---- */

typedef union
{
    nxs_double  NXS_ALIGNED(16) s[2];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_double  x, y; };
   __NXS_ANON_STRUCT__ struct{ nxs_double s0, s1; };
   __NXS_ANON_STRUCT__ struct{ nxs_double lo, hi; };
#endif
#if defined( __NXS_DOUBLE2__)
    __nxs_double2     v2;
#endif
}nxs_double2;

typedef union
{
    nxs_double  NXS_ALIGNED(32) s[4];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_double  x, y, z, w; };
   __NXS_ANON_STRUCT__ struct{ nxs_double  s0, s1, s2, s3; };
   __NXS_ANON_STRUCT__ struct{ nxs_double2 lo, hi; };
#endif
#if defined( __NXS_DOUBLE2__)
    __nxs_double2     v2[2];
#endif
#if defined( __NXS_DOUBLE4__)
    __nxs_double4     v4;
#endif
}nxs_double4;

/* nxs_double3 is identical in size, alignment and behavior to nxs_double4. See section 6.1.5. */
typedef  nxs_double4  nxs_double3;

typedef union
{
    nxs_double   NXS_ALIGNED(64) s[8];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_double  x, y, z, w; };
   __NXS_ANON_STRUCT__ struct{ nxs_double  s0, s1, s2, s3, s4, s5, s6, s7; };
   __NXS_ANON_STRUCT__ struct{ nxs_double4 lo, hi; };
#endif
#if defined( __NXS_DOUBLE2__)
    __nxs_double2     v2[4];
#endif
#if defined( __NXS_DOUBLE4__)
    __nxs_double4     v4[2];
#endif
#if defined( __NXS_DOUBLE8__ )
    __nxs_double8     v8;
#endif
}nxs_double8;

typedef union
{
    nxs_double  NXS_ALIGNED(128) s[16];
#if __NXS_HAS_ANON_STRUCT__
   __NXS_ANON_STRUCT__ struct{ nxs_double  x, y, z, w, __spacer4, __spacer5, __spacer6, __spacer7, __spacer8, __spacer9, sa, sb, sc, sd, se, sf; };
   __NXS_ANON_STRUCT__ struct{ nxs_double  s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, sA, sB, sC, sD, sE, sF; };
   __NXS_ANON_STRUCT__ struct{ nxs_double8 lo, hi; };
#endif
#if defined( __NXS_DOUBLE2__)
    __nxs_double2     v2[8];
#endif
#if defined( __NXS_DOUBLE4__)
    __nxs_double4     v4[4];
#endif
#if defined( __NXS_DOUBLE8__ )
    __nxs_double8     v8[2];
#endif
#if defined( __NXS_DOUBLE16__ )
    __nxs_double16    v16;
#endif
}nxs_double16;

/* Macro to facilitate debugging
 * Usage:
 *   Place NXS_PROGRAM_STRING_DEBUG_INFO on the line before the first line of your source.
 *   The first line ends with:   NXS_PROGRAM_STRING_DEBUG_INFO \"
 *   Each line thereafter of OpenCL C source must end with: \n\
 *   The last line ends in ";
 *
 *   Example:
 *
 *   const char *my_program = NXS_PROGRAM_STRING_DEBUG_INFO "\
 *   kernel void foo( int a, float * b )             \n\
 *   {                                               \n\
 *      // my comment                                \n\
 *      *b[ get_global_id(0)] = a;                   \n\
 *   }                                               \n\
 *   ";
 *
 * This should correctly set up the line, (column) and file information for your source
 * string so you can do source level debugging.
 */
#define  __NXS_STRINGIFY( _x )               # _x
#define  _NXS_STRINGIFY( _x )                __NXS_STRINGIFY( _x )
#define  NXS_PROGRAM_STRING_DEBUG_INFO       "#line "  _NXS_STRINGIFY(__LINE__) " \"" __FILE__ "\" \n\n"

#define _NXS_CONCAT(x,y) x ## y
#define NXS_CONCAT(x,y) _NXS_CONCAT(x,y)

#if defined(_WIN32) && defined(_MSC_VER) && __NXS_HAS_ANON_STRUCT__
    #pragma warning( pop )
#endif

#endif  /* __NXS_RUNTIME_H  */
