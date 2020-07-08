/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "editorApplication.h"

#include "base/app/include/launcherPlatform.h"
#include "base/editor/include/editorService.h"
#include "base/ui/include/uiStyleLibrary.h"
#include "base/ui/include/uiDataStash.h"
#include "base/ui/include/uiRenderer.h"

namespace application
{
    //---

    bool EditorApp::initialize(const base::app::CommandLine& commandline)
    {
        auto styles = base::LoadResource<ui::style::Library>(base::res::ResourcePath("editor/interface/styles/flat.scss"));
        if (!styles)
            return false;

        m_nativeRenderer.create();
        m_dataStash = base::CreateSharedPtr<ui::DataStash>(styles);
        m_dataStash->addIconSearchPath("editor/interface/icons/");
        m_renderer.create(m_dataStash.get(), m_nativeRenderer.get());
        m_lastUpdateTime.resetToNow();

        auto* service = base::GetService<ed::Editor>();
        if (!service)
            return false;

        if (!service->start(m_renderer.get()))
            return false;

        return true;
    }

    void EditorApp::cleanup()
    {
        if (auto* service = base::GetService<ed::Editor>())
            service->stop();

        m_renderer.reset();
        m_nativeRenderer.reset();
        m_dataStash.reset();
    }

    void EditorApp::update()
    {
        auto dt = std::clamp<float>(m_lastUpdateTime.timeTillNow().toSeconds(), 0.0001f, 0.1f);
        m_lastUpdateTime.resetToNow();
        m_renderer->updateAndRender(dt);

        if (m_renderer->windows().empty())
            base::platform::GetLaunchPlatform().requestExit("All windows closed");
    }

    //---

} // application

