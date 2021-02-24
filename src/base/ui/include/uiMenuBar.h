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

BEGIN_BOOMER_NAMESPACE(ui)

//--

DECLARE_UI_EVENT(EVENT_MENU_ITEM_CLICKED)

//--

/// typical menu bar, the popup menus are built on demand
class BASE_UI_API MenuBar : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(MenuBar, IElement);

public:
    MenuBar();

    void createMenu(base::StringView caption, const TPopupFunc& menuFunc);
    void createMenu(base::StringView caption, const PopupPtr& menu);

private:
    virtual bool handleTemplateProperty(base::StringView name, base::StringView value) override;
    virtual bool handleTemplateChild(base::StringView name, const base::xml::IDocument& doc, const base::xml::NodeID& id) override;
    virtual bool handleTemplateNewChild(const base::xml::IDocument& doc, const base::xml::NodeID& id, const base::xml::NodeID& childId, const ElementPtr& childElement) override;
    virtual bool handleTemplateFinalize() override;
};

//--

    /// menu button
class BASE_UI_API MenuBarItem : public Button
{
    RTTI_DECLARE_VIRTUAL_CLASS(MenuBarItem, Button);

public:
    MenuBarItem();
    MenuBarItem(const TPopupFunc& func, base::StringView text);

private:
    virtual bool handleTemplateProperty(base::StringView name, base::StringView value) override;
    virtual void handleHoverEnter(const Position& absolutePosition) override;
    virtual void clicked() override;

    TPopupFunc m_func;
    PopupWeakPtr m_openedPopup;
    base::NativeTimePoint m_nextOpenPossible;

    bool hasOtherPopups();
    void openPopup();
    bool closePopup();
    void closeOtherPopups();
};

//--

/// container for menu buttons
class BASE_UI_API MenuButtonContainer : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(MenuButtonContainer, IElement);

public:
    MenuButtonContainer();

    void createAction(base::StringID action, base::StringView text, base::StringView icon = "");
    void createSubMenu(const TPopupFunc& func, base::StringView text, base::StringView icon = "");
    void createSubMenu(const PopupPtr& popup, base::StringView text, base::StringView icon = "");
    void createSeparator();

    EventFunctionBinder createCallback(base::StringView text, base::StringView icon = "", base::StringView shortcut="", bool enabled=true);

    void show(IElement* owner);
    void showAsDropdown(IElement* owner);

    PopupPtr convertToPopup();

private:
    Timer m_timerUpdateState;

    void updateButtonState();

    virtual bool handleTemplateProperty(base::StringView name, base::StringView value) override;
    virtual bool handleTemplateChild(base::StringView name, const base::xml::IDocument& doc, const base::xml::NodeID& id) override;
    virtual bool handleTemplateNewChild(const base::xml::IDocument& doc, const base::xml::NodeID& id, const base::xml::NodeID& childId, const ElementPtr& childElement) override;
    virtual bool handleTemplateFinalize() override;
};

//--

/// menu button
class BASE_UI_API MenuButton : public Button
{
    RTTI_DECLARE_VIRTUAL_CLASS(MenuButton, Button);

public:
    MenuButton();
    MenuButton(base::StringID, base::StringView text, base::StringView icon, base::StringView shortcut="");
    MenuButton(const TPopupFunc& func, base::StringView text, base::StringView icon);

private:
    virtual bool handleTemplateProperty(base::StringView name, base::StringView value) override;
    virtual bool handleHoverDuration(const Position& absolutePosition) override;

    TPopupFunc m_func;
    PopupWeakPtr m_openedPopup;

    bool closePopup();
    void closeOtherPopups();
};

//--

END_BOOMER_NAMESPACE(ui)
