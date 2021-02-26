/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

#include "engine/ui/include/uiDragDrop.h"
#include "engine/ui/include/uiWindow.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

///--

class AssetDepotTreeModel;
class AssetBrowserTabFiles;
class AssetBrowserResourceListDialog;

//---

/// drag&drop data with file
class EDITOR_COMMON_API AssetBrowserFileDragDrop : public ui::IDragDropData
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
class EDITOR_COMMON_API AssetBrowserDirectoryDragDrop : public ui::IDragDropData
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
class EDITOR_COMMON_API AssetBrowser : public ui::Window
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowser, ui::Window);

public:
    AssetBrowser(ManagedDepot* depot);
    virtual ~AssetBrowser();

    //--

    // get selected file
    ManagedFile* selectedFile() const;

    // get the active directory
    ManagedDirectory* selectedDirectory() const;

    // show file in the asset browser
    bool showFile(ManagedFile* filePtr);

    // show directory in the depot tree and possible also as a file list
    bool showDirectory(ManagedDirectory* dir, bool exploreContent);

    //--

    virtual void configLoad(const ui::ConfigBlock& block) override;
    virtual void configSave(const ui::ConfigBlock& block) const override;

    //--

private:
    ManagedDepot* m_depot; // the managed depot

    GlobalEventTable m_depotEvents;

    RefPtr<AssetDepotTreeModel> m_depotTreeModel; // tree model for the depot

    ui::TreeViewPtr m_depotTree; // tree view for the depot
    ui::DockContainerPtr m_dockContainer;

    void navigateToDirectory(ManagedDirectory* dir);
    void navigateToDirectory(ManagedDirectory* dir, RefPtr<AssetBrowserTabFiles>& outTab);

    void closeDirectoryTab(ManagedDirectory* dir);

    virtual void handleExternalCloseRequest() override final;
};

///--

END_BOOMER_NAMESPACE_EX(ed)


