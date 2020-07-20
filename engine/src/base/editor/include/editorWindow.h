/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#pragma once

#include "editorConfig.h"

#include "base/ui/include/uiWindow.h"
#include "base/ui/include/uiDockContainer.h"
#include "base/ui/include/uiDockPanel.h"

namespace ed
{
    ///---

    class AssetBrowser;
    class AssetImportWindow;
    class ResourceEditor;
    class IResourceEditorOpener;

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

        void loadConfig(const ConfigGroup& config);
        void saveConfig(ConfigGroup& config) const;

        ManagedFile* selectedFile() const;
        ManagedDirectory* selectedDirectory() const;

        bool canOpenFile(const ManagedFileFormat& format) const;
        bool selectFile(ManagedFile* file);
        bool openFile(ManagedFile* file);
        bool saveFile(ManagedFile* file);

        bool saveFiles(const TFileSet& files);

        void collectOpenedFiles(Array<ManagedFile*>& outFiles) const;
        void requestEditorClose(const Array<ResourceEditor*>& editors);

        void addNewImportFiles(const ManagedDirectory* currentDirectory, TImportClass resourceClass, const Array<StringBuf>& selectedAssetPaths);
        void addReimportFiles(const Array<ManagedFileNativeResource*>& files);
        void addReimportFile(ManagedFileNativeResource* file, const res::ResourceConfigurationPtr& reimportConfiguration);

    protected:
        virtual void handleExternalCloseRequest() override;
        virtual void queryInitialPlacementSetup(ui::WindowInitialPlacementSetup& outSetup) const override;

        //--

        //--

        ui::DockContainerPtr m_dockArea;

        RefPtr<AssetBrowser> m_assetBrowserTab;
        RefPtr<AssetImportPrepareTab> m_assetImportPrepareTab;
        RefPtr<AssetImportMainTab> m_assetImportProcessTab;

        Array<IResourceEditorOpener*> m_editorOpeners;
    };

    ///---

} // editor

