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

class AssetDepotTreeDirectoryItem;
class AssetDepotTreeProjectRoot;
class AssetDepotTreeEngineRoot;
class AssetBrowserTabFiles;
class AssetBrowserResourceListDialog;

//---

/// drag&drop data with file
class EDITOR_COMMON_API AssetBrowserFileDragDrop : public ui::IDragDropData
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowserFileDragDrop, ui::IDragDropData);

public:
    AssetBrowserFileDragDrop(StringView depotPath);

    INLINE const StringBuf& depotPath() const { return m_depotPath; }

private:
    virtual ui::ElementPtr createPreview() const override;

    StringBuf m_depotPath;
};

//---

/// drag&drop data with file
class EDITOR_COMMON_API AssetBrowserDirectoryDragDrop : public ui::IDragDropData
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowserDirectoryDragDrop, ui::IDragDropData);

public:
    AssetBrowserDirectoryDragDrop(StringView depotPath);

    INLINE const StringBuf& depotPath() const { return m_depotPath; }

private:
    virtual ui::ElementPtr createPreview() const override;

    StringBuf m_depotPath;
};

///--

/// assert browser tab
class EDITOR_COMMON_API AssetBrowser : public ui::Window
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowser, ui::Window);

public:
    AssetBrowser();
    virtual ~AssetBrowser();

    //--

    // get selected file
    StringBuf selectedFile() const;

    // show file in the asset browser
    bool showFile(StringView depotPath);

    // show directory in the depot tree and possible also as a file list
    bool showDirectory(StringView path, bool exploreContent);

    //--

    virtual void configLoad(const ui::ConfigBlock& block) override;
    virtual void configSave(const ui::ConfigBlock& block) const override;

    //--

private:
    GlobalEventTable m_depotEvents;

    ui::TreeViewExPtr m_depotTree; // tree view for the depot
    ui::DockContainerPtr m_dockContainer;

    //--

    RefPtr<AssetDepotTreeEngineRoot> m_engineRoot;
    RefPtr<AssetDepotTreeProjectRoot> m_projectRoot;

    AssetDepotTreeDirectoryItem* findDirectory(StringView depotPath, bool autoExpand) const;

    //--

    void navigateToDirectory(StringView depotPath, RefPtr<AssetBrowserTabFiles>* outTab = nullptr);

    void closeDirectoryTab(StringView depotPath);

    virtual void handleExternalCloseRequest() override final;
};

///--

END_BOOMER_NAMESPACE_EX(ed)


