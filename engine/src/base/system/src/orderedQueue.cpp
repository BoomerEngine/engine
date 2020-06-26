/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading #]
***/

#include "build.h"
#include "orderedQueue.h"

#ifdef PLATFORM_WINDOWS
    #include "orderedQueueWindows.h"
#elif defined(PLATFORM_POSIX)
    #include "orderedQueuePOSIX.h"
#endif

namespace base
{
    //---

    IOrderedQueue::~IOrderedQueue()
    {}

    IOrderedQueue* IOrderedQueue::Create()
    {
#if defined(PLATFORM_WINDOWS)
        return prv::WinOrderedQueue::Create();
#elif defined(PLATFORM_POSIX)
        return prv::POSIXOrderedQueue::Create();
#else
        return nullptr; // no thread needed apparently :P
#endif
    }

    //---

} // inf