/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#pragma once

#include "base/app/include/application.h"
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

        INLINE ui::ConfigBlock& config() const { return *m_configRootBlock; }

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

        // get parent window native handle for given ui element (useful for OS interop)
        uint64_t windowNativeHandle(ui::IElement* elem) const;

        ///--

        // get the semi persistent data for the open/save file dialog - mainly the active directory, selected file filter, etc
        io::OpenSavePersistentData& openSavePersistentData(StringView category);

        ///---

        /// helper function to save object to XML on user's disk, the makeXMLFunc is only called if user actually wants the data to be saved
        bool saveToXML(ui::IElement* owner, StringView category, const std::function<ObjectPtr()>& makeXMLFunc, StringBuf* currentFileName = nullptr);

        /// helper function to save object to XML on user's disk, the makeXMLFunc is only called if user actually wants the data to be saved
        bool saveToXML(ui::IElement* owner, StringView category, const ObjectPtr& objectPtr, StringBuf* currentFileName=nullptr);

        /// helper function to load object from XML on user's disk 
        ObjectPtr loadFromXML(ui::IElement* owner, StringView category, SpecificClassType<IObject> expectedObjectClass);

        /// helper function to load object from XML on user's disk 
        template< typename T >
        INLINE RefPtr<T> loadFromXML(ui::IElement* owner, StringView category) { return rtti_cast<T>(loadFromXML(owner, category, T::GetStaticClass())); }

        ///---

        /// add a generic background runner to the list of runners
        void attachBackgroundJob(IBackgroundJob* runner);

        /// run a background command, usually this will spawn a child process and run the command there
        BackgroundJobPtr runBackgroundCommand(IBackgroundCommand* command);

        ///---

    private:
        UniquePtr<ManagedDepot> m_managedDepot;
        UniquePtr<vsc::IVersionControl> m_versionControl;

        UniquePtr<ui::ConfigFileStorageDataInterface> m_configStorage;
        UniquePtr<ui::ConfigBlock> m_configRootBlock;

        NativeTimePoint m_nextConfigSave;
        NativeTimePoint m_nextAutoSave;

        StringBuf m_configPath;

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

        void loadOpenSaveSettings(const ui::ConfigBlock& config);
        void saveOpenSaveSettings(const ui::ConfigBlock& config) const;

        void updateResourceEditors();
    };

} // editor
