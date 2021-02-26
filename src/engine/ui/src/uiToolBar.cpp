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

BEGIN_BOOMER_NAMESPACE_EX(ui)

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
    
static StringBuf RenderToolbarCaption(const ToolbarButtonSetup& setup)
{
    // 
    if (setup.m_caption && setup.m_icon)
    {
        return TempString("[center][img:{}][br][size:-]{}", setup.m_icon, setup.m_caption);
    }
    else if (setup.m_caption)
    {
        return TempString("{}", setup.m_caption);
    }
    else if (setup.m_icon)
    {
        return TempString("[img:{}]", setup.m_icon);
    }
    else
    {
        return TempString("Tool");
    }
}
        

void ToolBar::createButton(StringID action, const ToolbarButtonSetup& setup)
{
    if (action)
    {
        const auto captionString = RenderToolbarCaption(setup);
        auto but = createChildWithType<Button>("ToolbarButton"_id, captionString);

        if (setup.m_tooltip)
            but->tooltip(setup.m_tooltip);

        but->bind(EVENT_CLICKED, this) = action;
        but->customStyle("action"_id, action);

        m_actionButtons[action] = but;
    }
}

void ToolBar::updateButtonCaption(StringID action, const ToolbarButtonSetup& setup)
{
    ButtonPtr button;
    m_actionButtons.find(action, button);

    if (button)
    {
        if (const auto caption = rtti_cast<TextLabel>(*button->childrenList()))
        {
            const auto captionString = RenderToolbarCaption(setup);
            caption->text(captionString);
        }
    }
}

EventFunctionBinder ToolBar::createCallback(const ToolbarButtonSetup& setup)
{
    const auto captionString = RenderToolbarCaption(setup);
    auto but = createChildWithType<Button>("ToolbarButton"_id, captionString);

    if (setup.m_tooltip)
        but->tooltip(setup.m_tooltip);

    return but->bind(EVENT_CLICKED);
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
            if (const auto actionName = it->evalStyleValue<StringID>("action"_id))
            {
                auto actionState = checkAction(actionName);
                bool visible = actionState.test(ActionStatusBit::Defined);
                it->visibility(visible);

                if (visible)
                {
                    it->enable(actionState.test(ActionStatusBit::Enabled));
                    separatorAllowed = true;

                    if (auto button = rtti_cast<Button>(*it))
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

//---

END_BOOMER_NAMESPACE_EX(ui)
