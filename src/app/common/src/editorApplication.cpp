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
#include "rendering/device/include/renderingDeviceService.h"

namespace application
{
    //---

    bool EditorApp::initialize(const base::app::CommandLine& commandline)
    {
		auto dev = base::GetService<DeviceService>();
		if (!dev)
			return false;

        m_nativeRenderer.create();

        m_dataStash = base::RefNew<ui::DataStash>();

        m_renderer.create(m_dataStash.get(), m_nativeRenderer.get());

        m_lastUpdateTime.resetToNow();

        m_editor.create();

        return m_editor->initialize(m_renderer.get(), commandline);
    }

    void EditorApp::cleanup()
    {
        m_editor.reset();
        m_renderer.reset();
        m_nativeRenderer.reset();
        m_dataStash.reset();
    }

    void EditorApp::update()
    {
        m_editor->update();

        auto dt = std::clamp<float>(m_lastUpdateTime.timeTillNow().toSeconds(), 0.0001f, 0.1f);
        m_lastUpdateTime.resetToNow();
        m_renderer->updateAndRender(dt);

        if (m_renderer->windows().empty())
            base::platform::GetLaunchPlatform().requestExit("All windows closed");
    }

    //---

} // application

