/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "core/app/include/application.h"
#include "editor/common/include/service.h"
#include "gpu/device/include/output.h"
#include "engine/ui/include/nativeWindowRenderer.h"

BEGIN_BOOMER_NAMESPACE()

// editor application
class EditorApp : public IApplication
{
public:
    virtual void cleanup() override final;
    virtual bool initialize(const CommandLine& commandline) override final;
    virtual void update() override final;

private:
    NativeTimePoint m_lastUpdateTime;
};

END_BOOMER_NAMESPACE()