/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: bitmapFont #]
***/

#pragma once

#include "base/resource/include/resource.h"

BEGIN_BOOMER_NAMESPACE(base::font)

//--

/// glyph in the bitmap font
struct BASE_FONT_API BitmapFontGlyph
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(BitmapFontGlyph);

public:
    uint32_t charCode;
    base::Point offset;
    base::Point size;
    base::Point advance;
    base::Vector4 uv;
};

//--

/// printable glyph
struct BASE_FONT_API BitmapFontPrintableGlyph
{
    base::Point placement;
    base::Color color;
    const BitmapFontGlyph* glyph = nullptr;
};

//--

/// a simple bitmap font
class BASE_FONT_API BitmapFont : public base::res::IResource
{
    RTTI_DECLARE_VIRTUAL_CLASS(BitmapFont, base::res::IResource);

public:
    BitmapFont();
    BitmapFont(const image::ImagePtr& imageData, int ascender, int descender, int lineHeight, base::Array<BitmapFontGlyph>&& glyphs);
    virtual ~BitmapFont();

    //--

    /// get the compiled texture atlas image for this font
    INLINE const image::ImagePtr& imageAtlas() const { return m_image; }

    // get size of the upper part of the font (above baseline)
    INLINE int ascender() const { return m_ascender; }

    // get relative size of lower part of the font (below baseline)
    INLINE int descender() const { return m_descender; }

    // get relative // relative line height scalar
    INLINE int lineHeight() const { return m_lineHeight; }

    //--

    /// measure width of text
    base::Point measure(base::StringView txt) const;

    //--

    // generate glyphs
    base::Point render(base::StringView txt, base::Color initialColor, base::Array<BitmapFontPrintableGlyph>& outGlyphs) const;

private:
    // image with all font glyphs
    image::ImagePtr m_image;

    // glyphs
    base::Array<BitmapFontGlyph> m_glyphs;
    base::HashMap<uint32_t, const BitmapFontGlyph*> m_glyphMap;

    // sizing
    int m_ascender;
    int m_descender;
    int m_lineHeight;

    virtual void onPostLoad() override;

    void rebuildGlyphMap();
};

//--

END_BOOMER_NAMESPACE(base::font)
