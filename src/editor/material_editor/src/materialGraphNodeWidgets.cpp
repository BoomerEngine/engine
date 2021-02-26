/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "materialGraphEditor.h"
#include "materialGraphNodeWidgets.h"

#include "engine/ui/include/uiEditBox.h"
#include "core/object/include/dataView.h"
#include "core/object/include/rttiDataView.h"
#include "engine/ui/include/uiDataBox.h"
#include "core/graph/include/graphBlock.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

RTTI_BEGIN_TYPE_CLASS(MaterialGraphConstantScalarWidget);
    RTTI_METADATA(ui::GraphNodeInnerWidgetBlockClassMetadata)
        .addBlockClass("MaterialGraphBlock_ConstFloat"_id)
        .addBlockClass("MaterialGraphBlock_ConstVector2"_id)
        .addBlockClass("MaterialGraphBlock_ConstVector3"_id)
        .addBlockClass("MaterialGraphBlock_ConstVector4"_id);
RTTI_END_TYPE();

bool MaterialGraphConstantScalarWidget::bindToBlock(graph::Block* block)
{
    if (block)
    {
        if (auto proxy = block->createDataView())
        {
            rtti::DataViewInfo info;
            if (proxy->describeDataView("value", info).valid())
            {
                info.flags |= rtti::DataViewInfoFlagBit::VerticalEditor;
                if (auto box = ui::IDataBox::CreateForType(info))
                {
                    box->bindData(proxy, "value"); // TODO: bind undo redo
                    box->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
                    box->customVerticalAligment(ui::ElementVerticalLayout::Middle);
                    if (block->cls()->name().view().endsWith("ConstFloat"))
                        box->customMinSize(50, 20);
                    else
                        box->customMinSize(80, 20);

                    m_box = box;
                    attachChild(box);
                    return true;
                }
            }
        }
    }

    return false;
}

void MaterialGraphConstantScalarWidget::bindToActionHistory(ActionHistory* history)
{
    if (m_box)
        m_box->bindActionHistory(history);
}

//--

RTTI_BEGIN_TYPE_CLASS(MaterialGraphConstantColorWidget);
    RTTI_METADATA(ui::GraphNodeInnerWidgetBlockClassMetadata).addBlockClass("MaterialGraphBlock_ConstColor"_id);
RTTI_END_TYPE();

bool MaterialGraphConstantColorWidget::bindToBlock(graph::Block* block)
{
    if (block)
    {
        if (auto proxy = block->createDataView())
        {
            rtti::DataViewInfo info;
            if (proxy->describeDataView("color", info).valid())
            {
                info.flags |= rtti::DataViewInfoFlagBit::VerticalEditor;
                if (auto box = ui::IDataBox::CreateForType(info))
                {
                    box->bindData(proxy, "color");
                    box->customMinSize(100, 20);
                    box->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
                    box->customVerticalAligment(ui::ElementVerticalLayout::Middle);
                    attachChild(box);

                    m_box = box;
                    return true;
                }
            }
        }
    }

    return false;
}

void MaterialGraphConstantColorWidget::bindToActionHistory(ActionHistory* history)
{
    if (m_box)
        m_box->bindActionHistory(history);
}

//--

END_BOOMER_NAMESPACE_EX(ed)
