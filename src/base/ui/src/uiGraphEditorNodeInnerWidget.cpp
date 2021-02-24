/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: graph #]
*
***/

#include "build.h"
#include "uiGraphEditorNodeInnerWidget.h"

#include "base/graph/include/graphBlock.h"

BEGIN_BOOMER_NAMESPACE(ui)

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IGraphNodeInnerWidget);
    RTTI_METADATA(ElementClassNameMetadata).name("GraphInnerWidget");
RTTI_END_TYPE();

IGraphNodeInnerWidget::IGraphNodeInnerWidget()
{
    layoutVertical();
}

IGraphNodeInnerWidget::~IGraphNodeInnerWidget()
{}

void IGraphNodeInnerWidget::bindToActionHistory(base::ActionHistory* history)
{
}

//--

RTTI_BEGIN_TYPE_CLASS(GraphNodeInnerWidgetBlockClassMetadata);
RTTI_END_TYPE();

GraphNodeInnerWidgetBlockClassMetadata::GraphNodeInnerWidgetBlockClassMetadata()
{}

GraphNodeInnerWidgetBlockClassMetadata& GraphNodeInnerWidgetBlockClassMetadata::addBlockClass(base::StringID className)
{
    auto blockClass = RTTI::GetInstance().findClass(className).cast<base::graph::Block>();
    DEBUG_CHECK_EX(blockClass, base::TempString("Unknown graph block class '{}'", blockClass));
    if (blockClass)
        addBlockClass(blockClass);
    return *this;
}

//--

class GraphNodeInnerWidgetClassRegistry : public base::ISingleton
{
    DECLARE_SINGLETON(GraphNodeInnerWidgetClassRegistry);

public:
    GraphNodeInnerWidgetClassRegistry()
    {
        base::InplaceArray<base::SpecificClassType<IGraphNodeInnerWidget>, 100> innerWidgetClasses;
        RTTI::GetInstance().enumClasses(innerWidgetClasses);

        for (const auto innerClass : innerWidgetClasses)
        {
            if (const auto* blockClassMetadata = innerClass->findMetadata<GraphNodeInnerWidgetBlockClassMetadata>())
            {
                for (const auto blockClass : blockClassMetadata->classes())
                    m_classMap[blockClass].pushBack(innerClass);
            }
        }
    }

    INLINE const base::Array<base::SpecificClassType<IGraphNodeInnerWidget>>& classListForBlockClass(base::ClassType blockClass)
    {
        if (const auto* entry = m_classMap.find(blockClass))
            return *entry;

        static base::Array<base::SpecificClassType<IGraphNodeInnerWidget>> theEmptyTable;
        return theEmptyTable;
    }

private:
    base::HashMap<base::SpecificClassType<base::graph::Block>, base::Array<base::SpecificClassType<IGraphNodeInnerWidget>>> m_classMap;

    virtual void deinit() override
    {
        m_classMap.clear();
    }
};

GraphNodeInnerWidgetPtr CreateGraphBlockInnerWidget(base::graph::Block* block)
{
    if (block)
    {
        const auto& compatibleWidgetClasses = GraphNodeInnerWidgetClassRegistry::GetInstance().classListForBlockClass(block->cls());
        for (const auto& innerWidgetClass : compatibleWidgetClasses)
            if (auto widget = innerWidgetClass.create())
                if (widget->bindToBlock(block))
                    return widget;
    }

    return nullptr;
}

//--

END_BOOMER_NAMESPACE(ui)
