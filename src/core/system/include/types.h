/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform #]
***/

#pragma once

#include <inttypes.h>

//-----------------------------------------------------------------------------

// Invalid index
#define INDEX_NONE          (-1)

// Max uint32_t value
#define INDEX_MAX         (~0U)

// Max uint64_t value
#define INDEX_MAX64         (~0ULL)

// Infinite wait
#define INFINITE_TIME       (0xFFFFFFFFU)

//-----------------------------------------------------------------------------

// Size of the array
#define ARRAY_COUNT(array) (int)(sizeof(array) / sizeof((array)[0]))

//-----------------------------------------------------------------------------

// Nice macro to do enum flags
#define FLAG(x) (1U << (x))

#define STRINGIFY(a) #a
#define STRINGIFICATION(a) STRINGIFY(a)

//-----------------------------------------------------------------------------
/// Calling conventions

#ifdef PLATFORM_WIN32
    #define STDCALL _stdcall
#else
    #define STDCALL
#endif

//-----------------------------------------------------------------------------

// Some version definition stuff
#define BUILD_VER "Boomer Engine v4. Build " __DATE__ " " __TIME__ "."

// CL number (if defined)
#define BUILD_CL "Internal Build (No CL)"

//-----------------------------------------------------------------------------