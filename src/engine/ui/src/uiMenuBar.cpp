/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\menu #]
***/

#include "build.h"

#include "uiMenuBar.h"
#include "uiButton.h"
#include "uiTextLabel.h"
#include "uiRenderer.h"
#include "uiWindowPopup.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)
    
//---

RTTI_BEGIN_TYPE_CLASS(MenuBar);
    RTTI_METADATA(ElementClassNameMetadata).name("MenuBar");
RTTI_END_TYPE();

MenuBar::MenuBar()
{
    layoutHorizontal();
}

void MenuBar::createMenu(StringView caption, const PopupPtr& menu)
{
    auto func = [menu]() { return menu; };
    createMenu(caption, func);
}

void MenuBar::createMenu(StringView caption, const TPopupFunc& menuFunc)
{
    createChild<MenuBarItem>(menuFunc, caption);
}

//---

MenuBarItem::MenuBarItem()
    : Button({ ButtonModeBit::EventOnClick, ButtonModeBit::NoFocus })
{}

MenuBarItem::MenuBarItem(const TPopupFunc& func, StringView text)
    : Button(text, { ButtonModeBit::EventOnClick, ButtonModeBit::NoFocus })
    , m_func(func)
{
}

void MenuBarItem::handleHoverEnter(const Position& absolutePosition)
{
    if (hasOtherPopups())
        openPopup();
}

void MenuBarItem::clicked()
{
    if (m_nextOpenPossible.reached())
        openPopup();
}

void MenuBarItem::openPopup()
{
    if (m_func)
    {
        if (auto popup = m_func())
        {
            closeOtherPopups();

            auto selfRef = RefWeakPtr<MenuBarItem>(this);
            popup->bind(EVENT_WINDOW_CLOSED) = [selfRef]()
            {
                if (auto menu = selfRef.lock())
                    menu->closePopup();
            };

            popup->show(this, PopupWindowSetup().autoClose().bottomLeft());
            m_openedPopup = popup;
            addStyleClass("opened"_id);
        }
    }
}

bool MenuBarItem::closePopup()
{
    removeStyleClass("opened"_id);

    if (auto popup = m_openedPopup.lock())
    {
        renderer()->dettachWindow(popup);
        m_openedPopup.reset();

        m_nextOpenPossible.resetToNow();
        m_nextOpenPossible += 0.1;

        return true;
    }

    return false;
}

bool MenuBarItem::hasOtherPopups()
{
    if (auto* parent = parentElement())
    {
        for (ElementChildIterator it(parent->childrenList()); it; ++it)
        {
            if (auto* otherButton = rtti_cast<MenuBarItem>(*it))
            {
                if (otherButton->m_openedPopup.lock())
                    return true;
            }
        }
    }

    return false;
}

void MenuBarItem::closeOtherPopups()
{
    if (auto* parent = parentElement())
    {
        for (ElementChildIterator it(parent->childrenList()); it; ++it)
        {
            if (auto* otherButton = rtti_cast<MenuBarItem>(*it))
            {
                if (otherButton->closePopup())
                {
                    if (auto* window = parent->findParentWindow())
                        window->requestActivate();
                    break;
                }
            }
        }
    }
}

RTTI_BEGIN_TYPE_CLASS(MenuBarItem);
    RTTI_METADATA(ElementClassNameMetadata).name("MenuBarItem");
RTTI_END_TYPE();

//---

RTTI_BEGIN_TYPE_CLASS(MenuButtonContainer);
    RTTI_METADATA(ElementClassNameMetadata).name("MenuButtonContainer");
RTTI_END_TYPE();

MenuButtonContainer::MenuButtonContainer()
{
    layoutVertical();
    enableAutoExpand(true, false);
}

PopupPtr MenuButtonContainer::convertToPopup()
{
    bool hasValidChildren = false;
    for (ElementChildIterator it(childrenList()); it; ++it)
    {
        if (it->styleType() != "MenuSeparator"_id)
        {
            hasValidChildren = true;
            break;
        }
    }

    if (hasValidChildren)
    {
        auto ret = RefNew<PopupWindow>();
        ret->attachChild(this);
        return ret;
    }
        
    return nullptr;
}

void MenuButtonContainer::show(IElement* owner)
{
    if (auto popup = convertToPopup())
        popup->show(owner, PopupWindowSetup().autoClose().relativeToCursor());
}

void MenuButtonContainer::showAsDropdown(IElement* owner)
{
    if (auto popup = convertToPopup())
        popup->show(owner, PopupWindowSetup().autoClose().bottomLeft());
}

EventFunctionBinder MenuButtonContainer::createCallback(StringView text, StringView icon /*= ""*/, StringView shortcut /*= ""*/, bool enabled /*= true*/)
{
    auto button = createChild<MenuButton>(text, icon, shortcut);
    button->enable(enabled);
    return button->bind(EVENT_MENU_ITEM_CLICKED);
}

void MenuButtonContainer::createSubMenu(const TPopupFunc& func, StringView text, StringView icon /*= ""*/)
{
    createChild<MenuButton>(func, text, icon);
}

void MenuButtonContainer::createSubMenu(const PopupPtr& popup, StringView text, StringView icon /*= ""*/)
{
    const auto func = [popup]() { return popup; };
    createChild<MenuButton>(func, text, icon);
}

void MenuButtonContainer::createSeparator()
{
    bool separator = false;
    for (auto it = childrenList(); it; ++it)
        separator = (it->styleType() == "MenuSeparator"_id);

    if (!separator)
        createChildWithType<IElement>("MenuSeparator"_id);
}

//---

static PopupWindow* FindRootPopupWindow(IElement* element)
{
    if (element)
    {
        if (auto* popup = rtti_cast<PopupWindow>(element->findParentWindow()))
        {
            auto ownerPopup = rtti_cast<PopupWindow>(popup->owner().lock());
            while (ownerPopup)
            {
                popup = ownerPopup.get();
                ownerPopup = rtti_cast<PopupWindow>(popup->owner().lock());
            }

            return popup;
        }
    }

    return nullptr;
}

static Window* FindRootWindow(IElement* element)
{
    if (element)
    {
        if (auto* popup = rtti_cast<PopupWindow>(element->findParentWindow()))
        {
            auto ownerPopup = rtti_cast<PopupWindow>(popup->owner().lock());
            while (ownerPopup)
            {
                popup = ownerPopup.get();
                ownerPopup = rtti_cast<PopupWindow>(popup->owner().lock());
            }

            if (auto owner = popup->owner().lock())
                return owner->findParentWindow();
        }
    }

    return nullptr;
}

MenuButton::MenuButton()
    : Button({ ButtonModeBit::EventOnClickRelease, ButtonModeBit::NoFocus })
{}

MenuButton::MenuButton(StringView text /*= ""*/, StringView icon /*= ""*/, StringView shortcut /*= ""*/)
    : Button({ ButtonModeBit::EventOnClickRelease, ButtonModeBit::NoFocus })
{
    if (icon.empty() && text.beginsWith("[img:"))
    {
        auto pos = text.findFirstChar(']');
        if (pos != INDEX_NONE)
        {
            icon = text.leftPart(pos+1);
            text = text.subString(pos+1).trim();
        }
    }

    createInternalNamedChild<TextLabel>("MenuIcon"_id, icon);
    createInternalNamedChild<TextLabel>("MenuCaption"_id, text);
    createInternalNamedChild<TextLabel>("MenuShortcut"_id, shortcut);

    if (auto* parentPopup = FindRootPopupWindow(this))
        parentPopup->requestClose();

    bind(EVENT_CLICKED) = [this]() {

        if (auto* parentPopup = FindRootPopupWindow(this))
            parentPopup->requestClose();

        if (auto* parentWindow = FindRootWindow(this))
        {
            parentWindow->requestActivate();

            auto buttonRef = RefPtr<MenuButton>(AddRef(this));
            RunSync("MenuCommand") << [buttonRef](FIBER_FUNC)
            {
                buttonRef->call(EVENT_MENU_ITEM_CLICKED);
            };
        }
    };
}

MenuButton::MenuButton(const TPopupFunc& func, StringView text, StringView icon)
    : Button({ ButtonModeBit::EventOnClickRelease, ButtonModeBit::NoFocus })
    , m_func(func)
{
    createInternalNamedChild<TextLabel>("MenuIcon"_id, icon);
    createInternalNamedChild<TextLabel>("MenuCaption"_id, text);
    createInternalNamedChild<TextLabel>("MenuSubMenuIcon"_id);
}

bool MenuButton::closePopup()
{
    if (auto popup = m_openedPopup.lock())
    {
        popup->requestClose();
        m_openedPopup.reset();
        return true;
    }

    return false;
}

void MenuButton::closeOtherPopups()
{
    if (auto* parent = parentElement())
    {
        for (ElementChildIterator it(parent->childrenList()); it; ++it)
        {
            if (auto* otherButton = rtti_cast<MenuButton>(*it))
            {
                if (otherButton->closePopup())
                {
                    if (auto* window = parent->findParentWindow())
                        window->requestActivate();
                    break;
                }
            }
        }
    }
}

bool MenuButton::handleHoverDuration(const Position& absolutePosition)
{
    if (!m_openedPopup.empty() && !m_openedPopup.expired())
        return true;

    closeOtherPopups();

    if (m_func)
    {
        if (auto popup = m_func())
        {
            auto selfRef = RefWeakPtr<MenuButton>(this);

            popup->bind(EVENT_WINDOW_CLOSED) = [selfRef]()
            {
                if (auto menu = selfRef.lock())
                    menu->m_openedPopup.reset();
            };

            popup->show(this, PopupWindowSetup().autoClose().topRightNeighbour());

            m_openedPopup = popup;
            return true;
        }
    }

    return false;
}

RTTI_BEGIN_TYPE_CLASS(MenuButton);
    RTTI_METADATA(ElementClassNameMetadata).name("MenuButton");
RTTI_END_TYPE();

//---

END_BOOMER_NAMESPACE_EX(ui)
