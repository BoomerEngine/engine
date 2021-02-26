/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: font #]
***/

#pragma once

#include "core/containers/include/inplaceArray.h"

BEGIN_BOOMER_NAMESPACE_EX(font)

//---

/// input for font processing functions
/// kind of a StringView but handles both the ANSI and UTF16
/// NOTE: the view is not copyable, should be consumed directly
class ENGINE_FONT_API FontInputText : public NoCopy
{
public:
    FontInputText(); // empty string
    FontInputText(const char* str, uint32_t length = ~(uint32_t)0);
    FontInputText(const wchar_t* str, uint32_t length = ~(uint32_t)0);
    FontInputText(const StringBuf& str, uint32_t offset = 0, uint32_t length = ~(uint32_t)0);
    FontInputText(const UTF16StringVector& str, uint32_t offset = 0, uint32_t length = ~(uint32_t)0);

    /// is the text buffer empty ?
    INLINE bool empty() const { return m_chars.empty(); }

    /// get character codes
    INLINE const Array<uint32_t>& chars() const { return m_chars; }

private:
    InplaceArray<uint32_t, 128> m_chars;

    void build(const char* txt, const char* endTxt);
    void build(const wchar_t* txt, const wchar_t* endTxt);
};

END_BOOMER_NAMESPACE_EX(font)
