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
    class ResourceEditor;
    class IResourceEditorOpener;

    /// main editor window 
    class BASE_EDITOR_API MainWindow : public ui::Window
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MainWindow, ui::Window);

    public:
        MainWindow();
        virtual ~MainWindow();

        void loadConfig(const ConfigGroup& config);
        void saveConfig(ConfigGroup& config) const;

        ManagedFile* selectedFile() const;

        bool canOpenFile(const ManagedFileFormat& format) const;
        bool selectFile(ManagedFile* file);
        bool openFile(ManagedFile* file);
        bool saveFile(ManagedFile* file);

        void collectOpenedFiles(base::Array<ManagedFile*>& outFiles) const;
        void requestEditorClose(const base::Array<ResourceEditor*>& editors);

    protected:
        virtual void handleExternalCloseRequest() override;
        virtual void queryInitialPlacementSetup(ui::WindowInitialPlacementSetup& outSetup) const override;

        ui::DockContainerPtr m_dockArea;

        base::RefPtr<AssetBrowser> m_assetBrowser;
        //base::Array<base::RefPtr<ResourceEditor>> m_editors;
        base::Array<IResourceEditorOpener*> m_editorOpeners;
    };

    ///---

} // editor

