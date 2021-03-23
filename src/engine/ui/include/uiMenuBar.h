/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\menu #]
***/

#pragma once

#include "uiElement.h"
#include "uiButton.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

DECLARE_UI_EVENT(EVENT_MENU_ITEM_CLICKED)

//--

/// typical menu bar, the popup menus are built on demand
class ENGINE_UI_API MenuBar : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(MenuBar, IElement);

public:
    MenuBar();

    void createMenu(StringView caption, const TPopupFunc& menuFunc);
    void createMenu(StringView caption, const PopupPtr& menu);
};

//--

    /// menu button
class ENGINE_UI_API MenuBarItem : public Button
{
    RTTI_DECLARE_VIRTUAL_CLASS(MenuBarItem, Button);

public:
    MenuBarItem();
    MenuBarItem(const TPopupFunc& func, StringView text);

private:
    virtual void handleHoverEnter(const Position& absolutePosition) override;
    virtual void clicked() override;

    TPopupFunc m_func;
    PopupWeakPtr m_openedPopup;
    NativeTimePoint m_nextOpenPossible;

    bool hasOtherPopups();
    void openPopup();
    bool closePopup();
    void closeOtherPopups();
};

//--

/// container for menu buttons
class ENGINE_UI_API MenuButtonContainer : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(MenuButtonContainer, IElement);

public:
    MenuButtonContainer();

    void createSubMenu(const TPopupFunc& func, StringView text, StringView icon = "");
    void createSubMenu(const PopupPtr& popup, StringView text, StringView icon = "");
    void createSeparator();

    EventFunctionBinder createCallback(StringView text, StringView icon = "", StringView shortcut="", bool enabled=true);

    bool show(IElement* owner);
    bool showAsDropdown(IElement* owner);

    PopupPtr convertToPopup();
};

//--

/// menu button
class ENGINE_UI_API MenuButton : public Button
{
    RTTI_DECLARE_VIRTUAL_CLASS(MenuButton, Button);

public:
    MenuButton();
    MenuButton(StringView text, StringView icon, StringView shortcut="");
    MenuButton(const TPopupFunc& func, StringView text, StringView icon);

private:
    virtual bool handleHoverDuration(const Position& absolutePosition) override;

    TPopupFunc m_func;
    PopupWeakPtr m_openedPopup;

    bool closePopup();
    void closeOtherPopups();
};

//--

END_BOOMER_NAMESPACE_EX(ui)
