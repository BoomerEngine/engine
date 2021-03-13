/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "core/app/include/application.h"

BEGIN_BOOMER_NAMESPACE()

class BCCLogSinkWithErrorCapture;

// commandline processor application
class BCCApp : public IApplication
{
public:
    BCCApp();
    virtual ~BCCApp();

    virtual bool initialize(const CommandLine& commandline) override final;
    virtual void update() override final;
    virtual void cleanup() override final;

    UniquePtr<BCCLogSinkWithErrorCapture> m_globalSink;
    CommandHostPtr m_commandHost;

    NativeTimePoint m_startedTime;

    void runInternal();
};

END_BOOMER_NAMESPACE()

