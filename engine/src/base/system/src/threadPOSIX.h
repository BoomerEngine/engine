/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading\posix #]
* [#platform: posix #]
***/

#pragma once

#include "thread.h"
#include "atomic.h"

namespace base
{
    namespace prv
    {

        /// pthreads based thread
        class POSIXThread : public NoCopy
        {
        public:
            static void Init(void* data, const ThreadSetup& setup);
            static void Close(void* data);

            //---

            static void* ThreadFunc(void *ptr);

            pthread_t m_handle = 0;
        };

    } // prv
} // base