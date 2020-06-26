/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: launcher\posix #]
* [# platform: posix #]
***/
#pragma once

#include "launcherPlatformCommon.h"

namespace base
{
    namespace platform
    {
        namespace posix
        {

            class GenericOutput;

            class Platform : public CommonPlatform
            {
            public:
                Platform();
                virtual ~Platform();

                virtual ExitCode start(const app::CommandLine& cmdline) override;
                virtual bool update() override;
                virtual void cleanup() override;

            private:
                bool m_hasLog;
                bool m_hasErrors;

                UniquePtr<GenericOutput> m_output;

                ExitCode protectedStart(const app::CommandLine& cmdline);

                void installSignalHandlers();

                static void SignalHandler(int sig_num, void* info, void* ucontext);
                static void PipeHandler(int sig_num);
            };

        } // posix
    } // platform
} // base