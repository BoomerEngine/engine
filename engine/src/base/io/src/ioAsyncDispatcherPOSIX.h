/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system\posix #]
* [#platform: posix #]
***/

#pragma once

#include "absolutePath.h"
#include "base/containers/include/stringBuf.h"
#include "base/containers/include/threadSafeQueue.h"
#include "base/system/include/thread.h"
#include "base/system/include/semaphoreCounter.h"
#include "base/fibers/include/fiberSystem.h"
#include "base/containers/include/threadSafeStructurePool.h"

namespace base
{
    namespace io
    {
        namespace prv
        {

            class POSIXFileHandle;

            // dispatch for IO jobs
            class POSIXAsyncReadDispatcher
            {
            public:
                POSIXAsyncReadDispatcher(uint32_t maxInFlightRequests);
                ~POSIXAsyncReadDispatcher();

                // process async IO request, returns the number of bytes read
                CAN_YIELD uint64_t readAsync(int hFile, uint64_t offset, uint64_t size, void* outMemory);

            private:
                Mutex m_lock;
                uint32_t m_maxRequests;
            };

        } // prv
    } // io
} // base