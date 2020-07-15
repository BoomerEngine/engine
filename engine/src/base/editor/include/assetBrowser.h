/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

#include "base/ui/include/uiDragDrop.h"

namespace ed
{

    ///--

    class AssetDepotTreeModel;
    class AssetBrowserTabFiles;
    class AssetBrowserResourceListDialog;

    //--

    struct BASE_EDITOR_API AssetItemList : public base::NoCopy
    {
        Array<ManagedFile*> files;
        Array<ManagedDirectory*> dirs;

        INLINE bool empty() const { return files.empty() && dirs.empty(); }

        void clear();
        void collect(ManagedItem* item, bool recrusive = true);
        void collectDir(ManagedDirectory* dir, bool recrusive = true);
        void collectFile(ManagedFile* file);

    private:
        HashSet<ManagedFile*> fileSet;
        HashSet<ManagedDirectory*> dirSet;
    };

    //---

    /// drag&drop data with file
    class BASE_EDITOR_API AssetBrowserFileDragDrop : public ui::IDragDropData
    {
        RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowserFileDragDrop, ui::IDragDropData);

    public:
        AssetBrowserFileDragDrop(ManagedFile* file);

        INLINE ManagedFile* file() const { return m_file; }

    private:
        virtual ui::ElementPtr createPreview() const override;

        ManagedFile* m_file;
    };

    //---

    /// drag&drop data with file
    class BASE_EDITOR_API AssetBrowserDirectoryDragDrop : public ui::IDragDropData
    {
        RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowserDirectoryDragDrop, ui::IDragDropData);

    public:
        AssetBrowserDirectoryDragDrop(ManagedDirectory* dir);

        INLINE ManagedDirectory* directory() const { return m_dir; }

    private:
        virtual ui::ElementPtr createPreview() const override;

        ManagedDirectory* m_dir;
    };

    ///--

    /// assert browser tab
    class BASE_EDITOR_API AssetBrowser : public ui::DockPanel, public IManagedDepotListener
    {
        RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowser, ui::DockPanel);

    public:
        AssetBrowser(ManagedDepot* depot);
        virtual ~AssetBrowser();

        //--

        void loadConfig(const ConfigGroup& config);
        void saveConfig(ConfigGroup config) const;

        //--

        // get selected file
        ManagedFile* selectedFile() const;

        // get the active directory
        ManagedDirectory* selectedDirectory() const;

        // show file in the asset browser
        void showFile(ManagedFile* filePtr);

        // show directory in the depot tree and possible also as a file list
        void showDirectory(ManagedDirectory* dir, bool exploreContent);

        //--

    private:
        ManagedDepot* m_depot; // the managed depot

        base::RefPtr<AssetDepotTreeModel> m_depotTreeModel; // tree model for the depot

        ui::TreeViewPtr m_depotTree; // tree view for the depot
        ui::DockContainerPtr m_dockContainer;

        //base::RefWeakPtr<AssetBrowserResourceListDialog> m_resourceListDialog;

        void navigateToDirectory(ManagedDirectory* dir);
        void navigateToDirectory(ManagedDirectory* dir, base::RefPtr<AssetBrowserTabFiles>& outTab);

        //--

        void closeDirectoryTab(ManagedDirectory* dir);

        //--

        virtual void managedDepotEvent(ManagedItem* item, ManagedDepotEvent eventType) override final;
    };

    ///--

} // ed

