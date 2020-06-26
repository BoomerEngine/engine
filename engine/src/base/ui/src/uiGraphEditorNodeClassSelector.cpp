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

#include "base/containers/include/stringBuilder.h"
#include "base/graph/include/graphContainer.h"
#include "base/graph/include/graphBlock.h"

namespace ui
{
    //---

    BlockClassListModel::BlockClassListModel(const base::graph::Container& filter, const base::graph::Socket* compatibleSocket)
    {
        if (compatibleSocket == nullptr)
        {
            base::InplaceArray<base::SpecificClassType<base::graph::Block>, 64> allBlockClasses;
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
            base::InplaceArray<base::graph::SupprotedBlockConnectionInfo, 64> allCompatibleBlocks;
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

    base::StringBuf BlockClassListModel::content(const BlockClassListModelEntry& data, int colIndex) const
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

    ui::ElementPtr BlockClassListModel::tooltip(ui::AbstractItemView* view, ui::ModelIndex id) const
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

    BlockClassPickerBox::BlockClassPickerBox(const base::graph::Container& filter, const base::graph::Socket* compatibleSocket)
    {
        layoutVertical();

        // title bar
        createChild<WindowTitleBar>()->addStyleClass("mini"_id);

        // filter
        auto filterBar = createChild<ui::SearchBar>();

        // list of types
        m_list = createChild<ui::ListView>();
        m_list->customInitialSize(500, 400);
        m_list->expand();
        filterBar->bindItemView(m_list);

        // buttons
        {
            auto buttons = createChild<IElement>();
            buttons->customPadding(5);
            buttons->layoutHorizontal();

            {
                auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "[img:accept] Select");
                button->addStyleClass("green"_id);
                button->bind("OnClick"_id) = [this]() { closeIfValidTypeSelected(); };
            }

            {
                auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "Cancel");
                button->bind("OnClick"_id) = [this]() { requestClose(); };
            }
        }

        // bind model
        m_listModel = base::CreateSharedPtr<BlockClassListModel>(filter, compatibleSocket);
        m_list->model(m_listModel);
        m_list->bind("OnItemActivated"_id) = [this]()
        {
            closeIfValidTypeSelected();
        };
    }

    bool BlockClassPickerBox::handleKeyEvent(const base::input::KeyEvent & evt)
    {
        if (evt.pressed() && evt.keyCode() == base::input::KeyCode::KEY_ESCAPE)
        {
            requestClose();
            return true;
        }
        else if (evt.pressed() && evt.keyCode() == base::input::KeyCode::KEY_RETURN)
        {
            closeIfValidTypeSelected();
            return true;
        }

        return TBaseClass::handleKeyEvent(evt);
    }

    IElement* BlockClassPickerBox::handleFocusForwarding()
    {
        return m_list;
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
        call("OnBlockClassSelected"_id, result);
        requestClose();
    }

    //---

} // ui

