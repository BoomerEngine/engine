/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: bitmapFont #]
***/

#include "build.h"
#include "bitmapFontImportConfig.h"

BEGIN_BOOMER_NAMESPACE_EX(font)

//---

RTTI_BEGIN_TYPE_STRUCT(BitmapFontCharset)
    RTTI_PROPERTY(enabled).editable("Generate glyphs from this charset");
    RTTI_PROPERTY(firstCharCode).editable("Unicode for first character of the set");
    RTTI_PROPERTY(lastCharCode).editable("Unicode for last character of the set");
RTTI_END_TYPE();

void BitmapFontCharset::collectCharCodes(HashSet<uint32_t>& outCharCodes) const
{
    if (enabled)
    {
        for (uint32_t i = firstCharCode; i <= lastCharCode; ++i)
            outCharCodes.insert(i);
    }
}

//---

RTTI_BEGIN_TYPE_CLASS(BitmapFontImportConfig);
    RTTI_CATEGORY("Font");
    RTTI_PROPERTY(m_size).editable("Size of the font glyphs to generate");
    RTTI_PROPERTY(m_bold).editable("Should we make the glyphs bold");
    RTTI_PROPERTY(m_italic).editable("Should we make the glyphs slanted");
    RTTI_CATEGORY("Ranges");
    RTTI_PROPERTY(m_setASCII).editable();
    RTTI_PROPERTY(m_additionalCharSets).editable("Additional custom char sets to export");
    RTTI_END_TYPE();

    BitmapFontImportConfig::BitmapFontImportConfig()
    {
        m_setASCII.enabled = true;
        m_setASCII.firstCharCode = 32;
        m_setASCII.lastCharCode = 255;
    }

    void BitmapFontImportConfig::collectCharCodes(Array<uint32_t>& outCharCodes) const
    {
        HashSet<uint32_t> chars;

        m_setASCII.collectCharCodes(chars);

        for (auto& charSet : m_additionalCharSets)
            charSet.collectCharCodes(chars);

        outCharCodes = chars.keys();
        std::sort(outCharCodes.begin(), outCharCodes.end());
    }

//---

END_BOOMER_NAMESPACE_EX(font)
