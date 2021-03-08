/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#include "build.h"
#include "editorService.h"
#include "editorMainWindow.h"
#include "editorResourceContainerWindow.h"
#include "editorProgressDialog.h"
#include "editorBackgroundTask.h"

#include "assetBrowser.h"
#include "managedDepot.h"
#include "managedFile.h"
#include "assetFormat.h"
#include "versionControl.h"
#include "resourceEditor.h"

#include "core/io/include/io.h"
#include "core/app/include/commandline.h"
#include "core/resource/include/loader.h"
#include "engine/ui/include/uiRenderer.h"
#include "engine/ui/include/uiElementConfig.h"
#include "core/xml/include/xmlUtils.h"
#include "core/xml/include/xmlDocument.h"
#include "core/xml/include/xmlWrappers.h"
#include "core/io/include/fileFormat.h"
#include "core/net/include/tcpMessageServer.h"
#include "core/net/include/messageConnection.h"
#include "core/net/include/messagePool.h"
#include "engine/ui/include/uiDockLayout.h"
#include "engine/ui/include/uiButton.h"
#include "assetFileImportPrepareDialog.h"
#include "engine/ui/include/uiMessageBox.h"
#include "core/fibers/include/backgroundJob.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

ConfigProperty<double> cvConfigAutoSaveTime("Editor", "ConfigSaveInterval", 15.0f);
ConfigProperty<double> cvDataAutoSaveTime("Editor", "DataAutoSaveInterval", 120.0f);

//---

Editor::Editor()
{}

Editor::~Editor()
{
    saveConfig();

    if (m_assetBrowser)
    {
        m_assetBrowser->requestClose();
        m_assetBrowser.reset();
    }

    if (m_mainWindow)
    {
        m_mainWindow->requestClose();
        m_mainWindow.reset();
    }

    m_renderer = nullptr;
    m_configStorage->saveToFile(m_configPath);

    m_openSavePersistentData.clearPtr();

    m_messageServer.reset();
    m_managedDepot.reset();
    m_versionControl.reset();
    m_configRootBlock.reset();
    m_configStorage.reset();
}

static Editor* GEditor = nullptr;

Editor* Editor::GetInstance()
{
    return GEditor;
}

bool Editor::initialize(ui::Renderer* renderer, const app::CommandLine& cmdLine)
{
    DEBUG_CHECK_RETURN_EX_V(!GEditor, "Editor already created", false);
    DEBUG_CHECK_RETURN_EX_V(renderer, "UI renderer is required for editor", false);
    m_renderer = renderer;

    // we can initialize the managed depot only if we are running from uncooked data
    auto loaderService = GetService<LoadingService>();
    DEBUG_CHECK_RETURN_EX_V(loaderService && loaderService->loader(), "No resource loading service spawned, no editor can be created", false);

    // create configuration
    m_configStorage.create();
    m_configRootBlock.create(m_configStorage.get(), "");

    // start the message service 
    m_messageServer = RefNew<net::TcpMessageServer>();
    if (!m_messageServer->startListening(0))
    {            
        TRACE_ERROR("Unable to start TCP server for communication with background processes");
    }

    // cache format information
    ManagedFileFormatRegistry::GetInstance().cacheFormats();

    // load config data (does not apply the config)
    m_configPath = TempString("{}editor.config.xml", SystemPath(PathCategory::UserConfigDir));
    m_configStorage->loadFromFile(m_configPath);
        
    // load open/save settings
    loadOpenSaveSettings(m_configRootBlock->tag("OpenSaveDialog"));

    // initialize the managed depot
    m_managedDepot = CreateUniquePtr<ManagedDepot>();
    m_managedDepot->populate();
    m_managedDepot->configLoad(m_configRootBlock->tag("Depot"));
    GEditor = this;

    // create version control
    // TODO: add actual integration
    m_versionControl = vsc::CreateLocalStub();

    // create main window
    m_mainWindow = RefNew<MainWindow>();
    m_mainWindow->configLoad(m_configRootBlock->tag("MainWindow"));
    m_resourceContainers.pushBack(m_mainWindow);
    renderer->attachWindow(m_mainWindow);

    // create asset browser
    m_assetBrowser = RefNew<AssetBrowser>();
    m_assetBrowser->configLoad(m_configRootBlock->tag("AssetBrowser"));
    renderer->attachWindow(m_assetBrowser);

    // restore opened files
    loadOpenedFiles();

    // go back to main window
    m_mainWindow->requestActivate();

    // bind active instance
    return true;
}

void Editor::saveConfig()
{
    ScopeTimer timer;

    TRACE_INFO("Saving config...");

    saveOpenedFiles();
    saveOpenSaveSettings(m_configRootBlock->tag("OpenSaveDialogs"));

    m_managedDepot->configSave(m_configRootBlock->tag("Depot"));
    m_mainWindow->configSave(m_configRootBlock->tag("MainWindow"));
    m_assetBrowser->configSave(m_configRootBlock->tag("AssetBrowser"));

    m_configStorage->saveToFile(m_configPath); 
    m_nextConfigSave = NativeTimePoint::Now() + cvConfigAutoSaveTime.get();

    TRACE_INFO("Config saved in {}", timer);
}

void Editor::saveAssetsSafeCopy()
{
    //TRACE_INFO("Auto save...");
    //m_nextConfigSave = NativeTimePoint::Now() + cvDataAutoSaveTime.get();
}

void Editor::update()
{
    if (m_nextConfigSave.reached())
        saveConfig();

    if (m_nextAutoSave.reached())
        saveAssetsSafeCopy();

    // kick the depot changes
    m_managedDepot->update();

    // update the state of background processes
    updateBackgroundJobs();
        
    // update all opened resource editors
    updateResourceEditors();
}

//---

void Editor::showAssetBrowser(bool focus /*= true*/)
{
    m_assetBrowser->requestShow(focus);
}

StringBuf Editor::selectedFile() const
{
    return m_assetBrowser->selectedFile();
}

bool Editor::showFile(StringView filePtr)
{
    if (m_assetBrowser->showFile(filePtr))
    {
        m_assetBrowser->requestShow(true);
        return true;
    }

    return false;
}

bool Editor::showDirectory(StringView dir, bool exploreContent)
{
    if (m_assetBrowser->showDirectory(dir, exploreContent))
    {
        m_assetBrowser->requestShow(true);
        return true;
    }

    return false;
}

//--

ResourceEditorPtr Editor::findFileEditor(StringView depotPath) const
{
    ResourceEditorPtr ret;

    for (const auto& window : m_resourceContainers)
    {
        if (window->iterateEditors([depotPath, &ret](ResourceEditor* editor)
            {
                if (editor->context().physicalDepotPath == depotPath)
                {
                    ret = AddRef(editor);
                    return true;
                }

                return false;
            }))
        {
            return ret;
        }
    }

    return nullptr;
}

bool Editor::showFileEditor(StringView depotPath) const
{
    for (const auto& window : m_resourceContainers)
    {
        if (window->iterateEditors([depotPath, window](ResourceEditor* editor)
            {
                if (editor->context().physicalDepotPath == depotPath)
                {
                    window->requestShow(true);
                    return window->selectEditor(editor);
                }

                return false;
            }))
            return true;
    }

    return false;
}

//--


namespace prv
{
    class ManagedFileOpenerHelper : public ISingleton
    {
        DECLARE_SINGLETON(ManagedFileOpenerHelper);

    public:
        ManagedFileOpenerHelper()
        {
            Array<SpecificClassType<IResourceEditorOpener>> openers;
            RTTI::GetInstance().enumClasses(openers);

            for (auto openerClass : openers)
                if (auto opener = openerClass->createPointer<IResourceEditorOpener>())
                    m_openers.pushBack(opener);
        }

        ResourceEditorPtr createEditor(const ResourceEditorData& data) const
        {
            for (auto* opener : m_openers)
                if (const auto editor = opener->createEditor(data))
                    return editor;

            return nullptr;
        }

    private:
        Array<IResourceEditorOpener*> m_openers;

        virtual void deinit() override
        {
            m_openers.clearPtr();
        }
    };

} // prv

//--

IBaseResourceContainerWindow* Editor::findResourceContainer(StringView text) const
{
    for (const auto& window : m_resourceContainers)
        if (window->tag() == text)
            return window;

    return nullptr;
}

IBaseResourceContainerWindow* Editor::findOrCreateResourceContainer(StringView text)
{
    DEBUG_CHECK_RETURN_EX_V(!text.empty(), "Invalid container tag", nullptr);

    // use existing container
    for (const auto& window : m_resourceContainers)
        if (window->tag() == text)
            return window;

    DEBUG_CHECK_RETURN_EX_V(text != "Main", "Invalid container tag", nullptr);

    // create container window
    auto window = RefNew<FloatingResourceContainerWindow>(text);
    m_resourceContainers.pushBack(window);

    // load the window placement
    window->configLoad(m_configRootBlock->tag("ResourceContainers").tag(window->tag()));
    m_renderer->attachWindow(window);

    return window;
}

bool Editor::openFileEditor(StringView depotPath, bool activate)
{
    // show current editor
    if (showFileEditor(depotPath))
        return true;

    // load resource
    ResourceEditorData loadedData;

    // TODO: load !

    // create best editor for this file
    auto editor = prv::ManagedFileOpenerHelper::GetInstance().createEditor(loadedData);
    if (!editor)
        return false;

    // check for singular editor
    if (auto existingContainer = findResourceContainer(editor->containerTag()))
        if (existingContainer->singularEditorOnly())
            if (!existingContainer->closeContainedFiles())
                return false;

    // initialize the editor, this will load the editor
    // TODO: it's possible to move it to thread if needed
    if (!editor->initialize())
        return false;

    // initialize the additional generic aspects once the normal editor was initialized
    editor->createAspects();

    // load the general editor config
    const auto configBlock = GetEditor()->config().tag("Resources").tag(loadedData.physicalDepotPath);
    editor->configLoad(configBlock);

    // load the name of the resource container tag 
    auto containerTag = editor->containerTag();
    configBlock.read("ContainerTag", containerTag);

    // TODO: "center" container

    // create/get the container and attach resource editor to the container
    auto container = findOrCreateResourceContainer(containerTag);
    container->attachEditor(editor, activate);

    if (activate)
        container->requestShow(activate);

    return true;
}

bool Editor::closeFileEditor(StringView depotPath, bool force)
{
    if (auto editor = findFileEditor(depotPath))
    {
        if (editor->modified() && !force)
        {
            StringBuilder txt;
            txt.appendf("File '{}' is [b][color:#F00]modified[/color][/b].\n \nDo you want to save it or discard the changes?", editor->context().physicalDepotPath);

            ui::MessageBoxSetup setup;
            setup.title("Confirm closing editor");
            setup.message(txt.toString());
            setup.yes().no().cancel().defaultYes().warn();
            setup.caption(ui::MessageButton::Yes, "[img:save] Save");
            setup.caption(ui::MessageButton::No, "[img:delete_black] Discard");
            setup.m_constructiveButton = ui::MessageButton::Yes;
            setup.m_destructiveButton = ui::MessageButton::No;

            const auto ret = ui::ShowMessageBox(editor, setup);
            if (ret == ui::MessageButton::Yes)
            {
                if (!editor->save())
                {
                    ui::PostWindowMessage(editor, ui::MessageType::Error, "FileSave"_id, TempString("Error saving file '{}'", editor->context().physicalDepotPath));
                    return false;
                }
            }
            else if (ret == ui::MessageButton::Cancel)
            {
                return false;
            }
        }

        {
            const auto configBlock = GetEditor()->config().tag("Resources").tag(editor->context().physicalDepotPath);
            editor->configSave(configBlock);
        }

        if (auto* container = editor->findParent<IBaseResourceContainerWindow>())
            container->detachEditor(editor);

        editor->cleanup();
    }

    return true;
}

bool Editor::saveFileEditor(StringView depotPath, bool force /*= false*/)
{
    if (auto editor = findFileEditor(depotPath))
    {
        if (editor->modified() || force)
        {
            if (!editor->save())
            {
                ui::PostWindowMessage(editor, ui::MessageType::Error, "FileSave"_id, TempString("Error saving file '{}'", editor->context().physicalDepotPath));
                return false;
            }
        }
        else
        {
            return true;
        }
    }

    return false;
}

//--

void Editor::importFiles(StringView directoryDepotPath, TImportClass resourceClass, const Array<StringBuf>& selectedAssetPaths)
{
    auto dlg = RefNew<AssetImportPrepareDialog>();
    dlg->addNewImportFiles(directoryDepotPath, resourceClass, selectedAssetPaths);
    dlg->runModal(m_mainWindow);
}

void Editor::reimportFiles(const Array<StringBuf>& fileDepotPaths)
{
    auto dlg = RefNew<AssetImportPrepareDialog>();
    dlg->addReimportFiles(fileDepotPaths);
    dlg->runModal(m_mainWindow);
}

//---

void Editor::scheduleBackgroundJob(IBackgroundTask* job, bool openUI)
{
    if (job)
    {
        TRACE_INFO("Editor: Scheduled background job '{}'", job->description());

        // add command runner to list so we can observe if it's not dead
        {
            auto lock = CreateLock(m_backgroundJobsLock);
            m_pendingBackgroundJobs.pushBack(AddRef(job));
        }

        if (openUI)
            showBackgroundJobDialog(job);
    }
}

void Editor::showBackgroundJobDialog(IBackgroundTask* job)
{
    DEBUG_CHECK_RETURN_EX(job, "No job specified");
    m_pendingBackgroundJobUIRequest = AddRef(job);
}

void Editor::updateBackgroundJobs()
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
                m_activeBackgroundJob = job;
                break;
            }
        }
    }

    // update background job
    if (m_activeBackgroundJob)
    {
        auto status = m_activeBackgroundJob->update();
        if (status != BackgroundTaskStatus::Running)
        {
            m_mainWindow->statusBar()->pushBackgroundJobToHistory(m_activeBackgroundJob, status);
            m_mainWindow->statusBar()->resetBackgroundJobStatus();
            m_activeBackgroundJob.reset();
        }
        else
        {
            BackgroundTaskProgress progress;
            m_activeBackgroundJob->queryProgressInfo(progress);

            m_mainWindow->statusBar()->updateBackgroundJobStatus(m_activeBackgroundJob, progress.currentCount, progress.totalCount, progress.text, false);
        }
    }

    // manage the job details
    if (m_pendingBackgroundJobUIRequest)
    {
        auto job = std::move(m_pendingBackgroundJobUIRequest);
        m_pendingBackgroundJobUIRequest.reset();

        if (!m_pendingBackgroundJobUIOpenedDialogRequest)
        {
            if (auto dialog = job->fetchDetailsDialog())
            {
                m_pendingBackgroundJobUIOpenedDialogRequest = job;

                auto window = RefNew<ui::Window>(ui::WindowFeatureFlagBit::DEFAULT_DIALOG_RESIZABLE, TempString("Details of {}", job->description()));
                auto windowRef = window.get();

                window->attachChild(dialog);
                dialog->expand();

                auto buttons = window->createChild<ui::IElement>();
                buttons->layoutHorizontal();
                buttons->customPadding(5);
                buttons->customHorizontalAligment(ui::ElementHorizontalLayout::Right);

                {
                    auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "[img:cancel] Cancel").get();
                    button->enable(!job->canceled());
                    button->addStyleClass("red"_id);
                    button->bind(ui::EVENT_CLICKED) = [dialog, job, windowRef, button]()
                    {
                        job->requestCancel();
                        button->enable(false);
                    };
                }

                {
                    auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "Close");
                    button->bind(ui::EVENT_CLICKED) = [windowRef]() {
                        windowRef->requestClose();
                    };
                }

                window->runModal(m_mainWindow);

                m_pendingBackgroundJobUIOpenedDialogRequest.reset();
            }
        }
    }
}

//--

void Editor::loadOpenSaveSettings(const ui::ConfigBlock& config)
{

}

void Editor::saveOpenSaveSettings(const ui::ConfigBlock& config) const
{

}

OpenSavePersistentData& Editor::openSavePersistentData(StringView category)
{
    if (!category)
        category = "Generic";

    if (const auto* data = m_openSavePersistentData.find(category))
        return **data;

    auto entry = new OpenSavePersistentData;
    entry->directory = SystemPath(PathCategory::UserDocumentsDir);

    m_openSavePersistentData[StringBuf(category)] = entry;
    return *entry;
}

//--

bool Editor::saveToXML(ui::IElement* owner, StringView category, const std::function<ObjectPtr()>& makeXMLFunc, StringBuf* currentFileNamePtr)
{
    // use main window when nothing was provided
    if (!owner)
        owner = m_mainWindow;

    // get the open/save dialog settings
    auto& dialogSettings = openSavePersistentData(category);

    // add XML format
    InplaceArray<FileFormat, 1> formatList;
    formatList.emplaceBack("xml", "Extensible Markup Language file");

    // current file name
    StringBuf currentFileName;
    if (currentFileNamePtr)
        currentFileName = *currentFileNamePtr;
    else if (!dialogSettings.lastSaveFileName.empty())
        currentFileName = dialogSettings.lastSaveFileName;

    // ask for file path
    StringBuf selectedPath;
    const auto nativeHandle = windowNativeHandle(owner);
    if (!ShowFileSaveDialog(nativeHandle, currentFileName, formatList, selectedPath, dialogSettings))
        return false;

    // extract file name
    if (currentFileNamePtr)
        *currentFileNamePtr = StringBuf(selectedPath.view().fileName());
    else
        dialogSettings.lastSaveFileName = StringBuf(selectedPath.view().fileName());

    // get the object to save
    const auto object = makeXMLFunc();
    if (!object)
    {
        ui::PostWindowMessage(owner, ui::MessageType::Warning, "XMLSave"_id, "No object to save");
        return false;
    }

    // compile into XML document
    const auto document = SaveObjectToXML(object);
    if (!document)
    {
        ui::PostWindowMessage(owner, ui::MessageType::Warning, "XMLSave"_id, TempString("Failed to serialize '{}' into XML", object));
        return false;
    }

    // save on disk
    if (!xml::SaveDocument(*document, selectedPath))
    {
        ui::PostWindowMessage(owner, ui::MessageType::Warning, "XMLSave"_id, TempString("Failed to save XML to '{}'", selectedPath));
        return false;
    }

    // saved
    ui::PostWindowMessage(owner, ui::MessageType::Info, "XMLSave"_id, TempString("XML '{}' saved", selectedPath));
    return true;

}

bool Editor::saveToXML(ui::IElement* owner, StringView category, const ObjectPtr& objectPtr, StringBuf* currentFileNamePtr)
{
    return saveToXML(owner, category, [objectPtr]() { return objectPtr;  });
}

ObjectPtr Editor::loadFromXML(ui::IElement* owner, StringView category, SpecificClassType<IObject> expectedObjectClass)
{
    // use main window when nothing was provided
    if (!owner)
        owner = m_mainWindow;

    // get the open/save dialog settings
    auto& dialogSettings = openSavePersistentData(category);

    // add XML format
    InplaceArray<FileFormat, 1> formatList;
    formatList.emplaceBack("xml", "Extensible Markup Language file");

    // ask for file path
    Array<StringBuf> selectedPaths;
    const auto nativeHandle = windowNativeHandle(owner);
    if (!ShowFileOpenDialog(nativeHandle, false, formatList, selectedPaths, dialogSettings))
        return false;

    // load the document from the file
    const auto& loadPath = selectedPaths.front();
    auto& reporter = xml::ILoadingReporter::GetDefault();
    const auto document = xml::LoadDocument(reporter, loadPath);
    if (!document)
    {
        ui::PostWindowMessage(owner, ui::MessageType::Warning, "XMLLoad"_id, TempString("Failed to load XML from '{}'", loadPath));
        return nullptr;
    }

    // check the class without actual object loading
    const auto rootNode = xml::Node(document);
    if (const auto className = rootNode.attribute("class"))
    {
        const auto classType = RTTI::GetInstance().findClass(StringID::Find(className));
        if (!classType || classType->isAbstract() || !classType->is(expectedObjectClass))
        {
            ui::PostWindowMessage(owner, ui::MessageType::Warning, "XMLLoad"_id, TempString("Incompatible object '{}' from in XML '{}'", className, loadPath));
            return nullptr;
        }
    }
    else
    {
        ui::PostWindowMessage(owner, ui::MessageType::Warning, "XMLLoad"_id, TempString("No object serialized in XML '{}'", loadPath));
        return nullptr;
    }

    // load object
    const auto obj = LoadObjectFromXML(document);
    if (!obj)
    {
        ui::PostWindowMessage(owner, ui::MessageType::Warning, "XMLLoad"_id, TempString("Failed to deserialize XML from '{}'", loadPath));
        return nullptr;
    }

    // loaded
    ui::PostWindowMessage(owner, ui::MessageType::Info, "XMLLoad"_id, TempString("Loaded '{}' from '{}'", obj->cls()->name(), loadPath));
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

void Editor::updateResourceEditors()
{
    for (const auto& container : m_resourceContainers)
        container->iterateAllEditors([this](ResourceEditor* editor) { editor->update(); });

    for (auto i : m_resourceContainers.indexRange().reversed())
    {
        auto window = m_resourceContainers[i];
        if (!window->hasEditors() && !window->singularEditorOnly())
            window->requestClose();

        if (window->requestedClose())
        {
            const auto configGroup = m_configRootBlock->tag("ResourceContainers").tag(window->tag());
            window->configSave(configGroup);

            m_resourceContainers.erase(i);
        }
    }
}

void Editor::loadOpenedFiles()
{
    // open previously opened resources
    auto openedFilePaths = config().tag("Editor").readOrDefault<Array<StringBuf>>("OpenedFiles");
    for (const auto& path : openedFilePaths)
    {
        if (auto file = managedDepot().findManagedFile(path))
        {
            openFileEditor(file, false);
        }
    }

    // focus on resources
}

void Editor::collectResourceEditors(Array<ResourceEditorPtr>& outResourceEditors) const
{
    for (const auto& window : m_resourceContainers)
        window->iterateAllEditors([&outResourceEditors](ResourceEditor* editor) { outResourceEditors.pushBack(AddRef(editor)); });
}

void Editor::collectOpenedFiles(Array<OpenedFile>& outOpenedFiles) const
{
    for (const auto& window : m_resourceContainers)
        window->iterateAllEditors([&outOpenedFiles](ResourceEditor* editor) {
         if (editor->context().physicalDepotPath)
            {
                auto& outEntry = outOpenedFiles.emplaceBack();
                outEntry.data = editor->context().physicalResource;
                outEntry.depotPath = editor->context().physicalDepotPath;
            }
        });
}

void Editor::saveOpenedFiles() const
{
    InplaceArray<ResourceEditorPtr, 100> editors;
    collectResourceEditors(editors);

    // store list of opened files
    {
        HashSet<StringBuf> openedFiles;
        openedFiles.reserve(editors.size());

        for (const auto& editor : editors)
            if (editor->context().physicalDepotPath)
                openedFiles.insert(editor->context().physicalDepotPath);

        m_configRootBlock->tag("Editor").write("OpenedFiles", openedFiles.keys());
    }

    // store config for each editor
    for (const auto& editor : editors)
    {
        if (editor->context().physicalDepotPath)
        {
            const auto configBlock = m_configRootBlock->tag("Resources").tag(editor->context().physicalDepotPath);

            // store where the resource is aligned to
            if (auto* container = editor->findParent<IBaseResourceContainerWindow>())
                configBlock.write("ContainerTag", container->tag());

            editor->configSave(configBlock);
        }
    }

    // store each container window
    for (const auto& window : m_resourceContainers)
    {
        const auto configGroup = m_configRootBlock->tag("ResourceContainers").tag(window->tag());
        window->configSave(configGroup);
    }
}

//--

class ProgressJobAdaptor : public IBackgroundJob
{
public:
    ProgressJobAdaptor(RefPtr<ProgressDialog> dlg, const TLongJobFunc& func)
        : m_dlg(dlg)
        , m_func(func)
    {}

    virtual void cancel() override
    {
        m_dlg->signalCanceled();
    }

    virtual void run() override
    {
        m_func(*m_dlg);
        m_dlg->signalFinished();
    }

public:
    TLongJobFunc m_func;
    RefPtr<ProgressDialog> m_dlg;
};

void Editor::runLongAction(ui::IElement* owner, ProgressDialog* dlg, const TLongJobFunc& func, StringView title, bool canCancel)
{
    RefPtr<ProgressDialog> dlgPtr(AddRef(dlg));

    if (!owner)
    {
        owner = m_mainWindow;
        m_mainWindow->requestShow(true);
    }

    if (!dlgPtr)
        dlgPtr = RefNew<ProgressDialog>(title ? title : "Operation in progress, please wait", canCancel, false);

    auto job = RefNew<ProgressJobAdaptor>(dlgPtr, func);

    RunBackground(job, "BackgroundJob"_id);

    // TODO: wait a small amount of time - 0.1, 0.2s before showing a dialog

    dlgPtr->runModal(owner);
}

//--

Editor* GetEditor()
{
    return Editor::GetInstance();
}

//--

END_BOOMER_NAMESPACE_EX(ed)
