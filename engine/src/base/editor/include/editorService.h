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

    //---

    /// global editor app
    /// NOTE: we may have multiple games, etc but the editor is a singleton
    class BASE_EDITOR_API Editor : public NoCopy
    {
    public:
        Editor();
        ~Editor();

        //--

        /// get editor instance 
        static Editor* GetInstance();

        //--

        INLINE ManagedDepot& managedDepot() const { return *m_managedDepot; }

        INLINE ui::ConfigBlock& config() const { return *m_configRootBlock; }

        INLINE vsc::IVersionControl& versionControl() const { return *m_versionControl; }

        INLINE MainWindow& mainWindow() const { return *m_mainWindow; }

        INLINE AssetBrowser& assetBrowser() const { return *m_assetBrowser; }

        INLINE ui::Renderer& renderer() const { return *m_renderer; }

        INLINE net::TcpMessageServer& messageServer() const { return *m_messageServer; }

        ///---
        
        // start editor application, creates the main window
        bool initialize(ui::Renderer* renderer, const app::CommandLine& cmdLine);

        // update (tick)
        void update();

        ///---

        // get parent window native handle for given ui element (useful for OS interop)
        uint64_t windowNativeHandle(ui::IElement* elem) const;

        ///--

        // get the semi persistent data for the open/save file dialog - mainly the active directory, selected file filter, etc
        io::OpenSavePersistentData& openSavePersistentData(StringView category);

        ///---

        /// force save editor configuration
        void saveConfig();

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
        void scheduleBackgroundJob(IBackgroundJob* runner, bool openUI = false);

        /// show job status dialog
        void showBackgroundJobDialog(IBackgroundJob* job);

        ///---

        // show asset browser window
        void showAssetBrowser(bool focus=true);

        // get selected file
        ManagedFile* selectedFile() const;

        // get the active directory
        ManagedDirectory* selectedDirectory() const;

        // show file in the asset browser
        bool showFile(ManagedFile* filePtr);

        // show directory in the depot tree and possible also as a file list
        bool showDirectory(ManagedDirectory* dir, bool exploreContent);

        //--

        // check if given file can be opened
        bool canOpenFile(ManagedFile* file) const;

        // check if file is edited
        ResourceEditorPtr findFileEditor(ManagedFile* file) const;

        // show editor for given file, returns false if file is not edited
        bool showFileEditor(ManagedFile* file) const;

        // open file for edit
        bool openFileEditor(ManagedFile* file, bool activate=true);

        // close file editor
        bool closeFileEditor(ManagedFile* file, bool force=false);

        // save file in a file editor
        bool saveFileEditor(ManagedFile* file, bool force = false);

        // collect all opened files
        void collectOpenedFiles(Array<ManagedFile*>& outOpenedFiles) const;

        // collect all resource editors
        void collectResourceEditors(Array<ResourceEditorPtr>& outResourceEditors) const;

        //--

        // import files into asset depot
        void importFiles(const ManagedDirectory* currentDirectory, TImportClass resourceClass, const Array<StringBuf>& selectedAssetPaths);

        // reimport already imported files into the depot
        void reimportFiles(const Array<ManagedFileNativeResource*>& files);

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
        BackgroundJobPtr m_activeBackgroundJob;
        Array<BackgroundJobPtr> m_pendingBackgroundJobs;
        BackgroundJobPtr m_pendingBackgroundJobUIRequest;
        BackgroundJobPtr m_pendingBackgroundJobUIOpenedDialogRequest;

        void updateBackgroundJobs();

        //--

        ui::Renderer* m_renderer = nullptr;

        //--
            
        RefPtr<MainWindow> m_mainWindow;
        RefPtr<AssetBrowser> m_assetBrowser;

        Array<RefPtr<IBaseResourceContainerWindow>> m_resourceContainers; // NOTE: contains main window as well

        //--

        HashMap<StringBuf, io::OpenSavePersistentData*> m_openSavePersistentData;

        //--

        void saveAssetsSafeCopy();

        void updateResourceEditors();

        void loadOpenSaveSettings(const ui::ConfigBlock& config);
        void saveOpenSaveSettings(const ui::ConfigBlock& config) const;

        void loadOpenedFiles();
        void saveOpenedFiles() const;

        IBaseResourceContainerWindow* findOrCreateResourceContainer(StringView text);
        IBaseResourceContainerWindow* findResourceContainer(StringView text) const;
    };

    //---

    extern BASE_EDITOR_API Editor* GetEditor();

    //---

} // editor
