/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: impl\posix #]
* [# platform: posix #]
***/

#pragma once

#include "fiberSystemCommon.h"

namespace base
{
    namespace fibers
    {
        namespace prv
        {

            /// POSIX based implementation of the fiber system
            class PosixScheduler : public BaseScheduler
            {
            public:
                PosixScheduler();
                virtual ~PosixScheduler();

            private:
                static pthread_key_t GTlsThreadState;

                //--

                static void* ThreadFunc(void* threadParameter);
                static void WorkerFiberFunc(uint32_t fiberParameterLo, uint32_t fiberParameterHi);
                static void SchedulerFiberFunc(uint32_t fiberParameterLo, uint32_t fiberParameterHi);
                static FiberHandle CreateContext(void (*taskFunction)(uint32_t, uint32_t), uint32_t stackSize, const void* paramToPass);

                //--

                /// BaseScheduler interface
                virtual ThreadState* currentThreadState() const override final;
                virtual ThreadHandle createWorkerThread(void* state) override final;
                virtual void convertMainThread() override final;
                virtual FiberHandle createFiberState(uint32_t stackSize, void* state) override final;
                virtual void switchFiber(FiberHandle from, FiberHandle fiber) override final;
            };

        } // win
    } // fibers
} // base

