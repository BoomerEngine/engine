/***
* Boomer Engine v4 2015 - 2017
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\layout #]
***/

#pragma once

#include "uiElement.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

DECLARE_UI_EVENT(EVENT_GROUP_EXPANDED)
DECLARE_UI_EVENT(EVENT_GROUP_COLLAPSED)

//--

/// expandable container (3ds max style) that can be collapsed (hidden)
class ENGINE_UI_API ExpandableContainer : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(ExpandableContainer, IElement);

public:
    ExpandableContainer(bool expanded = false);

    //--

    // get the header element (allows to insert custom elements)
    INLINE const ElementPtr& header() const { return m_header; }

    // get the body element
    INLINE const ElementPtr& body() const { return m_container; }

    //--

    // is the container expanded ?
    bool expanded() const;

    // toggle expand state
    void expand(bool expand);

    //--

    virtual void attachChild(IElement* childElement) override final;
    virtual void detachChild(IElement* childElement) override final;

protected:
    ButtonPtr m_button;
    ElementPtr m_container;
    ElementPtr m_header;
};
    
//--

/// ui group container (3ds max style) that can be collapsed (hidden)
/// very good for placing tools in a side panel
class ENGINE_UI_API Group : public ExpandableContainer
{
    RTTI_DECLARE_VIRTUAL_CLASS(Group, ExpandableContainer);

public:
    Group(StringView caption = "Group", bool expanded = true);

    /// change name
    void name(StringView txt);

protected:
    TextLabelPtr m_caption;
};

//--

END_BOOMER_NAMESPACE_EX(ui)
