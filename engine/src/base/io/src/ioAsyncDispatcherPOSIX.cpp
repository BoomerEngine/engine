/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system\posix #]
* [#platform: posix #]
***/

#include "build.h"
#include "ioFileHandlePOSIX.h"
#include "ioAsyncDispatcherPOSIX.h"

#include <aio.h>

namespace base
{
    namespace io
    {
        namespace prv
        {

            POSIXAsyncReadDispatcher::POSIXAsyncReadDispatcher(uint32_t maxInFlightRequests)
                : m_maxRequests(maxInFlightRequests)
            {
            }

            POSIXAsyncReadDispatcher::~POSIXAsyncReadDispatcher()
            {
            }

            uint64_t POSIXAsyncReadDispatcher::readAsync(int hFile, uint64_t offset, uint64_t size, void* outMemory)
            {
                PC_SCOPE_LVL1(AsyncRead, base::profiler::colors::Red800);

                ScopeLock<> lock(m_lock);

                lseek64(hFile, offset, SEEK_SET);
                return read(hFile, outMemory, size);
            }

        } // prv
    } // io
} // base

