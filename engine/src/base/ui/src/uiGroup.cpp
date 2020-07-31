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

namespace ui
{
    //---

    RTTI_BEGIN_TYPE_CLASS(Group);
        RTTI_METADATA(ElementClassNameMetadata).name("Group");
    RTTI_END_TYPE();

    Group::Group(base::StringView<char> caption, bool expanded)
    {
        layoutMode(LayoutMode::Vertical);

        if (expanded)
            addStyleClass("expanded"_id);

        m_header = createInternalNamedChild<>("Header"_id);
        m_header->layoutHorizontal();

        {
            m_button = m_header->createChild<Button>(ButtonModeBit::EventOnClick);
            m_button->bind(EVENT_CLICKED, this) = [](Group* group) { group->expand(!group->expanded()); };
            m_button->createNamedChild<TextLabel>("ExpandIcon"_id);
            m_caption = m_button->createChild<TextLabel>();
        }

        m_container = createInternalNamedChild<>("Container"_id);
        m_container->layoutVertical();
        m_container->visibility(expanded);
    }

    bool Group::expanded() const
    {
        return m_container->visibility() == VisibilityState::Visible;
    }

    void Group::expand(bool expand)
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

    void Group::attachChild(IElement* childElement)
    {
        m_container->attachChild(childElement);
    }

    void Group::detachChild(IElement* childElement)
    {
        m_container->attachChild(childElement);
    }

    //---

    bool Group::handleTemplateProperty(base::StringView<char> name, base::StringView<char> value)
    {
        if (name == "text")
        {
            m_caption->text(base::StringBuf(value));
            return true;
        }
        else if (name == "expanded")
        {
            bool flag = false;
            if (base::MatchResult::OK == value.match(flag))
                expand(flag);

            return true;
        }

        return TBaseClass::handleTemplateProperty(name, value);
    }

    //---

} // ui