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

    //---

    MessageBoxSetup::MessageBoxSetup()
        : m_buttons(0)
        , m_defaultButton(MessageButton::None)
        , m_title("Message")
        , m_caption("Message Text")
        , m_type(MessageType::Info)
    {}

    //---

    class MessageBox : public PopupWindow
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MessageBox, PopupWindow);

    public:
        MessageBox(ElementWeakPtr owner);

        // configure message box
        void configure(const MessageBoxSetup& setup);

    private:
        ElementWeakPtr m_owner;
        uint8_t m_buttonMask = 0;

        virtual bool handleKeyEvent(const base::input::KeyEvent& evt) override;
        virtual void queryInitialPlacementSetup(WindowInitialPlacementSetup& outSetup) const override;
        void closeWithCode(MessageButton code);
    };

    MessageBox::MessageBox(ElementWeakPtr owner)
        : m_owner(owner)
    {
        layoutVertical();
        createChild<WindowTitleBar>()->addStyleClass("mini"_id);
    }

    void MessageBox::closeWithCode(MessageButton code)
    {
        call("OnResult"_id, code);
        requestClose();

        if (auto owner = m_owner.lock())
            owner->focus();
    }

    void MessageBox::configure(const MessageBoxSetup& setup)
    {
        // set title
        requestTitleChange(setup.title());

        // message area
        {
            auto box = createChild<ui::IElement>();
            box->layoutHorizontal();

            if (setup.type() == MessageType::Warning)
                box->createChild<ui::TextLabel>("[img:warning48]")->customMargins(20, 20, 10, 10);
            else if (setup.type() == MessageType::Error)
                box->createChild<ui::TextLabel>("[img:error48]")->customMargins(20, 20, 10, 10);
            else if (setup.type() == MessageType::Question)
                box->createChild<ui::TextLabel>("[img:help48]")->customMargins(20, 20, 10, 10);
            else if (setup.type() == MessageType::Info)
                box->createChild<ui::TextLabel>("[img:info48]")->customMargins(20, 20, 10, 10);

            auto text = box->createChild<ui::TextLabel>(setup.caption());
            text->customVerticalAligment(ElementVerticalLayout::Middle);
            text->customHorizontalAligment(ElementHorizontalLayout::Expand);
            text->customMargins(15, 15, 15, 15);
        }

        // buttons
        {
            auto buttons = createChild<ui::IElement>();
            buttons->layoutHorizontal();
            buttons->customHorizontalAligment(ElementHorizontalLayout::Center);
            buttons->customMargins(5, 5, 5, 15);

            if (setup.hasButton(MessageButton::OK) || (setup.buttonMask() == 0))
            {
                auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "[img:tick] OK");
                button->bind("OnClick"_id) = [this]() { closeWithCode(MessageButton::OK); };
            }

            if (setup.hasButton(MessageButton::Cancel))
            {
                auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "[img:cancel] Cancel");
                button->bind("OnClick"_id) = [this]() { closeWithCode(MessageButton::Cancel); };
            }

            if (setup.hasButton(MessageButton::Yes))
            {
                auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "[img:accept] Yes");
                button->bind("OnClick"_id) = [this]() { closeWithCode(MessageButton::Yes); };
            }

            if (setup.hasButton(MessageButton::No))
            {
                auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "[img:cross] No");
                button->bind("OnClick"_id) = [this]() { closeWithCode(MessageButton::No); };
            }

            m_buttonMask = setup.buttonMask();
        }
    }

    void MessageBox::queryInitialPlacementSetup(WindowInitialPlacementSetup& outSetup) const
    {
        TBaseClass::queryInitialPlacementSetup(outSetup);

        outSetup.mode = WindowInitialPlacementMode::WindowCenter;
    }

    bool MessageBox::handleKeyEvent(const base::input::KeyEvent& evt)
    {
        if (evt.pressed())
        {
            if (evt.keyCode() == base::input::KeyCode::KEY_RETURN)
            {
                if (m_buttonMask & (uint8_t)MessageButton::OK)
                {
                    closeWithCode(MessageButton::OK);
                    return true;
                }
                else if (m_buttonMask & (uint8_t)MessageButton::Yes)
                {
                    closeWithCode(MessageButton::Yes);
                    return true;
                }
            }
            else if (evt.keyCode() == base::input::KeyCode::KEY_ESCAPE)
            {
                if (m_buttonMask & (uint8_t)MessageButton::Cancel)
                {
                    closeWithCode(MessageButton::Cancel);
                    return true;
                }
                else if (m_buttonMask & (uint8_t)MessageButton::No)
                {
                    closeWithCode(MessageButton::No);
                    return true;
                }
            }
            else if (evt.keyCode() == base::input::KeyCode::KEY_Y)
            {
                if (m_buttonMask & (uint8_t)MessageButton::Yes)
                {
                    closeWithCode(MessageButton::Yes);
                    return true;
                }
            }
            else if (evt.keyCode() == base::input::KeyCode::KEY_N)
            {
                if (m_buttonMask & (uint8_t)MessageButton::No)
                {
                    closeWithCode(MessageButton::No);
                    return true;
                }
            }
        }

        return TBaseClass::handleKeyEvent(evt);
    }

    RTTI_BEGIN_TYPE_NATIVE_CLASS(MessageBox);
    RTTI_END_TYPE();

    //---

    EventFunctionBinder ShowMessageBox(IElement* parent, const MessageBoxSetup& setup)
    {
        DEBUG_CHECK(parent != nullptr);
        if (parent)
        {
            auto window = base::CreateSharedPtr<MessageBox>(parent);
            window->configure(setup);

            auto renderer = parent->renderer();
            DEBUG_CHECK_EX(renderer, "Cannot create popups for elements that are not attached to renderig hierarchy");
            if (renderer)
            {
                renderer->attachWindow(window);
                return window->bind("OnResult"_id, parent);
            }
        }

        return EventFunctionBinder(nullptr);
    }

    //---

} // ui

