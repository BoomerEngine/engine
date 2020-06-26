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

namespace ui
{
    
    //---

    RTTI_BEGIN_TYPE_CLASS(MenuBar);
        RTTI_METADATA(ElementClassNameMetadata).name("MenuBar");
    RTTI_END_TYPE();

    MenuBar::MenuBar()
    {
        layoutHorizontal();
    }

    void MenuBar::createMenu(base::StringView<char> caption, const PopupPtr& menu)
    {
        auto func = [menu]() { return menu; };
        createMenu(caption, func);
    }

    void MenuBar::createMenu(base::StringView<char> caption, const TPopupFunc& menuFunc)
    {
        createChild<MenuBarItem>(menuFunc, caption);
    }

    bool MenuBar::handleTemplateProperty(base::StringView<char> name, base::StringView<char> value)
    {
        return TBaseClass::handleTemplateProperty(name, value);
    }

    bool MenuBar::handleTemplateChild(base::StringView<char> name, const base::xml::IDocument& doc, const base::xml::NodeID& id)
    {
        return TBaseClass::handleTemplateChild(name, doc, id);
    }

    bool MenuBar::handleTemplateNewChild(const base::xml::IDocument& doc, const base::xml::NodeID& id, const base::xml::NodeID& childId, const ElementPtr& childElement)
    {
        return TBaseClass::handleTemplateNewChild(doc, id, childId, childElement);
    }

    bool MenuBar::handleTemplateFinalize()
    {
        return TBaseClass::handleTemplateFinalize();
    }

    //---

    MenuBarItem::MenuBarItem()
        : Button({ ButtonModeBit::EventOnClick, ButtonModeBit::NoFocus })
    {}

    MenuBarItem::MenuBarItem(const TPopupFunc& func, base::StringView<char> text)
        : Button(text, { ButtonModeBit::EventOnClick, ButtonModeBit::NoFocus })
        , m_func(func)
    {
    }

    bool MenuBarItem::handleTemplateProperty(base::StringView<char> name, base::StringView<char> value)
    {
        return TBaseClass::handleTemplateProperty(name, value);
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

                popup->bind("OnClosed"_id, this) = [](MenuBarItem* menu)
                {
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
                if (auto* otherButton = base::rtti_cast<MenuBarItem>(*it))
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
                if (auto* otherButton = base::rtti_cast<MenuBarItem>(*it))
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
        : m_timerUpdateState(this)
    {
        layoutVertical();
        enableAutoExpand(true, false);

        m_timerUpdateState = [this]() { updateButtonState(); };
        m_timerUpdateState.startOneShot(0.0001f);
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
            auto ret = base::CreateSharedPtr<PopupWindow>();
            ret->attachChild(this);
            return ret;
        }
        
        return nullptr;
    }

    void MenuButtonContainer::show(IElement* owner)
    {
        if (auto popup = convertToPopup())
            popup->show(owner, ui::PopupWindowSetup().autoClose().relativeToCursor());
    }

    void MenuButtonContainer::createAction(base::StringID action, base::StringView<char> text, base::StringView<char> icon /*= ""*/)
    {
        createChild<MenuButton>(action, text, icon);
    }

    EventFunctionBinder MenuButtonContainer::createCallback(base::StringView<char> text, base::StringView<char> icon /*= ""*/, base::StringView<char> shortcut /*= ""*/)
    {
        auto button = createChild<MenuButton>(base::StringID(), text, icon, shortcut);
        return button->bind("OnMenuItemClick"_id);
    }

    void MenuButtonContainer::createSubMenu(const TPopupFunc& func, base::StringView<char> text, base::StringView<char> icon /*= ""*/)
    {
        createChild<MenuButton>(func, text, icon);
    }

    void MenuButtonContainer::createSubMenu(const PopupPtr& popup, base::StringView<char> text, base::StringView<char> icon /*= ""*/)
    {
        const auto func = [popup]() { return popup; };
        createChild<MenuButton>(func, text, icon);
    }

    void MenuButtonContainer::createSeparator()
    {
        createChildWithType<IElement>("MenuSeparator"_id);
    }

    void MenuButtonContainer::updateButtonState()
    {
        bool separatorAllowed = false;
        for (ElementChildIterator it(childrenList()); it; ++it)
        {
            if (const auto actionName = it->evalStyleValue<base::StringID>("action"_id))
            {
                auto actionState = checkAction(actionName);

                bool enabled = actionState.test(ActionStatusBit::Defined) && actionState.test(ActionStatusBit::Enabled);
                it->enable(enabled);

                if (auto button = base::rtti_cast<Button>(*it))
                    button->toggle(actionState.test(ActionStatusBit::Toggled));
            }

            /*if (it->styleType() == "MenuSeparator"_id)
            {
                it->visibility(separatorAllowed);
                separatorAllowed = false;
            }
            else
            {
                if (const auto actionName = it->evalStyleValue<base::StringID>("action"_id))
                {
                    auto actionState = checkAction(actionName);
                    bool visible = actionState.test(ActionStatusBit::Defined);
                    it->visibility(visible);

                    if (visible)
                    {
                        it->enable(actionState.test(ActionStatusBit::Enabled));

                        if (auto button = base::rtti_cast<Button>(*it))
                            button->toggle(actionState.test(ActionStatusBit::Toggled));
                    }
                }
                else
                {
                    it->visibility(true);
                    separatorAllowed = true;
                }
            }*/
        }
    }

    bool MenuButtonContainer::handleTemplateProperty(base::StringView<char> name, base::StringView<char> value)
    {
        return TBaseClass::handleTemplateProperty(name, value);
    }

    bool MenuButtonContainer::handleTemplateChild(base::StringView<char> name, const base::xml::IDocument& doc, const base::xml::NodeID& id)
    {
        return TBaseClass::handleTemplateChild(name, doc, id);
    }

    bool MenuButtonContainer::handleTemplateNewChild(const base::xml::IDocument& doc, const base::xml::NodeID& id, const base::xml::NodeID& childId, const ElementPtr& childElement)
    {
        return TBaseClass::handleTemplateNewChild(doc, id, childId, childElement);
    }

    bool MenuButtonContainer::handleTemplateFinalize()
    {
        return TBaseClass::handleTemplateFinalize();
    }

    //---

    PopupWindow* FindRootPopupWindow(IElement* element)
    {
        if (element)
        {
            if (auto* popup = base::rtti_cast<PopupWindow>(element->findParentWindow()))
            {
                auto ownerPopup = base::rtti_cast<PopupWindow>(popup->owner().lock());
                while (ownerPopup)
                {
                    popup = ownerPopup.get();
                    ownerPopup = base::rtti_cast<PopupWindow>(popup->owner().lock());
                }

                return popup;
            }
        }

        return nullptr;
    }

    Window* FindRootWindow(IElement* element)
    {
        if (element)
        {
            if (auto* popup = base::rtti_cast<PopupWindow>(element->findParentWindow()))
            {
                auto ownerPopup = base::rtti_cast<PopupWindow>(popup->owner().lock());
                while (ownerPopup)
                {
                    popup = ownerPopup.get();
                    ownerPopup = base::rtti_cast<PopupWindow>(popup->owner().lock());
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

    MenuButton::MenuButton(base::StringID action /*= base::StringID()*/, base::StringView<char> text /*= ""*/, base::StringView<char> icon /*= ""*/, base::StringView<char> shortcut /*= ""*/)
        : Button({ ButtonModeBit::EventOnClickRelease, ButtonModeBit::NoFocus })
    {
        createInternalNamedChild<TextLabel>("MenuIcon"_id, icon);
        createInternalNamedChild<TextLabel>("MenuCaption"_id, text ? text : action.view());
        createInternalNamedChild<TextLabel>("MenuShortcut"_id, shortcut);

        if (action)
        {
            customStyle("action"_id, action);

            bind("OnClick"_id, this) = [](MenuButton* button) {

                if (auto* parentPopup = FindRootPopupWindow(button))
                    parentPopup->requestClose();

                if (auto* parentWindow = FindRootWindow(button))
                    parentWindow->requestActivate();

                if (auto action = button->evalStyleValue<base::StringID>("action"_id))
                    button->runAction(action);
            };
        }
        else
        {
            bind("OnClick"_id, this) = [](MenuButton* button) {

                if (auto* parentPopup = FindRootPopupWindow(button))
                    parentPopup->requestClose();

                if (auto* parentWindow = FindRootWindow(button))
                    parentWindow->requestActivate();

                button->call("OnMenuItemClick"_id);
            };
        }
    }

    MenuButton::MenuButton(const TPopupFunc& func, base::StringView<char> text, base::StringView<char> icon)
        : Button({ ButtonModeBit::EventOnClickRelease, ButtonModeBit::NoFocus })
        , m_func(func)
    {
        createInternalNamedChild<TextLabel>("MenuIcon"_id, icon);
        createInternalNamedChild<TextLabel>("MenuCaption"_id, text);
        createInternalNamedChild<TextLabel>("MenuSubMenuIcon"_id);
    }

    bool MenuButton::handleTemplateProperty(base::StringView<char> name, base::StringView<char> value)
    {
        return TBaseClass::handleTemplateProperty(name, value);
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
                if (auto* otherButton = base::rtti_cast<MenuButton>(*it))
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
                popup->bind("OnClosed"_id, this) = [](MenuButton* menu)
                {
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

} // ui