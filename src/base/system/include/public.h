/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

// Glue headers and logic
#include "base_system_glue.inl"

// Disable the bullshit
#define  _CRT_SECURE_NO_WARNINGS

// Disable exceptions
#undef _HAS_EXCEPTIONS
#define _HAS_EXCEPTIONS 0

// STL
#include <memory>
#include <algorithm>
#include <utility>
#include <atomic>
#include <functional>
#include <inttypes.h>
#include <iostream>
#include <string.h>

// Expose some base headers
#include "settings.h"
#include "types.h"
#include "algorithms.h" 
#include "singleton.h"
#include "format.h"
#include "output.h"
#include "module.h"
#include "timedScope.h"
#include "scopeLock.h"
#include "staticCRC.h"
#include "profiling.h"
#include "systemInfo.h"
#include "task.h"
#include "formatBuffer.h"
#include "atomic.h"

BEGIN_BOOMER_NAMESPACE(base)

typedef uint32_t ThreadID;
typedef uint32_t ProcessID;

class Event;
class Semaphore;
class Thread;

namespace process
{
    class IProcess;
    class IPipeWriter;
    class IPipeReader;
}

#define QUEUE_INSPECTOR const void* payload
typedef std::function<void(QUEUE_INSPECTOR)> TQueueInspectorFunc;

END_BOOMER_NAMESPACE(base)

