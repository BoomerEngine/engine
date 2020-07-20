/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#pragma once

#include "base/app/include/application.h"
#include "base/io/include/absolutePath.h"
#include "base/system/include/task.h"
#include "base/io/include/ioSystem.h"

namespace ed
{

    /// global editor app
    /// NOTE: we may have multiple games, etc but the editor is a singleton
    class BASE_EDITOR_API Editor : public app::ILocalService
    {
        RTTI_DECLARE_VIRTUAL_CLASS(Editor, app::ILocalService);

    public:
        Editor();
        ~Editor();

        //--

        INLINE ManagedDepot& managedDepot() const { return *m_managedDepot; }

        INLINE ConfigGroup& config() const { return *(ConfigGroup*)m_config.get(); }

        INLINE vsc::IVersionControl& versionControl() const { return *m_versionControl; }

        INLINE MainWindow& mainWindow() const { return *m_mainWindow; }

        INLINE ui::Renderer& renderer() const { return *m_renderer; }

        INLINE net::TcpMessageServer& messageServer() const { return *m_messageServer; }

        ///---
        
        // start editor application, creates the main window
        bool start(ui::Renderer* renderer);

        // stop, close all windows
        void stop();

        ///---

        // can given file format be opened ?
        bool canOpenFile(const ManagedFileFormat& format) const;

        // open editor(s) for given file
        bool openFile(ManagedFile* file);

        // select file in the asset browser
        bool selectFile(ManagedFile* file);

        // get selected depot file
        ManagedFile* selectedFile() const;

        // get selected directory
        ManagedDirectory* selectedDirectory() const;

        ///---

        // get parent window native handle for given ui element (useful for OS interop)
        uint64_t windowNativeHandle(ui::IElement* elem) const;

        ///--

        // get the semi persistent data for the open/save file dialog - mainly the active directory, selected file filter, etc
        io::OpenSavePersistentData& openSavePersistentData(StringView<char> category);

        // add a file to import list, if no directory is specified file is added to active directory
        bool addImportFiles(const Array<StringBuf>& assetPaths, SpecificClassType<res::IResource> importClass, const ManagedDirectory* directoryOverride = nullptr, bool showTab = true);

        // add existing files to reimport list
        bool addReimportFiles(const Array<ManagedFileNativeResource*>& filesToReimport, bool showTab = true);

        // add existing file to reimport list with a new config
        bool addReimportFile(ManagedFileNativeResource* fileToReimport, const res::ResourceConfigurationPtr& config, bool showTab = true);

        ///---

        /// helper function to save object to XML on user's disk, the makeXMLFunc is only called if user actually wants the data to be saved
        bool saveToXML(ui::IElement* owner, StringView<char> category, const std::function<ObjectPtr()>& makeXMLFunc, UTF16StringBuf* currentFileName = nullptr);

        /// helper function to save object to XML on user's disk, the makeXMLFunc is only called if user actually wants the data to be saved
        bool saveToXML(ui::IElement* owner, StringView<char> category, const ObjectPtr& objectPtr, UTF16StringBuf* currentFileName=nullptr);

        /// helper function to load object from XML on user's disk 
        ObjectPtr loadFromXML(ui::IElement* owner, StringView<char> category, SpecificClassType<IObject> expectedObjectClass);

        /// helper function to load object from XML on user's disk 
        template< typename T >
        INLINE RefPtr<T> loadFromXML(ui::IElement* owner, StringView<char> category) { return rtti_cast<T>(loadFromXML(owner, category, T::GetStaticClass())); }

        ///---

        /// add a generic background runner to the list of runners
        void attachBackgroundJob(IBackgroundJob* runner);

        /// run a background command, usually this will spawn a child process and run the command there
        BackgroundJobPtr runBackgroundCommand(IBackgroundCommand* command);

        ///---

    private:
        UniquePtr<ManagedDepot> m_managedDepot;
        UniquePtr<vsc::IVersionControl> m_versionControl;
        UniquePtr<ConfigRoot> m_config;

        NativeTimePoint m_nextConfigSave;
        NativeTimePoint m_nextAutoSave;

        io::AbsolutePath m_configPath;

        //--

        net::TcpMessageServerPtr m_messageServer;

        //--

        Mutex m_backgroundJobsLock;
        Array<BackgroundJobPtr> m_backgroundJobs;

        Array<BackgroundCommandWeakPtr> m_backgroundCommandsWithMissingConnections;
        Array<BackgroundJobUnclaimedConnectionPtr> m_backgroundJobsUnclaimedConnections;

        void updateBackgroundJobs();

        //--
            
        ImGuiID m_mainDockId;

        //--

        ui::Renderer* m_renderer = nullptr;

        //--
            
        RefPtr<MainWindow> m_mainWindow;
        
        //--

        HashMap<StringBuf, io::OpenSavePersistentData*> m_openSavePersistentData;

        //--

        virtual app::ServiceInitializationResult onInitializeService(const app::CommandLine& cmdLine) override final;
        virtual void onShutdownService() override final;
        virtual void onSyncUpdate() override final;

        void saveConfig();
        void saveAssetsSafeCopy();

        void loadOpenSaveSettings(const ConfigGroup& config);
        void saveOpenSaveSettings(ConfigGroup& config) const;
    };

} // editor
