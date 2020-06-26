/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\toolbar #]
***/

#include "build.h"
#include "uiToolBar.h"
#include "uiButton.h"
#include "uiTextLabel.h"

namespace ui
{

    //---

    RTTI_BEGIN_TYPE_CLASS(ToolBar);
        RTTI_METADATA(ElementClassNameMetadata).name("Toolbar");
    RTTI_END_TYPE();

    ToolBar::ToolBar()
        : m_timerUpdateState(this)
    {
        layoutHorizontal();
        enableAutoExpand(true, false);

        m_timerUpdateState.startRepeated(0.1f);
        m_timerUpdateState = [this]() { updateButtonState(); };
    }

    void ToolBar::createSeparator()
    {
        createChildWithType<IElement>("ToolbarSeparator"_id);
    }

    void ToolBar::createButton(base::StringID action, base::StringView<char> caption, base::StringView<char> tooltip/* = ""*/)
    {
        auto but = createChildWithType<Button>("ToolbarButton"_id);
        if (caption)
            but->createChild<ui::TextLabel>(caption);
        if (tooltip)
            but->tooltip(tooltip);

        if (action)
        {
            but->bind("OnClick"_id, this) = action;
            but->customStyle("action"_id, action);
        }
    }

    void ToolBar::attachChild(IElement* childElement)
    {
        TBaseClass::attachChild(childElement);
    }

    void ToolBar::detachChild(IElement* childElement)
    {
        TBaseClass::detachChild(childElement);
    }

    void ToolBar::updateButtonState()
    {
        bool separatorAllowed = false;
        for (ElementChildIterator it(childrenList()); it; ++it)
        {
            if (it->styleType() == "ToolbarSeparator"_id)
            {
                it->visibility(separatorAllowed);
                separatorAllowed = false;
            }
            else if (it->is<Button>())
            {
                if (const auto actionName = it->evalStyleValue<base::StringID>("action"_id))
                {
                    auto actionState = checkAction(actionName);
                    bool visible = actionState.test(ActionStatusBit::Defined);
                    it->visibility(visible);

                    if (visible)
                    {
                        it->enable(actionState.test(ActionStatusBit::Enabled));
                        separatorAllowed = true;

                        if (auto button = base::rtti_cast<Button>(*it))
                            button->toggle(actionState.test(ActionStatusBit::Toggled));
                    }
                }
                else
                {
                    it->visibility(true);
                    separatorAllowed = true;
                }
            }
        }
    }

    bool ToolBar::handleTemplateProperty(base::StringView<char> name, base::StringView<char> value)
    {
        return TBaseClass::handleTemplateProperty(name, value);
    }

    bool ToolBar::handleTemplateChild(base::StringView<char> name, const base::xml::IDocument& doc, const base::xml::NodeID& id)
    {
        if (name == "Tool")
        {
            auto action = doc.nodeAttributeOfDefault(id, "action");
            if (action)
            {
                auto caption = doc.nodeAttributeOfDefault(id, "caption", action);
                auto tooltip = doc.nodeAttributeOfDefault(id, "tooltip");
                createButton(base::StringID(action), caption, tooltip);
                return true;
            }
        }
        else if (name == "ToolSeparator")
        {
            createSeparator();
            return true;
        }

        return TBaseClass::handleTemplateChild(name, doc, id);;
    }

    //---

} // ui
