/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\layout #]
***/

#include "build.h"

#include "uiGroup.h"
#include "uiButton.h"
#include "uiTextLabel.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

RTTI_BEGIN_TYPE_CLASS(ExpandableContainer);
    RTTI_METADATA(ElementClassNameMetadata).name("Group");
RTTI_END_TYPE();

ExpandableContainer::ExpandableContainer(bool expanded)
{
    layoutMode(LayoutMode::Vertical);

    if (expanded)
        addStyleClass("expanded"_id);

    m_header = createInternalNamedChild<>("Header"_id);
    m_header->layoutHorizontal();

    {
        m_button = m_header->createChild<Button>(ButtonModeBit::EventOnClick);
        m_button->bind(EVENT_CLICKED) = [this]() { expand(!this->expanded()); };
        m_button->createNamedChild<TextLabel>("ExpandIcon"_id);
        //m_caption = m_button->createChild<TextLabel>(caption);
    }

    m_container = createInternalNamedChild<>("Container"_id);
    m_container->layoutVertical();
    m_container->visibility(expanded);
}

bool ExpandableContainer::expanded() const
{
    return m_container->visibility() == VisibilityState::Visible;
}

void ExpandableContainer::expand(bool expand)
{
    if (expand != expanded())
    {
        m_container->visibility(expand ? VisibilityState::Visible : VisibilityState::Hidden);

        if (expand)
        {
            addStyleClass("expanded"_id);
            call(EVENT_GROUP_EXPANDED);
        }
        else
        {
            removeStyleClass("expanded"_id);
            call(EVENT_GROUP_COLLAPSED);
        }
    }
}

void ExpandableContainer::attachChild(IElement* childElement)
{
    m_container->attachChild(childElement);
}

void ExpandableContainer::detachChild(IElement* childElement)
{
    m_container->attachChild(childElement);
}

//---

RTTI_BEGIN_TYPE_CLASS(Group);
RTTI_END_TYPE();

Group::Group(StringView caption, bool expanded)
    : ExpandableContainer(expanded)
{
    m_caption = m_button->createChild<TextLabel>(caption);
}

void Group::name(StringView txt)
{
    m_caption->text(txt);
}

//---

END_BOOMER_NAMESPACE_EX(ui)
