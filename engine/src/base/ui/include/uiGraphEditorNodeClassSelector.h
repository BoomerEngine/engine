/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: graph #]
***/

#pragma once

#include "uiEventFunction.h"
#include "uiWindow.h"
#include "uiSimpleListModel.h"
#include "base/graph/include/graphContainer.h"

namespace ui
{
    ///----

    // entry in the block class list model
    struct BlockClassListModelEntry
    {
        base::graph::SupprotedBlockConnectionInfo info;
        base::StringBuf dataString;
        base::StringBuf displayString;
    };

    // data model for listing block classes
    class BlockClassListModel : public SimpleTypedListModel<BlockClassListModelEntry>
    {
    public:
        BlockClassListModel(const base::graph::Container& filter, const base::graph::Socket* compatibleSocket);

    private:
        virtual base::StringBuf content(const BlockClassListModelEntry& data, int colIndex = 0) const override;
        virtual bool compare(const BlockClassListModelEntry& a, const BlockClassListModelEntry& b, int colIndex) const override;
        virtual bool filter(const BlockClassListModelEntry& data, const SearchPattern& filter, int colIndex = 0) const override;

        virtual ui::ElementPtr tooltip(ui::AbstractItemView* view, ui::ModelIndex id) const override;
    };

    ///----

    // result passed
    struct BlockClassPickResult
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(BlockClassPickResult);

        base::SpecificClassType<base::graph::Block> blockClass;
        base::StringID socketName;
    };

    // helper dialog that allows to select a type from type list
    class BlockClassPickerBox : public PopupWindow
    {
        RTTI_DECLARE_VIRTUAL_CLASS(BlockClassPickerBox, PopupWindow);

    public:
        BlockClassPickerBox(const base::graph::Container& filter, const base::graph::Socket* compatibleSocket);

        // generated OnBlockClassSelected when selected and general OnClosed when window itself is closed

    private:
        ListView* m_list;
        base::RefPtr<BlockClassListModel> m_listModel;

        virtual bool handleKeyEvent(const base::input::KeyEvent& evt) override;
        void closeWithType(const BlockClassPickResult& result);
        void closeIfValidTypeSelected();

        virtual IElement* handleFocusForwarding() override;
    };

    ///----

} // ui