/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: modal #]
***/

#pragma once

#include "uiEventFunction.h"

namespace ui
{
    ///----

    /// message box setup
    class BASE_UI_API MessageBoxSetup
    {
    public:
        MessageBoxSetup();

        //--

        /// set message type
        INLINE MessageBoxSetup& type(MessageType messageType)
        {
            m_type = messageType;
            return *this;
        }

        /// set message type to informative message
        INLINE MessageBoxSetup& info()
        {
            m_type = MessageType::Info;
            return *this;
        }

        /// set message type to warning message
        INLINE MessageBoxSetup& warn()
        {
            m_type = MessageType::Warning;
            return *this;
        }

        /// set message type to error message
        INLINE MessageBoxSetup& error()
        {
            m_type = MessageType::Error;
            return *this;
        }

        /// set message type to question message
        INLINE MessageBoxSetup& question()
        {
            m_type = MessageType::Question;
            return *this;
        }

        /// enable button
        INLINE MessageBoxSetup& button(MessageButton button)
        {
            if (button != MessageButton::None)
                m_buttons |= 1 << (uint8_t)button;
            return *this;
        }

        /// enable the Yes button
        INLINE MessageBoxSetup& yes()
        {
            return button(MessageButton::Yes);
        }

        /// enable the Yes button
        INLINE MessageBoxSetup& no()
        {
            return button(MessageButton::No);
        }

        /// enable the OK button
        INLINE MessageBoxSetup& ok()
        {
            return button(MessageButton::OK);
        }

        /// enable the Cancel button
        INLINE MessageBoxSetup& cancel()
        {
            return button(MessageButton::Cancel);
        }

        /// set the default button to Yes
        INLINE MessageBoxSetup& defaultYes()
        {
            m_defaultButton = MessageButton::Yes;
            return *this;
        }

        /// set the default button to No
        INLINE MessageBoxSetup& defaultNo()
        {
            m_defaultButton = MessageButton::No;
            return *this;
        }

        /// set the default button to Cancel
        INLINE MessageBoxSetup& defaultCancel()
        {
            m_defaultButton = MessageButton::Cancel;
            return *this;
        }

        /// set the default button to OK
        INLINE MessageBoxSetup& defaultOK()
        {
            m_defaultButton = MessageButton::OK;
            return *this;
        }

        /// set title string
        INLINE MessageBoxSetup& title(const char* txt)
        {
            m_title = txt;
            return *this;
        }

        /// set message string
        INLINE MessageBoxSetup& message(const char* txt)
        {
            m_caption = txt;
            return *this;
        }

        //--

        /// get the title string
        INLINE const base::StringBuf& title() const { return m_title; }

        /// get the caption string
        INLINE const base::StringBuf& caption() const { return m_caption; }

        /// get the message box type
        INLINE MessageType type() const { return m_type; }

        /// get the button mask
        INLINE uint8_t buttonMask() const { return m_buttons; }

        /// get the button mask
        INLINE bool hasButton(MessageButton button) const { return 0 != (m_buttons & (1 << (uint8_t)button)); }

        /// get the default button
        INLINE MessageButton defaultButton() const { return m_defaultButton; }

    private:
        base::StringBuf m_title;
        base::StringBuf m_caption;
        MessageType m_type;
        uint8_t m_buttons;
        MessageButton m_defaultButton;
    };

    ///----

    /// Show message box window, locks UI
    extern BASE_UI_API EventFunctionBinder ShowMessageBox(IElement* parent, const MessageBoxSetup& setup);

    ///----

} // ui