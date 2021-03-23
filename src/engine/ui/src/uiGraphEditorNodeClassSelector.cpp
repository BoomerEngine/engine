/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: graph #]
***/

#include "build.h"
#include "uiGraphEditorNodeClassSelector.h"
#include "uiButton.h"
#include "uiTextLabel.h"
#include "uiWindow.h"
#include "uiImage.h"
#include "uiRenderer.h"
#include "uiListView.h"
#include "uiGraphEditor.h"
#include "uiSearchBar.h"

#include "core/containers/include/stringBuilder.h"
#include "core/graph/include/graphContainer.h"
#include "core/graph/include/graphBlock.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

BlockClassListModel::BlockClassListModel(const graph::Container& filter, const graph::Socket* compatibleSocket)
{
    if (compatibleSocket == nullptr)
    {
        InplaceArray<SpecificClassType<graph::Block>, 64> allBlockClasses;
        filter.supportedBlockClasses(allBlockClasses);

        for (const auto cls : allBlockClasses)
        {
            if (filter.canAddBlockOfClass(cls))
            {
                BlockClassListModelEntry entry;
                entry.dataString = FormatBlockClassDisplayTitle(cls);
                entry.displayString = FormatBlockClassDisplayString(cls);
                entry.info.blockClass = cls;
                add(entry);
            }
        }
    }
    else
    {
        InplaceArray<graph::SupprotedBlockConnectionInfo, 64> allCompatibleBlocks;
        filter.supportedBlockConnections(compatibleSocket, allCompatibleBlocks);

        for (const auto& info : allCompatibleBlocks)
        {
            BlockClassListModelEntry entry;
            entry.dataString = FormatBlockClassDisplayTitle(info.blockClass);
            entry.displayString = FormatBlockClassDisplayString(info.blockClass);
            entry.info = info;
            add(entry);
        }
    }
}

StringBuf BlockClassListModel::content(const BlockClassListModelEntry& data, int colIndex) const
{
    return data.displayString;
}

bool BlockClassListModel::compare(const BlockClassListModelEntry& a, const BlockClassListModelEntry& b, int colIndex) const
{
    return a.dataString < b.dataString;
}

bool BlockClassListModel::filter(const BlockClassListModelEntry& data, const SearchPattern& filter, int colIndex /*= 0*/) const
{
    return filter.testString(data.dataString);
}

ElementPtr BlockClassListModel::tooltip(AbstractItemView* view, ModelIndex id) const
{
    if (const auto* ptr = dataPtr(id))
        if (ptr->info.blockClass)
            return CreateGraphBlockTooltip(ptr->info.blockClass);

    return nullptr;
}

//---

RTTI_BEGIN_TYPE_CLASS(BlockClassPickResult);
    RTTI_BIND_NATIVE_COPY(BlockClassPickResult);
    RTTI_PROPERTY(blockClass);
    RTTI_PROPERTY(socketName);
RTTI_END_TYPE();

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(BlockClassPickerBox);
RTTI_END_TYPE();

//---

BlockClassPickerBox::BlockClassPickerBox(const graph::Container& filter, const graph::Socket* compatibleSocket)
    : PopupWindow(WindowFeatureFlagBit::DEFAULT_POPUP_DIALOG, "Create block")
{
    // filter
    m_searchBar = createChild<SearchBar>();

    // list of types
    m_list = createChild<ListView>();
    m_list->customInitialSize(500, 400);
    m_list->expand();
    m_searchBar->bindItemView(m_list);

    // buttons
    {
        auto buttons = createChild();
        buttons->customPadding(5);
        buttons->layoutHorizontal();

        {
            auto button = buttons->createChildWithType<Button>("PushButton"_id, "[img:accept] Select");
            button->addStyleClass("green"_id);
            button->bind(EVENT_CLICKED) = [this]() { closeIfValidTypeSelected(); };
        }

        {
            auto button = buttons->createChildWithType<Button>("PushButton"_id, "Cancel");
            button->bind(EVENT_CLICKED) = [this]() { requestClose(); };
        }
    }

    // bind model
    m_listModel = RefNew<BlockClassListModel>(filter, compatibleSocket);
    m_list->model(m_listModel);
    m_list->bind(EVENT_ITEM_ACTIVATED) = [this]()
    {
        closeIfValidTypeSelected();
    };
}

IElement* BlockClassPickerBox::focusFindFirst()
{
    return m_searchBar;
}

bool BlockClassPickerBox::handleKeyEvent(const InputKeyEvent & evt)
{
    if (evt.pressed() && evt.keyCode() == InputKey::KEY_ESCAPE)
    {
        requestClose();
        return true;
    }
    else if (evt.pressed() && evt.keyCode() == InputKey::KEY_RETURN)
    {
        closeIfValidTypeSelected();
        return true;
    }

    return TBaseClass::handleKeyEvent(evt);
}

void BlockClassPickerBox::closeIfValidTypeSelected()
{
    if (m_list)
    {
        auto selected = m_listModel->data(m_list->selectionRoot());
        if (selected.info.blockClass)
        {
            BlockClassPickResult result;
            result.blockClass = selected.info.blockClass;

            if (!selected.info.socketNames.empty())
                result.socketName = selected.info.socketNames[0];

            closeWithType(result);
        }
    }
}

void BlockClassPickerBox::closeWithType(const BlockClassPickResult& result)
{
    call(EVENT_GRAPH_BLOCK_CLASS_SELECTED, result);
    requestClose();
}

//---

END_BOOMER_NAMESPACE_EX(ui)

