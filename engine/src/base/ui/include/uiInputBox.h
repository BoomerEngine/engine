/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: modal #]
***/

#pragma once

namespace ui
{
    ///----

    /// input box setup
    class BASE_UI_API InputBoxSetup
    {
    public:
        InputBoxSetup();

        base::StringBuf m_title;
        base::StringBuf m_message; // if empty we won't add the TextLabel
        base::StringBuf m_hint;
        TInputValidationFunction m_validation;
        bool m_multiline = false;

        INLINE InputBoxSetup& title(base::StringView<char> txt) { m_title = base::StringBuf(txt); return *this; }
        INLINE InputBoxSetup& message(base::StringView<char> txt) { m_message = base::StringBuf(txt); return *this; }
        INLINE InputBoxSetup& hint(base::StringView<char> txt) { m_hint = base::StringBuf(txt); return *this; }
        INLINE InputBoxSetup& multiline(bool flag = true) { m_multiline = flag; return *this; }
    };

    ///----

    /// Show input box window, locks UI
    /// Returns true if the text in the "text" argument was modified
    extern BASE_UI_API bool ShowInputBox(IElement* owner, const InputBoxSetup& setup, base::StringBuf& inOutText);

    ///----

} // ui