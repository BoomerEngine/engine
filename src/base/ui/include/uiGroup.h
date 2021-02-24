/***
* Boomer Engine v4 2015 - 2017
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\layout #]
***/

#pragma once

#include "uiElement.h"

BEGIN_BOOMER_NAMESPACE(ui)

//--

DECLARE_UI_EVENT(EVENT_GROUP_EXPANDED)
DECLARE_UI_EVENT(EVENT_GROUP_COLLAPSED)
    
//--

/// ui group container (3ds max style) that can be collapsed (hidden)
/// very good for placing tools in a side panel
class BASE_UI_API Group : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(Group, IElement);

public:
    Group(base::StringView caption = "Group", bool expanded = true);

    //--

    // is the container expanded ?
    bool expanded() const;

    // toggle expand state
    void expand(bool expand);

    //--

    virtual void attachChild(IElement* childElement) override final;
    virtual void detachChild(IElement* childElement) override final;

protected:
    base::RefPtr<Button> m_button;
    base::RefPtr<TextLabel> m_caption;
    base::RefPtr<IElement> m_container;
    base::RefPtr<IElement> m_header;

    //---

    virtual bool handleTemplateProperty(base::StringView name, base::StringView value) override;
};

//--

END_BOOMER_NAMESPACE(ui)
