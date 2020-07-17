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
#include "base/xml/include/xmlUtils.h"
#include "base/xml/include/xmlDocument.h"
#include "base/xml/include/xmlWrappers.h"
#include "base/io/include/fileFormat.h"

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

        // load open/save settings
        loadOpenSaveSettings(config()["OpenSaveDialog"]);

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

        if (m_assetImporter)
        {
            m_assetImporter->requestClose();
            m_assetImporter.reset();
        }

        if (m_mainWindow)
        {
            m_mainWindow->requestClose();
            m_mainWindow.reset();
        }

        m_renderer = nullptr;
        m_config->save(m_configPath);

        m_openSavePersistentData.clearPtr();
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

        // save open/save settings
        {
            auto openSaveConfig = config()["AssetImporter"];
            saveOpenSaveSettings(openSaveConfig);
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

    void Editor::loadOpenSaveSettings(const ConfigGroup& config)
    {

    }

    void Editor::saveOpenSaveSettings(ConfigGroup& config) const
    {

    }

    base::io::OpenSavePersistentData& Editor::openSavePersistentData(base::StringView<char> category)
    {
        if (!category)
            category = "Generic";

        if (const auto* data = m_openSavePersistentData.find(category))
            return **data;

        auto entry = MemNew(base::io::OpenSavePersistentData).ptr;
        entry->directory = IO::GetInstance().systemPath(base::io::PathCategory::UserDocumentsDir);

        m_openSavePersistentData[base::StringBuf(category)] = entry;
        return *entry;
    }

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

    bool Editor::addImportFiles(const base::Array<base::StringBuf>& assetPaths, base::SpecificClassType<base::res::IResource> importClass, const ManagedDirectory* directoryOverride)
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

        m_assetImporter->addNewImportFiles(directoryOverride, importClass, assetPaths);
        return true;
    }

    //--



    bool Editor::saveToXML(ui::IElement* owner, base::StringView<char> category, const std::function<base::ObjectPtr()>& makeXMLFunc, base::UTF16StringBuf* currentFileNamePtr)
    {
        // use main window when nothing was provided
        if (!owner)
            owner = m_mainWindow;

        // get the open/save dialog settings
        auto& dialogSettings = openSavePersistentData(category);

        // add XML format
        base::InplaceArray<base::io::FileFormat, 1> formatList;
        formatList.emplaceBack("xml", "Extensible Markup Language file");

        // current file name
        base::UTF16StringBuf currentFileName;
        if (currentFileNamePtr)
            currentFileName = *currentFileNamePtr;
        else if (!dialogSettings.lastSaveFileName.empty())
            currentFileName = dialogSettings.lastSaveFileName;

        // ask for file path
        base::io::AbsolutePath selectedPath;
        const auto nativeHandle = windowNativeHandle(owner);
        if (!IO::GetInstance().showFileSaveDialog(nativeHandle, currentFileName, formatList, selectedPath, dialogSettings))
            return false;

        // extract file name
        if (currentFileNamePtr)
            *currentFileNamePtr = selectedPath.fileNameWithExtensions();
        else
            dialogSettings.lastSaveFileName = selectedPath.fileNameWithExtensions();

        // get the object to save
        const auto object = makeXMLFunc();
        if (!object)
        {
            ui::PostWindowMessage(owner, ui::MessageType::Warning, "XMLSave"_id, "No object to save");
            return false;
        }

        // compile into XML document
        const auto document = base::SaveObjectToXML(object);
        if (!document)
        {
            ui::PostWindowMessage(owner, ui::MessageType::Warning, "XMLSave"_id, base::TempString("Failed to serialize '{}' into XML", object));
            return false;
        }

        // save on disk
        if (!base::xml::SaveDocument(*document, selectedPath))
        {
            ui::PostWindowMessage(owner, ui::MessageType::Warning, "XMLSave"_id, base::TempString("Failed to save XML to '{}'", selectedPath));
            return false;
        }

        // saved
        ui::PostWindowMessage(owner, ui::MessageType::Info, "XMLSave"_id, base::TempString("XML '{}' saved", selectedPath));
        return true;

    }

    bool Editor::saveToXML(ui::IElement* owner, base::StringView<char> category, const base::ObjectPtr& objectPtr, base::UTF16StringBuf* currentFileNamePtr)
    {
        return saveToXML(owner, category, [objectPtr]() { return objectPtr;  });
    }

    base::ObjectPtr Editor::loadFromXML(ui::IElement* owner, base::StringView<char> category, base::SpecificClassType<IObject> expectedObjectClass)
    {
        // use main window when nothing was provided
        if (!owner)
            owner = m_mainWindow;

        // get the open/save dialog settings
        auto& dialogSettings = openSavePersistentData(category);

        // add XML format
        base::InplaceArray<base::io::FileFormat, 1> formatList;
        formatList.emplaceBack("xml", "Extensible Markup Language file");

        // ask for file path
        base::Array<base::io::AbsolutePath> selectedPaths;
        const auto nativeHandle = windowNativeHandle(owner);
        if (!IO::GetInstance().showFileOpenDialog(nativeHandle, false, formatList, selectedPaths, dialogSettings))
            return false;

        // load the document from the file
        const auto& loadPath = selectedPaths.front();
        auto& reporter = base::xml::ILoadingReporter::GetDefault();
        const auto document = base::xml::LoadDocument(reporter, loadPath);
        if (!document)
        {
            ui::PostWindowMessage(owner, ui::MessageType::Warning, "XMLLoad"_id, base::TempString("Failed to load XML from '{}'", loadPath));
            return nullptr;
        }

        // check the class without actual object loading
        const auto rootNode = base::xml::Node(document);
        if (const auto className = rootNode.attribute("class"))
        {
            const auto classType = RTTI::GetInstance().findClass(StringID::Find(className));
            if (!classType || classType->isAbstract() || !classType->is(expectedObjectClass))
            {
                ui::PostWindowMessage(owner, ui::MessageType::Warning, "XMLLoad"_id, base::TempString("Incompatible object '{}' from in XML '{}'", className, loadPath));
                return nullptr;
            }
        }
        else
        {
            ui::PostWindowMessage(owner, ui::MessageType::Warning, "XMLLoad"_id, base::TempString("No object serialized in XML '{}'", loadPath));
            return nullptr;
        }

        // load object
        const auto obj = base::LoadObjectFromXML(document);
        if (!obj)
        {
            ui::PostWindowMessage(owner, ui::MessageType::Warning, "XMLLoad"_id, base::TempString("Failed to deserialize XML from '{}'", loadPath));
            return nullptr;
        }

        // loaded
        ui::PostWindowMessage(owner, ui::MessageType::Info, "XMLLoad"_id, base::TempString("Loaded '{}' from '{}'", obj->cls()->name(), loadPath));
        return obj;
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
