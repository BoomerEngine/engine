/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: modal #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(ui)

///----

/// input box setup
class ENGINE_UI_API InputBoxSetup
{
public:
    InputBoxSetup();

    StringBuf m_title;
    StringBuf m_message; // if empty we won't add the TextLabel
    StringBuf m_hint;
    TInputValidationFunction m_validation;
    bool m_multiline = false;

    INLINE InputBoxSetup& title(StringView txt) { m_title = StringBuf(txt); return *this; }
    INLINE InputBoxSetup& message(StringView txt) { m_message = StringBuf(txt); return *this; }
    INLINE InputBoxSetup& hint(StringView txt) { m_hint = StringBuf(txt); return *this; }
    INLINE InputBoxSetup& multiline(bool flag = true) { m_multiline = flag; return *this; }

    InputBoxSetup& fileNameValidation(bool withExt = false);
};

///----

/// Show input box window, locks UI
/// Returns true if the text in the "text" argument was modified
extern ENGINE_UI_API bool ShowInputBox(IElement* owner, const InputBoxSetup& setup, StringBuf& inOutText);

///----

END_BOOMER_NAMESPACE_EX(ui)
