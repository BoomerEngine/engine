/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: modal #]
***/

#include "build.h"
#include "uiMessageBox.h"
#include "uiButton.h"
#include "uiTextLabel.h"
#include "uiWindow.h"
#include "uiImage.h"
#include "uiRenderer.h"

#include "base/containers/include/stringBuilder.h"
#include "uiWindowPopup.h"

namespace ui
{
    //---

    RTTI_BEGIN_TYPE_ENUM(MessageButton);
        RTTI_ENUM_OPTION(None);
        RTTI_ENUM_OPTION(Yes);
        RTTI_ENUM_OPTION(No);
        RTTI_ENUM_OPTION(Cancel);
        RTTI_ENUM_OPTION(OK);
    RTTI_END_TYPE();

    //----

    MessageBoxSetup::MessageBoxSetup()
    {
        m_captions[(int)MessageButton::Yes] = "Yes";
        m_captions[(int)MessageButton::No] = "No";
        m_captions[(int)MessageButton::OK] = "OK";
        m_captions[(int)MessageButton::Cancel] = "Cancel";
        m_captions[(int)MessageButton::None] = "None";
    }

    //---

    static MessageButton DetermineEscapeButton(const MessageBoxSetup& setup)
    {
        if (setup.hasButton(MessageButton::Cancel))
            return MessageButton::Cancel;

        if (setup.hasButton(MessageButton::No))
            return MessageButton::No;

        if (setup.hasButton(MessageButton::OK) && setup.m_defaultButton == MessageButton::OK)
            return MessageButton::OK;

        if (setup.m_buttons == 0)
            return MessageButton::OK;

        return MessageButton::None;
    }

    static MessageButton DetermineAcceptButton(const MessageBoxSetup& setup)
    {
        if ((setup.m_defaultButton != MessageButton::None) && setup.hasButton(setup.m_defaultButton))
            return setup.m_defaultButton;

        if (setup.m_buttons == 0)
            return MessageButton::OK;

        return MessageButton::None;
    }

    ButtonPtr MakeButton(Window* window, IElement* container, MessageButton type, const MessageBoxSetup& setup)
    {
        auto caption = setup.m_captions[(int)type];
        auto button = container->createChildWithType<ui::Button>("PushButton"_id, caption);

        if (setup.m_constructiveButton == type) button->addStyleClass("green"_id);
        if (setup.m_destructiveButton == type) button->addStyleClass("red"_id);
        button->bind(EVENT_CLICKED) = [window, type]() { window->requestClose((int)type); };
        return button;
    }

    MessageButton ShowMessageBox(IElement* owner, const MessageBoxSetup& setup)
    {
        DEBUG_CHECK_RETURN_V(owner, setup.m_defaultButton);
        DEBUG_CHECK_RETURN_V(owner->renderer(), setup.m_defaultButton);

        auto window = base::CreateSharedPtr<Window>(WindowFeatureFlagBit::DEFAULT_DIALOG, setup.m_title);
        auto windowRef = window.get();

        // close with Escape
        const auto escapeButton = DetermineEscapeButton(setup);
        if (escapeButton != MessageButton::None)
        {
            window->actions().bindCommand("Cancel"_id) = [windowRef, escapeButton]() { windowRef->requestClose((int)escapeButton); };
            window->actions().bindShortcut("Cancel"_id, "Escape");
        }

        // accept with Enter
        const auto acceptButton = DetermineAcceptButton(setup);
        if (acceptButton != MessageButton::None)
        {
            window->actions().bindCommand("Accept"_id) = [windowRef, acceptButton]() { windowRef->requestClose((int)acceptButton); };
            window->actions().bindShortcut("Accept"_id, "Enter");
        }

        // message area
        {
            auto box = window->createChild<ui::IElement>();
            box->layoutHorizontal();

            const char* iconName = "[img:info48]";
            switch (setup.m_type)
            {
            case MessageType::Warning:
                iconName = "[img:warning48]";
                break;

            case MessageType::Error:
                iconName = "[img:error48]";
                break;

            case MessageType::Question:
                iconName = "[img:help48]";
                break;
            }

            box->createChild<ui::TextLabel>(iconName)->customMargins(20, 20, 10, 10);

            auto text = box->createChild<ui::TextLabel>(setup.m_message);
            text->customVerticalAligment(ElementVerticalLayout::Middle);
            text->customHorizontalAligment(ElementHorizontalLayout::Expand);
            text->customMargins(15, 15, 15, 15);
        }

        // buttons
        {
            auto buttons = window->createChild();
            buttons->layoutHorizontal();
            buttons->customHorizontalAligment(ElementHorizontalLayout::Center);
            buttons->customMargins(5, 5, 5, 15);

            if (setup.hasButton(MessageButton::Yes))
                MakeButton(window, buttons, MessageButton::Yes, setup);

            if (setup.hasButton(MessageButton::No))
                MakeButton(window, buttons, MessageButton::No, setup);

            if (setup.hasButton(MessageButton::OK))
                MakeButton(window, buttons, MessageButton::OK, setup);

            if (setup.hasButton(MessageButton::Cancel))
                MakeButton(window, buttons, MessageButton::Cancel, setup);
        }

        owner->renderer()->playSound(setup.m_type);

        return (MessageButton)window->runModal(owner);
    }

    //---

} // ui

