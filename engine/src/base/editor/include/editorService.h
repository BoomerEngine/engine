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

        // get parent window native handle for given ui element (usefull for OS interop)
        uint64_t windowNativeHandle(ui::IElement* elem) const;

        ///--

        // show/open the asset importer window
        bool openAssetImporter();

        // add a file to import list, if no directory is specified file is added to active directory
        bool addImportFiles(const base::Array<base::StringBuf>& assetPaths, const ManagedDirectory* directoryOverride = nullptr);

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

        virtual app::ServiceInitializationResult onInitializeService(const app::CommandLine& cmdLine) override final;
        virtual void onShutdownService() override final;
        virtual void onSyncUpdate() override final;

        void saveConfig();
        void saveAssetsSafeCopy();
    };

} // editor
