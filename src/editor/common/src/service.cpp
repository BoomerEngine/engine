/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#include "build.h"
#include "service.h"
#include "window.h"
#include "progressDialog.h"
#include "task.h"
#include "mainWindow.h"

#include "core/io/include/io.h"
#include "core/app/include/commandline.h"
#include "core/resource/include/loader.h"
#include "core/resource/include/depot.h"
#include "core/containers/include/path.h"
#include "core/xml/include/xmlUtils.h"
#include "core/xml/include/xmlDocument.h"
#include "core/xml/include/xmlWrappers.h"
#include "core/io/include/fileFormat.h"
#include "core/net/include/tcpMessageServer.h"
#include "core/net/include/messageConnection.h"
#include "core/net/include/messagePool.h"
#include "core/fibers/include/backgroundJob.h"
#include "core/resource_compiler/include/fingerprintService.h"
#include "core/resource/include/metadata.h"

#include "engine/ui/include/uiDockLayout.h"
#include "engine/ui/include/uiButton.h"
#include "engine/ui/include/uiMessageBox.h"
#include "engine/ui/include/nativeWindowRenderer.h"
#include "engine/ui/include/uiDataStash.h"
#include "engine/ui/include/uiRenderer.h"
#include "engine/ui/include/uiElementConfig.h"
#include "engine/canvas/include/service.h"

#include "gpu/device/include/deviceService.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

ConfigProperty<double> cvConfigAutoSaveTime("Editor", "ConfigSaveInterval", 15.0f);
ConfigProperty<double> cvDataAutoSaveTime("Editor", "DataAutoSaveInterval", 120.0f);

//---

RTTI_BEGIN_TYPE_CLASS(EditorService);
RTTI_METADATA(DependsOnServiceMetadata).dependsOn<gpu::DeviceService>();
RTTI_METADATA(DependsOnServiceMetadata).dependsOn<LoadingService>();
RTTI_METADATA(DependsOnServiceMetadata).dependsOn<canvas::CanvasService>();
RTTI_END_TYPE();

//--

EditorService::EditorService()
{}

EditorService::~EditorService()
{}

bool EditorService::onInitializeService(const CommandLine& cmdLine)
{
    // create configuration
    m_configStorage.create();
    m_configRootBlock.create(m_configStorage.get(), "");

    // load config data (does not apply the config)
    m_configPath = TempString("{}editor.config.xml", SystemPath(PathCategory::UserConfigDir));
    m_configStorage->loadFromFile(m_configPath);
    return true;
}

void EditorService::onShutdownService()
{
    saveConfig();

    if (m_stared)
    {
        for (auto window : m_windows)
            m_renderer->dettachWindow(window);

        m_renderer.reset();
        m_dataStash.reset();
        m_nativeRenderer.reset();
    }

    if (m_mainWindow)
    {
        m_mainWindow->requestClose();
        m_mainWindow.reset();
    }

    m_configStorage->saveToFile(m_configPath);

    m_configRootBlock.reset();
    m_configStorage.reset();
}

void EditorService::onSyncUpdate()
{
    if (m_nextConfigSave.reached())
        saveConfig();

    updateUI();
    updateBackgroundJobs();
    updateWindows();
}

bool EditorService::start()
{
    DEBUG_CHECK_RETURN_EX_V(!m_stared, "Editor already started", true);

    // create low-level UI window handler
    m_nativeRenderer.create();

    // create UI data stash (styles, fonts, icons)
    m_dataStash = RefNew<ui::DataStash>();

    // create high-level UI renderer, needed to show any windows
    m_renderer.create(m_dataStash.get(), m_nativeRenderer.get());

    // attach all existing windows
    for (const auto& window : m_windows)
        m_renderer->attachWindow(window);

    // attach main window
    m_mainWindow = RefNew<MainWindow>();
    attachWindow(m_mainWindow);

    // go back to main window
    m_mainWindow->requestActivate();

    // started
    m_stared = true;
    TRACE_INFO("Editor service started");
    return true;
}

void EditorService::saveConfig()
{
    ScopeTimer timer;

    TRACE_INFO("Saving config...");

    saveWindows();

    DispatchGlobalEvent(eventKey(), EVENT_EDITOR_CONFIG_SAVE);

    m_configStorage->saveToFile(m_configPath); 
    m_nextConfigSave = NativeTimePoint::Now() + cvConfigAutoSaveTime.get();

    TRACE_INFO("Config saved in {}", timer);
}

//---

void EditorService::attachWindow(IEditorWindow* window)
{
    DEBUG_CHECK_RETURN_EX(window, "Invalid window");
    DEBUG_CHECK_RETURN_EX(!m_windows.contains(window), "Window already attached");

    // create native representation of the window
    m_windows.pushBack(AddRef(window));
    m_renderer->attachWindow(window); // note: does not yet create a physical representation

    // load the configuration
    if (window->tag())
    {
        auto windowConfig = m_configRootBlock->tag("Windows").tag(window->tag());
        window->configLoad(windowConfig);
    }
}

void EditorService::detachWindow(IEditorWindow* window)
{
    DEBUG_CHECK_RETURN_EX(window, "Invalid window");
    DEBUG_CHECK_RETURN_EX(m_windows.contains(window), "Window not attached");

    // force-save the config 
    if (window->tag())
    {
        auto windowConfig = m_configRootBlock->tag("Windows").tag(window->tag());
        window->configSave(windowConfig);
    }

    // detach window
    m_windows.remove(window);
    m_renderer->dettachWindow(window);
}

//---

void EditorService::scheduleTask(IEditorTask* job, bool openUI)
{
    if (job)
    {
        TRACE_INFO("Editor: Scheduled editor task '{}'", job->description());

        // add command runner to list so we can observe if it's not dead
        {
            auto lock = CreateLock(m_backgroundJobsLock);
            m_pendingBackgroundJobs.pushBack(AddRef(job));
        }

        /*if (openUI)
            showBackgroundJobDialog(job);*/
    }
}

/*void EditorService::showBackgroundJobDialog(IBackgroundTask* job)
{
    DEBUG_CHECK_RETURN_EX(job, "No job specified");
    m_pendingBackgroundJobUIRequest = AddRef(job);
}*/

void EditorService::updateBackgroundJobs()
{
    // start new background job
    if (!m_activeBackgroundJob)
    {
        auto lock = CreateLock(m_backgroundJobsLock);
        while (!m_pendingBackgroundJobs.empty())
        {
            auto job = m_pendingBackgroundJobs[0];
            m_pendingBackgroundJobs.erase(0);

            if (job->start())
            {
                DispatchGlobalEvent(eventKey(), EVENT_EDITOR_TASK_STARTED, job);
                m_activeBackgroundJob = job;
                break;
            }
        }
    }

    // update background job
    if (m_activeBackgroundJob)
    {
        if (m_activeBackgroundJob->update())
        {
            DispatchGlobalEvent(eventKey(), EVENT_EDITOR_TASK_FINISHED, m_activeBackgroundJob);
            m_activeBackgroundJob.reset();
        }
    }
}

//--

void EditorService::updateUI()
{
    static auto lastUpdateTime = NativeTimePoint::Now();

    auto dt = std::clamp<float>(lastUpdateTime.timeTillNow().toSeconds(), 0.0001f, 0.1f);
    lastUpdateTime.resetToNow();

    m_renderer->updateAndRender(dt);
}

void EditorService::updateWindows()
{
    // tick all the window
    for (const auto& window : m_windows)
        window->update();

    // auto remove closed windows from the list
    for (auto i : m_windows.indexRange().reversed())
    {
        auto window = m_windows[i];

        // auto close resource container windows that have no resources
        /*if (auto container = rtti_cast<IBaseResourceContainerWindow>(m_windows[i]))
        {
            if (!container->hasEditors())
                window->requestClose();
        }*/

        // when closing a window remove it from the list
        if (window->requestedClose())
        {
            auto windowConfig = m_configRootBlock->tag("Windows").tag(window->tag());
            window->configSave(windowConfig);

            m_windows.erase(i);
        }
    }
}

void EditorService::saveWindows() const
{
    for (const auto& window : m_windows)
    {
        if (window->tag())
        {
            auto windowConfig = m_configRootBlock->tag("Windows").tag(window->tag());
            window->configSave(windowConfig);
        }
    }
}

//-

END_BOOMER_NAMESPACE_EX(ed)
