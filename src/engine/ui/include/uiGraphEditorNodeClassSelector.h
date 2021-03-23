/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: graph #]
***/

#pragma once

#include "uiEventFunction.h"
#include "uiWindowPopup.h"
#include "uiSimpleListModel.h"

#include "core/graph/include/graphContainer.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

///----

// entry in the block class list model
struct BlockClassListModelEntry
{
    graph::SupprotedBlockConnectionInfo info;
    StringBuf dataString;
    StringBuf displayString;
};

// data model for listing block classes
class BlockClassListModel : public SimpleTypedListModel<BlockClassListModelEntry>
{
public:
    BlockClassListModel(const graph::Container& filter, const graph::Socket* compatibleSocket);

private:
    virtual StringBuf content(const BlockClassListModelEntry& data, int colIndex = 0) const override;
    virtual bool compare(const BlockClassListModelEntry& a, const BlockClassListModelEntry& b, int colIndex) const override;
    virtual bool filter(const BlockClassListModelEntry& data, const SearchPattern& filter, int colIndex = 0) const override;

    virtual ElementPtr tooltip(AbstractItemView* view, ModelIndex id) const override;
};

///----

// result passed
struct BlockClassPickResult
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(BlockClassPickResult);

    SpecificClassType<graph::Block> blockClass;
    StringID socketName;
};

DECLARE_UI_EVENT(EVENT_GRAPH_BLOCK_CLASS_SELECTED, BlockClassPickResult)

// helper dialog that allows to select a type from type list
class BlockClassPickerBox : public PopupWindow
{
    RTTI_DECLARE_VIRTUAL_CLASS(BlockClassPickerBox, PopupWindow);

public:
    BlockClassPickerBox(const graph::Container& filter, const graph::Socket* compatibleSocket);

    // generated OnBlockClassSelected when selected and general OnClosed when window itself is closed

private:
    ListView* m_list;
    RefPtr<BlockClassListModel> m_listModel;
    SearchBarPtr m_searchBar;

    virtual IElement* focusFindFirst() override;

    virtual bool handleKeyEvent(const InputKeyEvent& evt) override;
    void closeWithType(const BlockClassPickResult& result);
    void closeIfValidTypeSelected();
};

///----

END_BOOMER_NAMESPACE_EX(ui)
