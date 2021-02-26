/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: graph #]
*
***/

#pragma once

#include "uiElement.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

/// widget that can appear inside a graph node to allow additional data edit
class ENGINE_UI_API IGraphNodeInnerWidget : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(IGraphNodeInnerWidget, IElement);

public:
    IGraphNodeInnerWidget();
    virtual ~IGraphNodeInnerWidget();

    // try to bind to given block
    virtual bool bindToBlock(graph::Block* block) = 0;

    // bind inner widget to action history (to provide undo/redo)
    virtual void bindToActionHistory(ActionHistory* history);
};

//--

/// metadata for the graph node inner widget classes that specified to which block classes we may try to bind
class ENGINE_UI_API GraphNodeInnerWidgetBlockClassMetadata : public rtti::IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(GraphNodeInnerWidgetBlockClassMetadata, rtti::IMetadata);

public:
    GraphNodeInnerWidgetBlockClassMetadata();

    GraphNodeInnerWidgetBlockClassMetadata& addBlockClass(StringID className);

    INLINE GraphNodeInnerWidgetBlockClassMetadata& addBlockClass(SpecificClassType<graph::Block> sourceClass)
    {
        if (sourceClass)
            m_classes.pushBackUnique(sourceClass);
        return *this;
    }

    template< typename T >
    INLINE GraphNodeInnerWidgetBlockClassMetadata& addBlockClass()
    {
        static_assert(std::is_base_of< graph::Block, T >::value, "Only block classes are supported by this metadata");
        addBlockClass(T::GetStaticClass());
        return *this;
    }

    INLINE const Array<SpecificClassType<graph::Block>>& classes() const
    {
        return m_classes;
    }

private:
    Array<SpecificClassType<graph::Block>> m_classes;
};

//--

// create inner widget for a block
extern ENGINE_UI_API GraphNodeInnerWidgetPtr CreateGraphBlockInnerWidget(graph::Block* block);

//--

END_BOOMER_NAMESPACE_EX(ui)
