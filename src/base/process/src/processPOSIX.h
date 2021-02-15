/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\process\posix #]
* [#platform: posix #]
***/

#pragma once

#include "process.h"
#include "base/system/include/atomic.h"

namespace base
{
    namespace process
    {
        namespace prv
        {

            /// POSIX based process
            class POSIXProcess : public IProcess
            {
            public:
                static POSIXProcess* Create(const ProcessSetup& setup);

                //--

                POSIXProcess();
                virtual ~POSIXProcess();

                virtual bool wait(uint32_t timeoutMS) override;
                virtual void terminate() override;
                virtual ProcessID id() const override;
                virtual bool isRunning() const override;
                virtual bool exitCode(int& outExitCode) const override;

            private:
                pid_t m_pid;
            };

        } // prv
    } // process
} // base