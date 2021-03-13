/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\toolbar #]
***/

#include "build.h"
#include "uiToolBar.h"
#include "uiTextLabel.h"
#include "uiButton.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

ToolBarButtonInfo::ToolBarButtonInfo(StringID id)
    : m_id(id)
{}

ToolBarButtonInfo::ToolBarButtonInfo(const ToolBarButtonInfo& other) = default;
ToolBarButtonInfo& ToolBarButtonInfo::operator=(const ToolBarButtonInfo& other) = default;

StringBuf ToolBarButtonInfo::MakeIconWithSmallTextCaption(StringView icon, StringView text)
{
    if (text && icon)
        return TempString("[center][img:{}][br][size:-]{}", icon, text);
    else if (text)
        return StringBuf(text);
    else if (icon)
        return TempString("[img:{}]", icon);
    else
        return "Tool";
}

StringBuf ToolBarButtonInfo::MakeIconWithTextCaption(StringView icon, StringView text)
{
    if (text && icon)
        return TempString("[img:{}] {}", icon, text);
    else if (text)
        return StringBuf(text);
    else if (icon)
        return TempString("[img:{}]", icon);
    else
        return "Tool";
}

ButtonPtr ToolBarButtonInfo::create() const
{
    DEBUG_CHECK_RETURN_EX_V(m_caption, "Empty button caption", nullptr);

    auto but = RefNew<Button>(m_caption);
    but->styleType("ToolbarButton"_id);
    but->tooltip(m_tooltip);
    but->name(m_id);
    but->enable(m_enabled);
    but->toggle(m_toggled);

    return but;
}

//---

RTTI_BEGIN_TYPE_CLASS(ToolBar);
    RTTI_METADATA(ElementClassNameMetadata).name("Toolbar");
RTTI_END_TYPE();

ToolBar::ToolBar()
{
    layoutHorizontal();
    enableAutoExpand(true, false);
}

ToolBar::~ToolBar()
{
}

void ToolBar::createSeparator()
{
    createChildWithType<IElement>("ToolbarSeparator"_id);
}

EventFunctionBinder ToolBar::createButton(const ToolBarButtonInfo& info)
{
    if (auto button = info.create())
    {
        attachChild(button);
        return button->bind(EVENT_CLICKED);
    }

    return nullptr;
}

void ToolBar::updateButtonCaption(StringID id, StringView caption)
{
    for (auto it = childrenList(); it; ++it)
    {
        if (it->name() == id)
        {
            if (auto button = rtti_cast<Button>(*it))
            {
                button->text(caption);
                break;
            }
        }
    }
}

void ToolBar::updateButtonCaption(StringID id, StringView caption, StringView icon)
{
    const auto txt = ToolBarButtonInfo::MakeIconWithSmallTextCaption(icon, caption);
    updateButtonCaption(id, txt);
}

void ToolBar::enableButton(StringID id, bool state)
{
    for (auto it = childrenList(); it; ++it)
    {
        if (it->name() == id)
        {
            if (auto button = rtti_cast<Button>(*it))
            {
                button->enable(state);
                break;
            }
        }
    }
}

void ToolBar::toggleButton(StringID id, bool state)
{
    for (auto it = childrenList(); it; ++it)
    {
        if (it->name() == id)
        {
            if (auto button = rtti_cast<Button>(*it))
            {
                button->toggle(state);
                break;
            }
        }
    }
}

//---

END_BOOMER_NAMESPACE_EX(ui)
