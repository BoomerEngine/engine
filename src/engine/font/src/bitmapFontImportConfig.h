/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: bitmapFont #]
***/

#pragma once

#include "core/resource/include/resource.h"
#include "core/resource/include/resourceMetadata.h"

BEGIN_BOOMER_NAMESPACE_EX(font)

/// char set
struct ENGINE_FONT_API BitmapFontCharset
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(BitmapFontCharset);

public:
    bool enabled = false;
    uint32_t firstCharCode = 0;
    uint32_t lastCharCode = 0;

    void collectCharCodes(HashSet<uint32_t>& outCharCodes) const;
};

/// manifest for cooking textures
class ENGINE_FONT_API BitmapFontImportConfig : public res::ResourceConfiguration
{
    RTTI_DECLARE_VIRTUAL_CLASS(BitmapFontImportConfig, res::ResourceConfiguration);

public:
    BitmapFontImportConfig();

    //--

    uint32_t m_size = 12;
    bool m_bold = false;
    bool m_italic = false;

    //--

    BitmapFontCharset m_setASCII;

    Array<BitmapFontCharset> m_additionalCharSets;

    //--

    void collectCharCodes(Array<uint32_t>& outCharCodes) const;
};

END_BOOMER_NAMESPACE_EX(font)
