/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "core/app/include/application.h"
#include "editor/common/include/editorService.h"
#include "gpu/device/include/renderingOutput.h"
#include "engine/ui/include/nativeWindowRenderer.h"

BEGIN_BOOMER_NAMESPACE()

// editor application
class EditorApp : public app::IApplication
{
public:
    virtual void cleanup() override final;
    virtual bool initialize(const app::CommandLine& commandline) override final;
    virtual void update() override final;

private:
    UniquePtr<ui::Renderer> m_renderer;
    RefPtr<ui::DataStash> m_dataStash;
    UniquePtr<ui::NativeWindowRenderer> m_nativeRenderer;
    NativeTimePoint m_lastUpdateTime;
    UniquePtr<ed::Editor> m_editor;
};

END_BOOMER_NAMESPACE()