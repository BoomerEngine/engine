/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "public.h"

//#define DEEP_SHADOW_DEBUG

#ifdef DEEP_SHADOW_DEBUG
#define TRACE_DEEP(x, ...) { TRACE_INFO(x, __VA_ARGS__) }
#else
#define TRACE_DEEP(x, ...) { (void)x; }
#endif