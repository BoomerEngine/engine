/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: modal #]
***/

#pragma once

#include "uiEventFunction.h"

BEGIN_BOOMER_NAMESPACE(ui)

///----

/// message button
enum class MessageButton
{
    None,
    Yes,
    No,
    Cancel,
    OK,

    MAX,
};

/// message box setup
class BASE_UI_API MessageBoxSetup
{
public:
    MessageBoxSetup();

    base::StringBuf m_title;
    base::StringBuf m_message;
    MessageType m_type = MessageType::Info;
    uint8_t m_buttons = 0;
    MessageButton m_defaultButton = MessageButton::OK;
    MessageButton m_destructiveButton = MessageButton::None;
    MessageButton m_constructiveButton = MessageButton::None;

    base::StringBuf m_captions[(int)MessageButton::MAX];

    INLINE MessageBoxSetup& type(MessageType messageType) { m_type = messageType; return *this; }
    INLINE MessageBoxSetup& info() { m_type = MessageType::Info; return *this; }
    INLINE MessageBoxSetup& warn() { m_type = MessageType::Warning; return *this; }
    INLINE MessageBoxSetup& error() { m_type = MessageType::Error; return *this; }
    INLINE MessageBoxSetup& question() { m_type = MessageType::Question; return *this; }

    INLINE MessageBoxSetup& button(MessageButton button) { if (button != MessageButton::None) m_buttons |= 1 << (uint8_t)button; return *this; }
    INLINE MessageBoxSetup& yes() { button(MessageButton::Yes); return *this; }
    INLINE MessageBoxSetup& no() { button(MessageButton::No); return *this; }
    INLINE MessageBoxSetup& ok() { button(MessageButton::OK); return *this; }
    INLINE MessageBoxSetup& cancel() { button(MessageButton::Cancel); return *this; }
    INLINE MessageBoxSetup& caption(MessageButton button, base::StringView txt) { m_captions[(int)button] = base::StringBuf(txt); return *this; }

    INLINE MessageBoxSetup& defaultYes() { m_defaultButton = MessageButton::Yes; return *this; }
    INLINE MessageBoxSetup& defaultNo() { m_defaultButton = MessageButton::No; return *this; }
    INLINE MessageBoxSetup& defaultOK() { m_defaultButton = MessageButton::OK; return *this; }
    INLINE MessageBoxSetup& defaultCancel() { m_defaultButton = MessageButton::Cancel; return *this; }

    INLINE MessageBoxSetup& title(base::StringView txt) { m_title = base::StringBuf(txt); return *this; }
    INLINE MessageBoxSetup& message(base::StringView txt) { m_message = base::StringBuf(txt); return *this; }

    INLINE bool hasButton(MessageButton button) const { return 0 != (m_buttons & (1 << (uint8_t)button)); }
};

///----

/// Show a little nice MODAL message box, old-school way but still works best
/// This functions returns the button pressed
extern BASE_UI_API MessageButton ShowMessageBox(IElement* parent, const MessageBoxSetup& setup);

///----

END_BOOMER_NAMESPACE(ui)
