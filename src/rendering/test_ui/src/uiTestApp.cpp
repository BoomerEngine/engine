/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: test #]
***/

#include "build.h"
#include "uiTestApp.h"
#include "uiTestWindow.h"

#include "base/app/include/application.h"
#include "base/input/include/inputContext.h"
#include "base/input/include/inputStructures.h"
#include "base/canvas/include/canvas.h"

#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingOutput.h"
#include "rendering/device/include/renderingCommandBuffer.h"
#include "rendering/device/include/renderingDeviceService.h"
#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/ui_host/include/renderingWindowRenderer.h"

#include "base/app/include/launcherPlatform.h"
#include "base/ui/include/uiStyleLibrary.h"
#include "base/ui/include/uiDataStash.h"

BEGIN_BOOMER_NAMESPACE(rendering::test)

//--

UIApp::UIApp()
{}

UIApp::~UIApp()
{
}

bool UIApp::initialize(const base::app::CommandLine& commandline)
{
	auto dev = base::GetService<DeviceService>();
	if (!dev)
		return false;

	m_nativeRenderer.create();
    m_dataStash = base::RefNew<ui::DataStash>();

    m_renderer.create(m_dataStash.get(), m_nativeRenderer.get());
    m_lastUpdateTime.resetToNow();

    auto window = base::RefNew<TestWindow>();
    m_renderer->attachWindow(window.get());

    return true;
}

void UIApp::cleanup()
{
    m_renderer.reset();
    m_nativeRenderer.reset();
    m_dataStash.reset();
}

void UIApp::update()
{
    auto dt = std::clamp<float>(m_lastUpdateTime.timeTillNow().toSeconds(), 0.0001f, 0.1f);
    m_lastUpdateTime.resetToNow();
    m_renderer->updateAndRender(dt);

    if (m_renderer->windows().empty())
        base::platform::GetLaunchPlatform().requestExit("All windows closed");
}

END_BOOMER_NAMESPACE(rendering::test)

base::app::IApplication& GetApplicationInstance()
{
    static rendering::test::UIApp theApp;
    return theApp;
}

