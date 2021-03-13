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

//------

/// toolbar button creation info
struct ENGINE_UI_API ToolBarButtonInfo
{
    ToolBarButtonInfo(StringID id = StringID());
    ToolBarButtonInfo(const ToolBarButtonInfo& other);
    ToolBarButtonInfo& operator=(const ToolBarButtonInfo& other);

    //-

    bool m_enabled = true; // initially enabled
    bool m_toggled = false; // initially toggled
    StringID m_id; // only if we want to find it
    StringBuf m_caption; // caption
    StringBuf m_tooltip; // tooltip to display
    KeyShortcut m_shortcut; // automatically bind a shortcut

    //--

    // constructs icon + text on the side caption
    static StringBuf MakeIconWithTextCaption(StringView icon, StringView text);

    // constructs the icon + text on the bottom caption
    static StringBuf MakeIconWithSmallTextCaption(StringView icon, StringView text);

    //--

    // compile a toolbar button, can be added to a toolbar ;)
    ButtonPtr create() const;

    //--

    ToolBarButtonInfo& separator() { m_caption = "|"; return *this; }
    ToolBarButtonInfo& disabled() { m_enabled = false; return *this; }
    ToolBarButtonInfo& enabled(bool flag) { m_enabled = flag; return *this; }
    ToolBarButtonInfo& toggled(bool flag) { m_toggled = flag;  return *this; }
    ToolBarButtonInfo& shortcut(KeyShortcut key) { m_shortcut = key; return *this; }
    ToolBarButtonInfo& caption(StringView txt) { m_caption = StringBuf(txt); return *this; }
    ToolBarButtonInfo& caption(StringView txt, StringView icon) { m_caption = MakeIconWithSmallTextCaption(icon, txt); return *this; }
    ToolBarButtonInfo& tooltip(StringView txt) { m_tooltip = StringBuf(txt); return *this; }
};

//--

/// toolbar - collection of tool elements
class ENGINE_UI_API ToolBar : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(ToolBar, IElement);

public:
    ToolBar();
    virtual ~ToolBar();

    //--

    // add button, allows to bind function
    EventFunctionBinder createButton(const ToolBarButtonInfo& info);

    // add a vertical separator
    void createSeparator();

    //--

    // update caption of a button (should have ID)
    void updateButtonCaption(StringID id, StringView caption);
    void updateButtonCaption(StringID id, StringView caption, StringView icon);

    // toggle button state
    void toggleButton(StringID id, bool state);

    // enable disable button
    void enableButton(StringID id, bool state);

    //-
};

//--

END_BOOMER_NAMESPACE_EX(ui)
