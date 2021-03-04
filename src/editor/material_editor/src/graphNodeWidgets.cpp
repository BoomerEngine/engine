/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "graphEditor.h"
#include "graphNodeWidgets.h"

#include "engine/ui/include/uiEditBox.h"
#include "engine/ui/include/uiComboBox.h"
#include "engine/ui/include/uiDataBox.h"
#include "engine/ui/include/uiDataBoxCustomChoice.h"
#include "engine/material/include/materialTemplate.h"
#include "engine/material/include/runtimeLayout.h"
#include "engine/material_graph/include/graphBlock_Parameter.h"
#include "engine/material_graph/include/graph.h"

#include "core/object/include/dataView.h"
#include "core/object/include/rttiDataView.h"
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

class DataBoxCustomChoiceMaterialParameter : public ui::DataBoxCustomChoice
{
    RTTI_DECLARE_VIRTUAL_CLASS(DataBoxCustomChoiceMaterialParameter, ui::DataBoxCustomChoice);

public:
    DataBoxCustomChoiceMaterialParameter(const MaterialGraph* graph, MaterialDataLayoutParameterType type)
        : m_graph(graph)
        , m_type(type)
    {}

    DataViewResult readOptionValue(StringBuf& outText) override final
    {
        StringID name;
        const auto ret = readValue(name);
        outText = StringBuf(name.view());
        return ret;
    }

    DataViewResult writeOptionValue(int optionIndex, StringView optionCaption) override final
    {
        const StringID paramName(optionCaption);
        return writeValue(paramName);
    }

    void queryOptions(Array<StringBuf>& outOptions) const override final
    {
        outOptions.reserve(m_graph->parameters().size());

        for (const auto& param : m_graph->parameters())
            if (param->queryType() == m_type)
                outOptions.emplaceBack(param->name().view());
    }

private:
    const MaterialGraph* m_graph = nullptr;
    MaterialDataLayoutParameterType m_type;
};

RTTI_BEGIN_TYPE_NATIVE_CLASS(DataBoxCustomChoiceMaterialParameter);
RTTI_END_TYPE();

RTTI_BEGIN_TYPE_CLASS(MaterialGraphParameterNameSelector);
    RTTI_METADATA(ui::GraphNodeInnerWidgetBlockClassMetadata)
        .addBlockClass("MaterialGraphBlock_BoundParameterScalar"_id)
        .addBlockClass("MaterialGraphBlock_BoundParameterVector2"_id)
        .addBlockClass("MaterialGraphBlock_BoundParameterVector3"_id)
        .addBlockClass("MaterialGraphBlock_BoundParameterVector4"_id)
        .addBlockClass("MaterialGraphBlock_BoundParameterColor"_id)
        .addBlockClass("MaterialGraphBlock_SamplerTexture"_id)
        .addBlockClass("MaterialGraphBlock_SamplerNormal"_id)
        .addBlockClass("MaterialGraphBlock_StaticSwitch"_id);
RTTI_END_TYPE();

bool MaterialGraphParameterNameSelector::bindToBlock(graph::Block* baseBlock)
{
    if (auto* block = rtti_cast<IMaterialGraphBlockBoundParameter>(baseBlock))
    {
        if (auto graph = block->findParent<MaterialGraph>())
        {
            if (auto proxy = block->createDataView())
            {
                auto box = RefNew<DataBoxCustomChoiceMaterialParameter>(graph, block->parameterType());
                box->bindData(proxy, "paramName");
                box->customMinSize(160, 20);
                box->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
                box->customVerticalAligment(ui::ElementVerticalLayout::Middle);
                attachChild(box);

                m_box = box;
                return true;
            }
        }
    }

    return false;
}

void MaterialGraphParameterNameSelector::bindToActionHistory(ActionHistory* history)
{
    if (m_box)
        m_box->bindActionHistory(history);
}

//--

END_BOOMER_NAMESPACE_EX(ed)
