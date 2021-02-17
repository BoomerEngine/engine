/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base/app/include/application.h"

namespace application
{

    class BCCLogSinkWithErrorCapture;

    // commandline processor application
    class BCCApp : public base::app::IApplication
    {
    public:
        BCCApp();
        virtual ~BCCApp();

        virtual bool initialize(const base::app::CommandLine& commandline) override final;
        virtual void update() override final;
        virtual void cleanup() override final;

        base::UniquePtr<BCCLogSinkWithErrorCapture> m_globalSink;
        base::app::CommandHostPtr m_commandHost;

        base::NativeTimePoint m_startedTime;

        void runInternal();
    };

} // application
