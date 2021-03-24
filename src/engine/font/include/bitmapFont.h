/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: bitmapFont #]
***/

#pragma once

#include "core/resource/include/resource.h"

BEGIN_BOOMER_NAMESPACE_EX(font)

//--

/// glyph in the bitmap font
struct ENGINE_FONT_API BitmapFontGlyph
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(BitmapFontGlyph);

public:
    uint32_t charCode;
    Point offset;
    Point size;
    Point advance;
    Vector4 uv;
};

//--

/// printable glyph
struct ENGINE_FONT_API BitmapFontPrintableGlyph
{
    Point placement;
    Color color;
    const BitmapFontGlyph* glyph = nullptr;
};

//--

/// a simple bitmap font
class ENGINE_FONT_API BitmapFont : public IResource
{
    RTTI_DECLARE_VIRTUAL_CLASS(BitmapFont, IResource);

public:
    BitmapFont();
    BitmapFont(const ImagePtr& imageData, int ascender, int descender, int lineHeight, Array<BitmapFontGlyph>&& glyphs);
    virtual ~BitmapFont();

    //--

    /// get the compiled texture atlas image for this font
    INLINE const ImagePtr& imageAtlas() const { return m_image; }

    // get size of the upper part of the font (above baseline)
    INLINE int ascender() const { return m_ascender; }

    // get relative size of lower part of the font (below baseline)
    INLINE int descender() const { return m_descender; }

    // get relative // relative line height scalar
    INLINE int lineHeight() const { return m_lineHeight; }

    //--

    /// measure width of text
    Point measure(StringView txt) const;

    //--

    // generate glyphs
    Point render(StringView txt, Color initialColor, Array<BitmapFontPrintableGlyph>& outGlyphs) const;

private:
    // image with all font glyphs
    ImagePtr m_image;

    // glyphs
    Array<BitmapFontGlyph> m_glyphs;
    HashMap<uint32_t, const BitmapFontGlyph*> m_glyphMap;

    // sizing
    int m_ascender;
    int m_descender;
    int m_lineHeight;

    virtual void onPostLoad() override;

    void rebuildGlyphMap();
};

//--

END_BOOMER_NAMESPACE_EX(font)
