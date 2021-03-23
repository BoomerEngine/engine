/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

#include "engine/ui/include/uiTreeViewEx.h"
#include "engine/ui/include/uiDockPanel.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

class AssetBrowserService;

DECLARE_UI_EVENT(EVENT_DEPOT_ACTIVE_DIRECTORY_CHANGED, StringBuf)

//--

// item in the tree
class AssetBrowserTreeDirectoryItem : public ui::ITreeItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowserTreeDirectoryItem, ui::ITreeItem);

public:
    AssetBrowserTreeDirectoryItem(StringView depotPath, StringView name);

    INLINE const StringBuf& name() const { return m_name; }
    INLINE const StringBuf& depotPath() const { return m_depotPath; }

    AssetBrowserTreeDirectoryItem* findChild(StringView name, bool autoExpand=true);
    
    virtual void updateLabel();

protected:
    StringBuf m_depotPath;
    StringBuf m_name;

    void checkIfSubFoldersExist();
    bool m_hasChildren = false;

    //--

    virtual StringBuf queryTooltipString() const override;

    //--

    virtual ui::DragDropDataPtr queryDragDropData(const BaseKeyFlags& keys, const ui::Position& position) const override;
    virtual ui::DragDropHandlerPtr handleDragDrop(const ui::DragDropDataPtr& data, const ui::Position& entryPosition) override;
    virtual void handleDragDropGenericCompletion(const ui::DragDropDataPtr& data, const ui::Position& entryPosition) override;

    //--

    virtual bool handleItemContextMenu(ui::ICollectionView* view, const ui::CollectionItems& items, const ui::Position& pos, InputKeyMask controlKeys) override;
    virtual bool handleItemFilter(const ui::ICollectionView* view, const ui::SearchPattern& filter) const override;
    virtual void handleItemSort(const ui::ICollectionView* view, int colIndex, SortingData& outData) const override;

    virtual bool handleItemCanExpand() const override;
    virtual void handleItemExpand() override;
    virtual void handleItemCollapse() override;

    virtual void updateButtonState() override;

    //--

    ui::TextLabelPtr m_label;
};

//--

class AssetBrowserTreeEngineRoot : public AssetBrowserTreeDirectoryItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowserTreeEngineRoot, AssetBrowserTreeDirectoryItem);

public:
    AssetBrowserTreeEngineRoot();

protected:
    virtual void updateLabel();
};

//--

class AssetBrowserTreeProjectRoot : public AssetBrowserTreeDirectoryItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowserTreeProjectRoot, AssetBrowserTreeDirectoryItem);

public:
    AssetBrowserTreeProjectRoot();

protected:
    virtual void updateLabel();
};

//--

class AssetBrowserTreePanel : public ui::DockPanel
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowserTreePanel, ui::DockPanel);

public:
    AssetBrowserTreePanel();

    //--

    StringBuf directory();

    void directory(StringView directoryDepotPath);

    //--

    void configLoad(const ui::ConfigBlock& block);
    void configSave(const ui::ConfigBlock& block) const;

    //--

private:
    //--

    AssetBrowserService* m_as = nullptr;

    ui::TreeViewExPtr m_depotTree; // tree view for the depot

    //--

    RefPtr<AssetBrowserTreeEngineRoot> m_engineRoot;
    RefPtr<AssetBrowserTreeProjectRoot> m_projectRoot;

    AssetBrowserTreeDirectoryItem* findDirectory(StringView depotPath, bool autoExpand) const;

    //--

    void cmdNewDirectory();
    void cmdSyncDirectory();
    void cmdDelete();
    void cmdCopy();
    void cmdCut();
    void cmdPaste();

    //---

    GlobalEventTable m_depotEvents;

    void bindEvents();

    void handleDepotDirectoryAdded(StringView path);
    void handleDepotDirectoryRemoved(StringView path);
    void handleDepotDirectoryUpdated(StringView path);

    //---
};

//--

END_BOOMER_NAMESPACE_EX(ed)
