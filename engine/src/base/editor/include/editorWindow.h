/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#pragma once

#include "base/ui/include/uiWindow.h"
#include "base/ui/include/uiDockContainer.h"
#include "base/ui/include/uiDockPanel.h"

namespace ed
{
    ///---

    class AssetBrowser;

    typedef HashSet<const ManagedFile*> TFileSet;
    typedef SpecificClassType<res::IResource> TImportClass;

    /// main editor window 
    class BASE_EDITOR_API MainWindow : public ui::Window
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MainWindow, ui::Window);

    public:
        MainWindow();
        virtual ~MainWindow();

        ui::DockLayoutNode& layout();

        virtual void configLoad(const ui::ConfigBlock& block) override;
        virtual void configSave(const ui::ConfigBlock& block) const override;

        ManagedFile* selectedFile() const;
        ManagedDirectory* selectedDirectory() const;

        bool selectFile(ManagedFile* file);
        bool selectDirectory(ManagedDirectory* dir, bool exploreContent);

        void addNewImportFiles(const ManagedDirectory* currentDirectory, TImportClass resourceClass, const Array<StringBuf>& selectedAssetPaths);
        void addReimportFiles(const Array<ManagedFileNativeResource*>& files);
        void addReimportFile(ManagedFileNativeResource* file, const res::ResourceConfigurationPtr& reimportConfiguration, bool quickstart);

        bool iterateMainTabs(const std::function<bool(ui::DockPanel * panel)>& enumFunc, ui::DockPanelIterationMode mode = ui::DockPanelIterationMode::All) const;
        void attachMainTab(ui::DockPanel* mainTab, bool focus=true);
        void detachMainTab(ui::DockPanel* mainTab);
        bool activateMainTab(ui::DockPanel* mainTab);

    protected:
        virtual void handleExternalCloseRequest() override;
        virtual void queryInitialPlacementSetup(ui::WindowInitialPlacementSetup& outSetup) const override;

        //--

        ui::DockContainerPtr m_dockArea;

        RefPtr<AssetBrowser> m_assetBrowserTab;
        RefPtr<AssetImportPrepareTab> m_assetImportPrepareTab;
        RefPtr<AssetImportMainTab> m_assetImportProcessTab;
    };

    ///---

} // editor

