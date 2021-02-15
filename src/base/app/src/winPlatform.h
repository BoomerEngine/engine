/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: launcher\windows #]
* [# platform: winapi #]
***/
#pragma once

#include "launcherPlatformCommon.h"

namespace base
{
    namespace platform
    {
        namespace win
        {

            class GenericOutput;

            class Platform : public CommonPlatform
            {
            public:
                Platform();
                virtual ~Platform();

                virtual bool handleStart(const app::CommandLine& cmdline, app::IApplication* app) override;
                virtual void handleUpdate() override;
                virtual void handleCleanup() override;

            private:
                bool m_hasLog;
                bool m_hasErrors;

                UniquePtr<GenericOutput> m_output;

                bool protectedStart(const app::CommandLine& cmdline, app::IApplication* app);
            };

        } // win
    } // platform
} // base