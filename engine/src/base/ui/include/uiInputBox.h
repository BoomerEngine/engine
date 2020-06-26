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

        //--

        /// set title string
        InputBoxSetup& title(const char* txt);

        /// set message string
        InputBoxSetup& message(const char* txt);

        //--

        /// get the title string
        INLINE const base::StringBuf& title() const { return m_title; }

        /// get the caption string
        INLINE const base::StringBuf& caption() const { return m_caption; }

    private:
        base::StringBuf m_title;
        base::StringBuf m_caption;
    };

    ///----

    /// Show input box window, locks UI
    /// Returns true if the text in the "text" inout argument was modified
    extern BASE_UI_API bool ShowInputBox(const ElementPtr& parent, const InputBoxSetup& setup, base::StringBuf& text);

    ///----

} // ui