/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\toolbar #]
***/

#pragma once

#include "uiElement.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

/// toolbar button
struct ToolbarButtonSetup
{
    StringView m_icon;
    StringView m_caption;
    StringView m_tooltip;

    INLINE bool valid() const { return m_icon || m_caption; }

    INLINE ToolbarButtonSetup() {}
    INLINE ToolbarButtonSetup& icon(StringView txt) { m_icon = txt; return *this; };
    INLINE ToolbarButtonSetup& caption(StringView txt) { m_caption = txt; return *this; };
    INLINE ToolbarButtonSetup& tooltip(StringView txt) { m_tooltip = txt; return *this; };
};

//--

/// toolbar - collection of tool elements
class ENGINE_UI_API ToolBar : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(ToolBar, IElement);

public:
    ToolBar();

    // add a vertical separator to the toolbar
    void createSeparator();

    // add a simple tool button
    void createButton(StringID action, const ToolbarButtonSetup& setup);

    // add a simple tool button with direct action
    EventFunctionBinder createCallback(const ToolbarButtonSetup& setup);

    // update caption on a button
    void updateButtonCaption(StringID action, const ToolbarButtonSetup& setup);

protected:
    Timer m_timerUpdateState;

    HashMap<StringID, ButtonPtr> m_actionButtons;

    void updateButtonState();

    virtual void attachChild(IElement* childElement) override;
    virtual void detachChild(IElement* childElement) override;
};

//--

END_BOOMER_NAMESPACE_EX(ui)
