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

BEGIN_BOOMER_NAMESPACE(ui)

//--

/// widget that can appear inside a graph node to allow additional data edit
class BASE_UI_API IGraphNodeInnerWidget : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(IGraphNodeInnerWidget, IElement);

public:
    IGraphNodeInnerWidget();
    virtual ~IGraphNodeInnerWidget();

    // try to bind to given block
    virtual bool bindToBlock(base::graph::Block* block) = 0;

    // bind inner widget to action history (to provide undo/redo)
    virtual void bindToActionHistory(base::ActionHistory* history);
};

//--

/// metadata for the graph node inner widget classes that specified to which block classes we may try to bind
class BASE_UI_API GraphNodeInnerWidgetBlockClassMetadata : public base::rtti::IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(GraphNodeInnerWidgetBlockClassMetadata, base::rtti::IMetadata);

public:
    GraphNodeInnerWidgetBlockClassMetadata();

    GraphNodeInnerWidgetBlockClassMetadata& addBlockClass(base::StringID className);

    INLINE GraphNodeInnerWidgetBlockClassMetadata& addBlockClass(base::SpecificClassType<base::graph::Block> sourceClass)
    {
        if (sourceClass)
            m_classes.pushBackUnique(sourceClass);
        return *this;
    }

    template< typename T >
    INLINE GraphNodeInnerWidgetBlockClassMetadata& addBlockClass()
    {
        static_assert(std::is_base_of< base::graph::Block, T >::value, "Only block classes are supported by this metadata");
        addBlockClass(T::GetStaticClass());
        return *this;
    }

    INLINE const base::Array<base::SpecificClassType<base::graph::Block>>& classes() const
    {
        return m_classes;
    }

private:
    base::Array<base::SpecificClassType<base::graph::Block>> m_classes;
};

//--

// create inner widget for a block
extern BASE_UI_API GraphNodeInnerWidgetPtr CreateGraphBlockInnerWidget(base::graph::Block* block);

//--

END_BOOMER_NAMESPACE(ui)