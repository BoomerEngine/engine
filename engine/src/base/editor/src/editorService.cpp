/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#include "build.h"
#include "editorService.h"
#include "editorConfig.h"
#include "editorWindow.h"
#include "assetFileImportWindow.h"

#include "managedDepot.h"
#include "managedFile.h"
#include "managedFileFormat.h"
#include "versionControl.h"

#include "base/io/include/absolutePath.h"
#include "base/io/include/ioSystem.h"
#include "base/app/include/commandline.h"
#include "base/resource_compiler/include/depotStructure.h"
#include "base/resource/include/resourceLoadingService.h"
#include "base/ui/include/uiRenderer.h"

namespace ed
{

    //---

    ConfigProperty<double> cvConfigAutoSaveTime("Editor", "ConfigSaveInterval", 15.0f);
    ConfigProperty<double> cvDataAutoSaveTime("Editor", "DataAutoSaveInterval", 120.0f);

    //---

    RTTI_BEGIN_TYPE_CLASS(Editor);
        RTTI_METADATA(app::DependsOnServiceMetadata).dependsOn<res::LoadingService>();
    RTTI_END_TYPE();

    Editor::Editor()
        : m_mainDockId(0)
    {}

    Editor::~Editor()
    {
    }

    app::ServiceInitializationResult Editor::onInitializeService(const app::CommandLine& cmdLine)
    {
        // only use in editor
        if (!cmdLine.hasParam("editor"))
            return app::ServiceInitializationResult::Silenced;

        // we can initialize the managed depot only if we are running from uncooked data
        auto loaderService = GetService<res::LoadingService>();
        if (!loaderService || !loaderService->loader())
        {
            TRACE_ERROR("No resource loading service spawned, no editor can be created");
            return app::ServiceInitializationResult::FatalError;
        }

        // prepare depot for the editor
        auto depot = loaderService->loader()->queryUncookedDepot();
        if (!depot)
        {
            TRACE_ERROR("Unable to convert engine depot into writable state, no editor will start");
            return app::ServiceInitializationResult::FatalError;
        }

        // enable live tracking of files
        depot->enableDepotObservers();

        // create configuration
        m_config.create();

        // cache format information
        ManagedFileFormatRegistry::GetInstance().cacheFormats();

        // initialize the managed depot
        m_managedDepot = CreateUniquePtr<ManagedDepot>(*depot, config()["Depot"]);
        m_managedDepot->populate();

        // create version control
        // TODO: add actual integration
        m_versionControl = vsc::CreateLocalStub();

        // load config
        m_configPath = IO::GetInstance().systemPath(io::PathCategory::UserConfigDir).addFile("editor.ini");
        m_config->load(m_configPath);

        // we are initialized
        return app::ServiceInitializationResult::Finished;
    }

    void Editor::onShutdownService()
    {
        m_managedDepot.reset();
        m_versionControl.reset();
        m_config.reset();
    }

    bool Editor::start(ui::Renderer* renderer)
    {
        DEBUG_CHECK(renderer);
        m_renderer = renderer;

        if (!m_mainWindow)
        {
            m_mainWindow = base::CreateSharedPtr<MainWindow>();
            m_mainWindow->loadConfig(config()["Editor"]);
            renderer->attachWindow(m_mainWindow);
        }

        return true;
    }

    void Editor::stop()
    {
        saveConfig();

        if (m_mainWindow)
        {
            m_mainWindow->requestClose();
            m_mainWindow.reset();
        }

        m_renderer = nullptr;
        m_config->save(m_configPath);
    }

    void Editor::saveConfig()
    {
        TRACE_INFO("Saving config...");

        if (m_mainWindow)
        {
            auto mainWindowConfig = config()["Editor"];
            m_mainWindow->saveConfig(mainWindowConfig);
        }

        if (m_assetImporter)
        {
            auto assetWindowConfig = config()["AssetImporter"];
            m_assetImporter->saveConfig(assetWindowConfig);
        }

        m_config->save(m_configPath); 
        m_nextConfigSave = NativeTimePoint::Now() + cvConfigAutoSaveTime.get();
    }

    void Editor::saveAssetsSafeCopy()
    {
        //TRACE_INFO("Auto save...");
        //m_nextConfigSave = NativeTimePoint::Now() + cvDataAutoSaveTime.get();
    }

    void Editor::onSyncUpdate()
    {
        if (m_nextConfigSave.reached())
            saveConfig();

        if (m_nextAutoSave.reached())
            saveAssetsSafeCopy();

        // process internal IO events
        m_managedDepot->dispatchEvents();
    }

    //---

    bool Editor::canOpenFile(const ManagedFileFormat& format) const
    {
        return m_mainWindow->canOpenFile(format);
    }

    bool Editor::openFile(ManagedFile* file)
    {
        return m_mainWindow->openFile(file);
    }

    bool Editor::selectFile(ManagedFile* file)
    {
        return m_mainWindow->selectFile(file);
    }

    ManagedFile* Editor::selectedFile() const
    {
        return m_mainWindow->selectedFile();
    }

    ManagedDirectory* Editor::selectedDirectory() const
    {
        return m_mainWindow->selectedDirectory();
    }

    //--

    bool Editor::openAssetImporter()
    {
        if (!m_assetImporter)
        {
            m_assetImporter = base::CreateSharedPtr<AssetImportWindow>();
            m_assetImporter->loadConfig(config()["AssetImporter"]);
            m_renderer->attachWindow(m_assetImporter);
        }

        m_assetImporter->requestShow(true);
        return true;
    }

    bool Editor::addImportFiles(const base::Array<base::StringBuf>& assetPaths, const ManagedDirectory* directoryOverride)
    {
        if (!directoryOverride)
        {
            directoryOverride = m_mainWindow->selectedDirectory();
            if (!directoryOverride)
            {
                ui::PostNotificationMessage(m_mainWindow, ui::MessageType::Warning, "ImportAsset"_id, "No directory selected to import to");
                return false;
            }
        }

        if (!openAssetImporter())
            return false;

        m_assetImporter->addFiles(directoryOverride, assetPaths);
        return true;
    }

    //--

    uint64_t Editor::windowNativeHandle(ui::IElement* elem) const
    {
        if (elem)
            if (auto window = elem->findParentWindow())
                return m_renderer->queryWindowNativeHandle(window);

        return 0;
    }

    //--

} // editor
