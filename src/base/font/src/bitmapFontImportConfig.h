/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: bitmapFont #]
***/

#pragma once

#include "base/resource/include/resource.h"
#include "base/resource/include/resourceMetadata.h"

BEGIN_BOOMER_NAMESPACE(base::font)

/// char set
struct BASE_FONT_API BitmapFontCharset
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(BitmapFontCharset);

public:
    bool enabled = false;
    uint32_t firstCharCode = 0;
    uint32_t lastCharCode = 0;

    void collectCharCodes(base::HashSet<uint32_t>& outCharCodes) const;
};

/// manifest for cooking textures
class BASE_FONT_API BitmapFontImportConfig : public base::res::ResourceConfiguration
{
    RTTI_DECLARE_VIRTUAL_CLASS(BitmapFontImportConfig, base::res::ResourceConfiguration);

public:
    BitmapFontImportConfig();

    //--

    uint32_t m_size = 12;
    bool m_bold = false;
    bool m_italic = false;

    //--

    BitmapFontCharset m_setASCII;

    base::Array<BitmapFontCharset> m_additionalCharSets;

    //--

    void collectCharCodes(base::Array<uint32_t>& outCharCodes) const;
};

END_BOOMER_NAMESPACE(base::font)
