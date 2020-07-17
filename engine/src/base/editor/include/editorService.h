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
        base::io::OpenSavePersistentData& openSavePersistentData(base::StringView<char> category);

        // show/open the asset importer window
        bool openAssetImporter();

        // add a file to import list, if no directory is specified file is added to active directory
        bool addImportFiles(const base::Array<base::StringBuf>& assetPaths, base::SpecificClassType<base::res::IResource> importClass, const ManagedDirectory* directoryOverride = nullptr);

        ///---

        /// helper function to save object to XML on user's disk, the makeXMLFunc is only called if user actually wants the data to be saved
        bool saveToXML(ui::IElement* owner, base::StringView<char> category, const std::function<base::ObjectPtr()>& makeXMLFunc, base::UTF16StringBuf* currentFileName = nullptr);

        /// helper function to save object to XML on user's disk, the makeXMLFunc is only called if user actually wants the data to be saved
        bool saveToXML(ui::IElement* owner, base::StringView<char> category, const base::ObjectPtr& objectPtr, base::UTF16StringBuf* currentFileName=nullptr);

        /// helper function to load object from XML on user's disk 
        base::ObjectPtr loadFromXML(ui::IElement* owner, base::StringView<char> category, base::SpecificClassType<IObject> expectedObjectClass);

        /// helper function to load object from XML on user's disk 
        template< typename T >
        INLINE base::RefPtr<T> loadFromXML(ui::IElement* owner, base::StringView<char> category) { return base::rtti_cast<T>(loadFromXML(owner, category, T::GetStaticClass())); }

        ///---

    private:
        UniquePtr<ManagedDepot> m_managedDepot;
        UniquePtr<vsc::IVersionControl> m_versionControl;
        UniquePtr<ConfigRoot> m_config;

        NativeTimePoint m_nextConfigSave;
        NativeTimePoint m_nextAutoSave;

        io::AbsolutePath m_configPath;
            
        ImGuiID m_mainDockId;

        //--

        ui::Renderer* m_renderer = nullptr;

        //--
            
        base::RefPtr<MainWindow> m_mainWindow;
        base::RefPtr<AssetImportWindow> m_assetImporter;

        //--

        base::HashMap<base::StringBuf, base::io::OpenSavePersistentData*> m_openSavePersistentData;

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
