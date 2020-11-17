/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#include "build.h"
#include "editorService.h"
#include "editorWindow.h"

#include "backgroundCommand.h"
#include "backgroundCommandLocalRunner.h"

#include "managedDepot.h"
#include "managedFile.h"
#include "managedFileFormat.h"
#include "versionControl.h"
#include "resourceEditor.h"

#include "base/io/include/ioSystem.h"
#include "base/app/include/commandline.h"
#include "base/resource_compiler/include/depotStructure.h"
#include "base/resource/include/resourceLoadingService.h"
#include "base/ui/include/uiRenderer.h"
#include "base/ui/include/uiElementConfig.h"
#include "base/xml/include/xmlUtils.h"
#include "base/xml/include/xmlDocument.h"
#include "base/xml/include/xmlWrappers.h"
#include "base/io/include/fileFormat.h"
#include "base/net/include/tcpMessageServer.h"
#include "base/net/include/messageConnection.h"
#include "base/net/include/messagePool.h"
#include "base/ui/include/uiDockLayout.h"

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
        m_configStorage.create();
        m_configRootBlock.create(m_configStorage.get(), "");

        // start the message service 
        m_messageServer = RefNew<net::TcpMessageServer>();
        if (!m_messageServer->startListening(0))
        {
            TRACE_ERROR("Unable to start TCP server for communication with background processes");
            return app::ServiceInitializationResult::FatalError;
        }

        // cache format information
        ManagedFileFormatRegistry::GetInstance().cacheFormats();

        // load config data (does not apply the config)
        m_configPath = TempString("{}editor.ini", base::io::SystemPath(io::PathCategory::UserConfigDir));
        m_configStorage->loadFromFile(m_configPath);
        
        // load open/save settings
        loadOpenSaveSettings(m_configRootBlock->tag("OpenSaveDialog"));

        // initialize the managed depot
        m_managedDepot = CreateUniquePtr<ManagedDepot>(*depot);
        m_managedDepot->populate();
        m_managedDepot->configLoad(m_configRootBlock->tag("Depot"));

        // create version control
        // TODO: add actual integration
        m_versionControl = vsc::CreateLocalStub();

        // we are initialized
        return app::ServiceInitializationResult::Finished;
    }

    void Editor::onShutdownService()
    {
        m_messageServer.reset();
        m_managedDepot.reset();
        m_versionControl.reset();
        m_configRootBlock.reset();
        m_configStorage.reset();
    }

    bool Editor::start(ui::Renderer* renderer)
    {
        DEBUG_CHECK(renderer);
        m_renderer = renderer;

        if (!m_mainWindow)
        {
            m_mainWindow = RefNew<MainWindow>();
            m_mainWindow->configLoad(m_configRootBlock->tag("Editor"));
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
        m_configStorage->saveToFile(m_configPath);

        m_openSavePersistentData.clearPtr();
    }

    void Editor::saveConfig()
    {
        TRACE_INFO("Saving config...");

        if (m_managedDepot)
            m_managedDepot->configSave(m_configRootBlock->tag("Depot"));

        if (m_mainWindow)
            m_mainWindow->configSave(m_configRootBlock->tag("Editor"));

        saveOpenSaveSettings(m_configRootBlock->tag("OpenSaveDialogs"));

        // save
        m_configStorage->saveToFile(m_configPath); 
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

        // kick the depot changes
        m_managedDepot->update();

        // update the state of background processes
        updateBackgroundJobs();
        
        // update all opened resource editors
        updateResourceEditors();
    }

    //---

    static BackgroundJobPtr CreateCommandRunner(const app::CommandLine& cmdline, IBackgroundCommand* command)
    {
        return BackgroundCommandRunnerLocal::Run(cmdline, command);
    }

    void Editor::attachBackgroundJob(IBackgroundJob* job)
    {
        if (job)
        {
            TRACE_INFO("Editor: Started background job '{}'", job->description());

            // add command runner to list so we can observe if it's not dead
            {
                auto lock = CreateLock(m_backgroundJobsLock);
                m_backgroundJobs.pushBack(AddRef(job));
            }

            ui::PostWindowMessage(m_mainWindow, ui::MessageType::Info, "BackgroundJob"_id, TempString("Background job {} started", job->description()));
        }
    }

    BackgroundJobPtr Editor::runBackgroundCommand(IBackgroundCommand* command)
    {
        // no command
        if (!command)
            return nullptr;

        // gather the required commandline parameters
        app::CommandLine commandline;
        if (!command->configure(commandline))
        {
            TRACE_WARNING("Editor: Command '{}' failed to configure", command->name());
            return nullptr;
        }

        // insert the command name
        commandline.addCommand(command->name());

        // add the information about message server to the commandline so the running commands will be able to communicate with the editor
        if (true)
        {
            const auto localServerAddress = socket::Address::Local4(m_messageServer->listeningAddress().port());
            commandline.param("messageServer", TempString("{}", localServerAddress));
            commandline.param("messageConnectionKey", command->connectionKey());
            commandline.param("messageStartupTimestamp", TempString("{}", NativeTimePoint::Now().rawValue()));
        }

        // create command runner
        auto runner = CreateCommandRunner(commandline, command);
        if (!runner)
        {
            TRACE_WARNING("Editor: Command '{}' failed to start", command->name());
            return nullptr;
        }

        // remember background command that require network connections
        m_backgroundCommandsWithMissingConnections.pushBack(command);

        // keep the command runner around for the duration of the command execution
        attachBackgroundJob(runner);
        return runner;
    }

    class BackgroundJobUnclaimedConnection : public IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(BackgroundJobUnclaimedConnection, IObject);

    public:
        net::MessageConnectionPtr m_connection;
        NativeTimePoint m_expirationTime;
        StringBuf m_receivedConnectionKey;

        BackgroundJobUnclaimedConnection(const net::MessageConnectionPtr& connection)
            : m_connection(connection)
        {
            m_expirationTime = NativeTimePoint::Now() + 60.0;
        }

        bool update()
        {
            while (auto message = m_connection->pullNextMessage())
            {
                message->dispatch(this, m_connection);

                if (!m_receivedConnectionKey.empty())
                    return false;
            }

            if (m_expirationTime.reached())
                return false;

            return true; // keep going
        }

        void handleHelloMessage(const app::CommandHelloMessage& msg)
        {
            const auto diff = NativeTimePoint(msg.startupTimestamp).timeTillNow().toSeconds();
            TRACE_INFO("Editor: Got remote connection key '{}' from '{}', took {} to start", msg.connectionKey, m_connection->remoteAddress(), TimeInterval(diff));

            m_receivedConnectionKey = msg.connectionKey;

            app::CommandHelloResponseMessage response;
            response.timestampSentBack = msg.localTimestamp;
            m_connection->send(response);
        }
    };

    RTTI_BEGIN_TYPE_NATIVE_CLASS(BackgroundJobUnclaimedConnection);
        RTTI_FUNCTION_SIMPLE(handleHelloMessage);
    RTTI_END_TYPE();

    void Editor::updateBackgroundJobs()
    {
        auto lock = CreateLock(m_backgroundJobsLock);

        // TODO: handle dead server case

        // check for new connections
        while (auto newConnection = m_messageServer->pullNextAcceptedConnection())
        {
            TRACE_INFO("Editor: Got new connection to message server from '{}'", newConnection->remoteAddress());

            auto unclaimedWrapper = base::RefNew<BackgroundJobUnclaimedConnection>(newConnection);
            m_backgroundJobsUnclaimedConnections.pushBack(unclaimedWrapper);
        }

        // update the unclaimed connections - wait for the "hello" message
        for (auto index : m_backgroundJobsUnclaimedConnections.indexRange().reversed())
        {
            const auto& runner = m_backgroundJobsUnclaimedConnections[index];

            if (!runner->update())
            {
                if (runner->m_receivedConnectionKey)
                {
                    bool commandFound = false;
                    for (auto& ptr : m_backgroundCommandsWithMissingConnections)
                    {
                        if (auto command = ptr.lock())
                        {
                            if (command->connectionKey() == runner->m_receivedConnectionKey)
                            {
                                command->confirmed(runner->m_connection);
                                m_backgroundCommandsWithMissingConnections.remove(ptr);
                                commandFound = true;
                            }
                        }
                    }

                    if (!commandFound)
                    {
                        TRACE_WARNING("Editor: No active background command with connection key '{}'", runner->m_receivedConnectionKey);
                        runner->m_connection->close();
                    }
                }
                else
                {
                    TRACE_WARNING("Editor: Message connect '{}' did not follow up with HelloMessage, closing", runner->m_connection->remoteAddress());
                    runner->m_connection->close();
                }

                // no more update needed
                m_backgroundJobsUnclaimedConnections.eraseUnordered(index);
            }
        }

        // update the jobs itself
        for (auto index : m_backgroundJobs.indexRange().reversed())
        {
            const auto& runner = m_backgroundJobs[index];
            if (!runner->update())
            {
                const auto exitCode = runner->exitCode();
                if (exitCode == 0)
                    ui::PostWindowMessage(m_mainWindow, ui::MessageType::Info, "BackgroundJob"_id, TempString("Background job {} finished", runner->description()));
                else
                    ui::PostWindowMessage(m_mainWindow, ui::MessageType::Warning, "BackgroundJob"_id, TempString("Background job {} failed with exit code {}", runner->description(), exitCode));

                m_backgroundJobs.eraseUnordered(index);
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

    io::OpenSavePersistentData& Editor::openSavePersistentData(StringView category)
    {
        if (!category)
            category = "Generic";

        if (const auto* data = m_openSavePersistentData.find(category))
            return **data;

        auto entry = MemNew(io::OpenSavePersistentData).ptr;
        entry->directory = base::io::SystemPath(io::PathCategory::UserDocumentsDir);

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
        InplaceArray<io::FileFormat, 1> formatList;
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
        if (!base::io::ShowFileSaveDialog(nativeHandle, currentFileName, formatList, selectedPath, dialogSettings))
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
        InplaceArray<io::FileFormat, 1> formatList;
        formatList.emplaceBack("xml", "Extensible Markup Language file");

        // ask for file path
        Array<StringBuf> selectedPaths;
        const auto nativeHandle = windowNativeHandle(owner);
        if (!base::io::ShowFileOpenDialog(nativeHandle, false, formatList, selectedPaths, dialogSettings))
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
        m_mainWindow->layout().iteratePanels([this](ui::DockPanel* panel) -> bool
            {
                if (auto* resourceEditor = rtti_cast<ResourceEditor>(panel))
                    resourceEditor->update();

                return false;
            }, ui::DockPanelIterationMode::All);
    }

    //--

} // editor
