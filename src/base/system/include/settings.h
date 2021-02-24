/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform #]
***/

#pragma once

//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <utility>
#include <cwctype>
#include <clocale>
#include <numeric>
#include <unordered_set>

//-----------------------------------------------------------------------------

// System headers
#if defined(_WIN64)

    // Define the more universal macro
    #define PLATFORM_X64
    #define PLATFORM_64BIT
    #define PLATFORM_WINDOWS
    #define PLATFORM_LITTLEENDIAN
    #define PLATFORM_MSVC
    #define PLATFORM_NAME "pc"
    #define PLATFORM_WINAPI

#elif defined(_WIN32)

    // Define the more universal macro
    #define PLATFORM_X86
    #define PLATFORM_WIN32
    #define PLATFORM_32BIT
    #define PLATFORM_WINDOWS
    #define PLATFORM_LITTLEENDIAN
    #define PLATFORM_MSVC
    #define PLATFORM_NAME "pc"
    #define PLATFORM_WINAPI

#elif defined(__linux__)

    #define PLATFORM_NAME "linux"
    #define PLATFORM_LINUX
    #define PLATFORM_POSIX
    #define PLATFORM_LITTLEENDIAN

    #if defined (__clang__)
        #define PLATFORM_CLANG
    #elif defined(__GNUC__)
        #define PLATFORM_GCC
    #else
        #error "Unrecognized compiler (not a gcc or clang)"
    #endif

    #define _MSC_FULL_VER 0

    #define strcpy_s strcpy
    #define strcat_s strcat
    #define vsprintf_s vsprintf
    #define _strnicmp strncasecmp

    // detect CPU platform
    #if defined(__i386__)
        #define PLATFORM_X86
        #define PLATFORM_32BIT
    #elif defined(__amd64__)
        #define PLATFORM_X64
        #define PLATFORM_64BIT
    #elif defined(__arm__)
        #define PLATFORM_ARM
        #define PLATFORM_32BIT
    #else
        #error "Unrecognized CPU platform"
    #endif


    #include <unistd.h>

#else

    // Unknown platform
    #error "Please define platform crap"

#endif

// Sizes
#ifdef PLATFORM_32BIT

    // Maximum reasonable sizes for IO and memory operations
    #define MAX_IO_SIZE (512ULL << 20)
    #define MAX_MEM_SIZE (512ULL << 20)
    #define MAX_SIZE_T (~(size_t)0U)

#elif defined(PLATFORM_64BIT)

    // Maximum reasonable sizes for IO and memory operations
    #define MAX_IO_SIZE (4ULL << 30)
    #define MAX_MEM_SIZE (64ULL << 30)
    #define MAX_SIZE_T (~(size_t)0ULL)

#endif

// CPU specific shit
#if defined(__i386__) || defined(__amd64__) || defined(_M_AMD64) || defined(_M_IX86)

    // MSVC bull
    #ifdef PLATFORM_MSVC
        #include <intrin.h>
    #endif

    // SSE
    #if defined(__SSE__) || (defined(_M_IX86_FP) && (_M_IX86_FP >= 1)) || defined(_M_AMD64)
        #define PLATFORM_SSE
        #include <xmmintrin.h>
    #endif

    // SSE2
    #if defined(__SSE2__) || (defined(_M_IX86_FP) && (_M_IX86_FP >= 2)) || defined(_M_AMD64)
        #define PLATFORM_SSE2
        #include <emmintrin.h>
    #endif

    // SSE3
    #if defined(__SSE3__) || (defined(_M_IX86_FP) && (_M_IX86_FP >= 2)) || defined(_M_AMD64)
        #define PLATFORM_SSE3
        #include <pmmintrin.h>
    #endif

    // SSE4
    #if defined(__SSE4_1__) || (defined(_M_IX86_FP) && (_M_IX86_FP >= 2)) || defined(_M_AMD64)
        #define PLATFORM_SSE41
        #include <smmintrin.h>
    #endif

    // SSE specific macros
    #define _MM_SHUFFLE_REV(x,y,z,w) _MM_SHUFFLE(w,z,y,x)

#endif

// Disable some warnings under MSVC
#ifdef PLATFORM_MSVC

    // Disable some more anoying level 4 Warnings
    #pragma warning ( disable: 4100 ) // unused function parameter
    #pragma warning ( disable: 4127 ) // conditional expression is constant
    #pragma warning ( disable: 4189 ) // local variable is initialized but not referenced
    #pragma warning ( disable: 4714 ) // function '' marked as __forceinline not inlined
    #pragma warning ( disable: 4324 ) // '' : structure was padded due to __declspec(align())
    #pragma warning ( disable: 4985 ) // attributes not present on previous declaration.
    #pragma warning ( disable: 4275 ) // dll linkage inconsistency
    #pragma warning ( disable: 4251 ) // dll interface
    #pragma warning ( disable: 4530 ) //  C++ exception handler used, but unwind semantics are not enabled.Specify / EHsc
    #pragma warning ( disable: 4091 ) // 'typedef ' : ignored on left of '' when no variable is declared
    #pragma warning ( disable: 4505 ) // unreferenced local function has been removed
    #pragma warning ( disable: 4200 ) // nonstandard extension used : zero - sized array in struct / union
    #pragma warning ( disable: 4359 ) // '' : Alignment specifier is less than actual alignment(4), and will be ignored.
	#pragma warning ( disable: 4624 ) // destructor was implicitly defined as deleted
	#pragma warning ( disable: 4389 ) // '==' : signed / unsigned mismatch
	#pragma warning ( disable: 4018 ) //  '<' : signed / unsigned mismatch
	#pragma warning ( disable: 4244 ) // anoying MS shit
	#pragma warning ( disable: 4267 ) //  'argument' : conversion from 'size_t' to 'const uint32_t', possible loss of data
	#pragma warning ( disable: 4201 ) //: nonstandard extension used : nameless struct / union
    #pragma warning ( disable: 4146 ) //: unary minus operator applied to unsigned type, result still unsigned

    #define TYPE_ALIGN_GLUE(x) x
    #define TYPE_ALIGN(x,y) __declspec(align( x )) TYPE_ALIGN_GLUE(y)
    #define TYPE_TLS __declspec(thread)

#ifdef PLATFORM_64BIT
    #define TYPE_UNALIGNED __unaligned
#else
    #define TYPE_UNALIGNED
#endif

    #define INLINE inline
    #define ALWAYS_INLINE __forceinline

    #define BYTESWAP_16(x) _byteswap_ushort(x)
    #define BYTESWAP_32(x) _byteswap_ulong(x)
    #define BYTESWAP_64(x) _byteswap_uin64(x)

    #define SPRINTF_S(buf, size, ...) sprintf_s(buf, size, __VA_ARGS__ )

#elif defined(PLATFORM_GCC)

    #define TYPE_ALIGN(x, type) type alignas(x)
    #define TYPE_TLS __thread
    #define TYPE_UNALIGNED

    #define INLINE inline
    #define ALWAYS_INLINE __attribute__((always_inline)) inline

    #define BYTESWAP_16(x) (uint16_t)__builtin_bswap16((uint16_t)x)
    #define BYTESWAP_32(x) (uint32_t)__builtin_bswap32((uint32_t)x)
    #define BYTESWAP_64(x) (uint64_t)__builtin_bswap64((uint64_t)x)

    #define SPRINTF_S(buf, size, ...) sprintf(buf, __VA_ARGS__)
    #define _stricmp strcasecmp
    #define strncpy_s strncpy
    #define strcpy_s strcpy

#elif defined(PLATFORM_CLANG)

    #define TYPE_ALIGN(x, type) type alignas(x)
    #define TYPE_TLS __thread
    #define TYPE_UNALIGNED

    #define INLINE inline
    #define ALWAYS_INLINE inline

    #define BYTESWAP_16(x) (uint16_t)__builtin_bswap16((uint16_t)x)
    #define BYTESWAP_32(x) (uint32_t)__builtin_bswap32((uint32_t)x)
    #define BYTESWAP_64(x) (uint64_t)__builtin_bswap64((uint64_t)x)

    #define SPRINTF_S(buf, size, ...) sprintf(buf, __VA_ARGS__)
    #define _stricmp strcasecmp
    #define strncpy_s strncpy
    #define strcpy_s strcpy

#else

    #error "Unsupported compiler"

#endif

#define CAN_YIELD

#define VA_ARGS(...) , ##__VA_ARGS__

// TODO: Move this to build system
// TODO: Disables IPv6 and high datagram bandwidth tests on Travis CI
#if defined(PLATFORM_LINUX)
    #define FUCKED_UP_NETSTACK
#elif defined(PLATFORM_WINDOWS)
    #define BUILD_WITH_DEVTOOLS
    //#define BUILD_WITH_GAME
#endif

// magic
#define memzero(ptr, size) memset(ptr, 0, size)

// namespace macros, needed for reflection tools as well as parsing the "namespace X {" is tedious
#define BEGIN_BOOMER_NAMESPACE(name) namespace name {
#define END_BOOMER_NAMESPACE(name) }
