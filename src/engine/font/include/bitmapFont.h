/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: bitmapFont #]
***/

#pragma once

#include "core/resource/include/resource.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// glyph in the bitmap font
struct ENGINE_FONT_API BitmapFontGlyph
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(BitmapFontGlyph);

public:
    uint32_t charCode = 0;
    int page = 0;
    Point offset;
    Point size;
    Point advance;
    Vector4 uv; // xy=min, zw=max
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
    struct Setup
    {
        Array<ImagePtr> imagePages; // all the same size
        int ascender = 0;
        int descender = 0;
        int lineHeight = 0;
        Array<BitmapFontGlyph> glyphs;
    };

    BitmapFont();
    BitmapFont(Setup&& setup);
    virtual ~BitmapFont();

    //--

    /// compiled image pages, we can have more then one but they will be the same size
    INLINE const Array<ImagePtr>& images() const { return m_images; }

    /// glyphs
    INLINE const Array<BitmapFontGlyph>& glyph() const { return m_glyphs; }

    // get size of the upper part of the font (above baseline)
    INLINE int ascender() const { return m_ascender; }

    // get relative size of lower part of the font (below baseline)
    INLINE int descender() const { return m_descender; }

    // get relative // relative line height scalar
    INLINE int lineHeight() const { return m_lineHeight; }

    //--

    /// measure width of text, the height is always the line height
    Point measure(StringView txt) const;

    // generate glyphs
    Point render(StringView txt, Color initialColor, Array<BitmapFontPrintableGlyph>& outGlyphs) const;

    //--

private:
    // images with all font glyphs
    Array<ImagePtr> m_images;

    // glyphs
    Array<BitmapFontGlyph> m_glyphs;
    HashMap<uint32_t, const BitmapFontGlyph*> m_glyphMap;

    // sizing
    int m_ascender = 0;
    int m_descender = 0;
    int m_lineHeight = 0;

    virtual void onPostLoad() override;

    void rebuildGlyphMap();
};

//--

END_BOOMER_NAMESPACE()
