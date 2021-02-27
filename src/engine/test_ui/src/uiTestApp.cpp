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

#include "core/app/include/application.h"
#include "core/input/include/inputContext.h"
#include "core/input/include/inputStructures.h"
#include "engine/canvas/include/canvas.h"

#include "gpu/device/include/device.h"
#include "gpu/device/include/output.h"
#include "gpu/device/include/commandBuffer.h"
#include "gpu/device/include/deviceService.h"
#include "gpu/device/include/commandWriter.h"
#include "engine/ui/include/nativeWindowRenderer.h"

#include "core/app/include/launcherPlatform.h"
#include "engine/ui/include/uiStyleLibrary.h"
#include "engine/ui/include/uiDataStash.h"

BEGIN_BOOMER_NAMESPACE_EX(test)

//--

UIApp::UIApp()
{}

UIApp::~UIApp()
{
}

bool UIApp::initialize(const app::CommandLine& commandline)
{
	auto dev = GetService<DeviceService>();
	if (!dev)
		return false;

	m_nativeRenderer.create();
    m_dataStash = RefNew<ui::DataStash>();

    m_renderer.create(m_dataStash.get(), m_nativeRenderer.get());
    m_lastUpdateTime.resetToNow();

    auto window = RefNew<TestWindow>();
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
        platform::GetLaunchPlatform().requestExit("All windows closed");
}

END_BOOMER_NAMESPACE_EX(test)

boomer::app::IApplication& GetApplicationInstance()
{
    static boomer::test::UIApp theApp;
    return theApp;
}

