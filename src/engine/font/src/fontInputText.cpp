/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: font #]
***/

#include "build.h"
#include "fontInputText.h"
#include "core/containers/include/stringParser.h"
#include "core/containers/include/utf8StringFunctions.h"

BEGIN_BOOMER_NAMESPACE()

FontInputText::FontInputText()
{}

FontInputText::FontInputText(const char* str, uint32_t length /*= ~(uint32_t)0*/)
{
    auto actualLength = str ? std::min<uint32_t>(length, (uint32_t)strlen(str)) : 0;
    build(str, str + actualLength);
}

FontInputText::FontInputText(const wchar_t* str, uint32_t length /*= ~(uint32_t)0*/)
{
    auto actualLength = str ? std::min<uint32_t>(length, (uint32_t)wcslen(str)) : 0;
    build(str, str + actualLength);
}

FontInputText::FontInputText(const StringBuf& str, uint32_t offset /*= 0*/, uint32_t length /*= ~(uint32_t)0*/)
{
    auto actualLength = std::min<uint32_t>(length, std::max<int>(0, (int)str.length() - (int)offset));
    build(str.c_str(), str.c_str() + actualLength);
}

FontInputText::FontInputText(const UTF16StringVector& str, uint32_t offset /*= 0*/, uint32_t length /*= ~(uint32_t)0*/)
{
    auto actualLength = std::min<uint32_t>(length, std::max<int>(0, (int)str.length() - (int)offset));
    build(str.c_str(), str.c_str() + actualLength);
}

void FontInputText::build(const char* txt, const char* endTxt)
{
    while (txt < endTxt)
    {
        auto ch = utf8::NextChar(txt);
        m_chars.pushBack(ch);
    }
}

void FontInputText::build(const wchar_t* txt, const wchar_t* endTxt)
{
    while (txt < endTxt)
    {
        auto ch = *txt++;
        m_chars.pushBack(ch);
    }
}

END_BOOMER_NAMESPACE()
