/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

#include "managedDepot.h"

#include "engine/ui/include/uiTreeViewEx.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

class AssetDepotTreeDirectoryItem : public ui::ITreeItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetDepotTreeDirectoryItem, ui::ITreeItem);

public:
    AssetDepotTreeDirectoryItem(StringView depotPath, StringView name);

    INLINE const StringBuf& name() const { return m_name; }
    INLINE const StringBuf& depotPath() const { return m_depotPath; }

    AssetDepotTreeDirectoryItem* findChild(StringView name, bool autoExpand=true);

protected:
    StringBuf m_depotPath;
    StringBuf m_name;

    void checkIfSubFoldersExist();
    bool m_hasChildren = false;

    //--

    virtual StringBuf queryTooltipString() const override;

    //--

    virtual ui::DragDropDataPtr queryDragDropData(const input::BaseKeyFlags& keys, const ui::Position& position) const override;
    virtual ui::DragDropHandlerPtr handleDragDrop(const ui::DragDropDataPtr& data, const ui::Position& entryPosition) override;
    virtual void handleDragDropGenericCompletion(const ui::DragDropDataPtr& data, const ui::Position& entryPosition) override;

    //--

    virtual bool handleItemContextMenu(const ui::CollectionItems& items, ui::PopupPtr& outPopup) const override;
    virtual bool handleItemFilter(const ui::SearchPattern& filter) const override;
    virtual bool handleItemSort(const ui::ICollectionItem* other, int colIndex) const override;

    virtual bool handleItemCanExpand() const override;
    virtual void handleItemExpand() override;
    virtual void handleItemCollapse() override;

    virtual void updateButtonState() override;

    //--

    ui::TextLabelPtr m_label;

    virtual void updateLabel();
};

//--

class AssetDepotTreeEngineRoot : public AssetDepotTreeDirectoryItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetDepotTreeEngineRoot, AssetDepotTreeDirectoryItem);

public:
    AssetDepotTreeEngineRoot();

protected:
    virtual void updateLabel();
};

//--

class AssetDepotTreeProjectRoot : public AssetDepotTreeDirectoryItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetDepotTreeProjectRoot, AssetDepotTreeDirectoryItem);

public:
    AssetDepotTreeProjectRoot();

protected:
    virtual void updateLabel();
};

//--

END_BOOMER_NAMESPACE_EX(ed)
