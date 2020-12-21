/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base/app/include/application.h"
#include "rendering/device/include/renderingOutput.h"
#include "rendering/ui_host/include/renderingWindowRenderer.h"

namespace application
{
    // editor application
    class APP_COMMON_API EditorApp : public base::app::IApplication
    {
    public:
        virtual void cleanup() override final;
        virtual bool initialize(const base::app::CommandLine& commandline) override final;
        virtual void update() override final;

    private:
        base::UniquePtr<ui::Renderer> m_renderer;
        base::RefPtr<ui::DataStash> m_dataStash;
        base::UniquePtr<rendering::NativeWindowRenderer> m_nativeRenderer;
        base::NativeTimePoint m_lastUpdateTime;
    };

} // application