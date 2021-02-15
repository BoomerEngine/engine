/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: impl\winapi #]
* [# platform: winapi #]
***/

#pragma once

#include "fiberSystem.h"
#include "fiberSystemCommon.h"
#include "base/system/include/spinlock.h"
#include "base/containers/include/array.h"
#include "base/system/include/debug.h"

#include <Windows.h>

namespace base
{
    namespace fibers
    {
        namespace prv
        {

            /// WinAPI based implementation of the fiber system
            class WinApiScheduler : public BaseScheduler
            {
            public:
                WinApiScheduler();
                virtual ~WinApiScheduler();

            private:
                static const uint32_t GTlsThreadState;

                //--

                static DWORD WINAPI ThreadFunc(void* threadParameter);
                static void WINAPI WorkerFiberFunc(void* threadParameter);
                static void WINAPI SchedulerFiberFunc(void* threadParameter);

                //--

                /// BaseScheduler interface
                virtual ThreadState* currentThreadState() const override final;
                virtual ThreadHandle createWorkerThread(void* state) override final;
                virtual void convertMainThread() override final;
                virtual FiberHandle createFiberState(uint32_t stackSize, void *state) override final;
                virtual void switchFiber(FiberHandle from, FiberHandle to) override final;
            };

        } // win
    } // fibers
} // base

