/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: font #]
***/

#pragma once

#include "core/resource/include/resource.h"
#include "core/system/include/mutex.h"

BEGIN_BOOMER_NAMESPACE()

//---

/// font styling parameters, control how the font is pixelated into the bitmaps
struct ENGINE_FONT_API FontStyleParams
{
    uint32_t size = 10; // size in pixels
    float blur = 0.0f; // size of the blur
    bool bold = false; // make characters bold (additional width), NOTE: procedural effect, quality may vary
    bool italic = false; // make characters italic (slanted), NOTE: procedural effect, quality may vary

    // calculate unique hash describing this rendering setup
    uint32_t calcHash() const;

    FontStyleParams(uint32_t size = 10, bool bold = false, bool italic = false, float blur = 0.0f);
};

//---

/// font metrics
struct FontMetrics
{
    uint32_t lineHeight = 0; // vertical separation between text lines at given font size
    uint32_t textWidth = 0; // total width of the text at given font size
};

//---

/// vector font, rendered via bitmaps
class ENGINE_FONT_API Font : public IResource
{
    RTTI_DECLARE_VIRTUAL_CLASS(Font, IResource);

public:
    Font();
    Font(const Buffer& data);
    virtual ~Font();

    // get relative size of the upper part of the font (above baseline)
    INLINE float relativeAscender() const { return m_ascender; }

    // get relative size of lower part of the font (below baseline)
    INLINE float relativeDescender() const { return m_descender; }

    // get relative // relative line height scalar
    INLINE float relativeLineHeight() const { return m_lineHeight; }

    // get font glyph cache
    INLINE const FontGlyphCache& glyphCache() const { return *m_glyphCache; }

    // get raw font data (for use with different font systems)
    INLINE const Buffer& data() const { return m_packedData; }

    //--

    /// calculate line separation for given font size
    uint32_t lineSeparation(uint32_t fontSize) const;

    /// compute font metrics for given text
    void measureText(const FontStyleParams& styleParams, StringView utfStr, FontMetrics& outMetrics) const;

    /// generate renderable characters for this font
    /// printable characters are written in the render buffer 
    /// the images for particular glyphs needed to render the text are maintained in the provided glyph cache
    /// returns size of the text (the same as measureText)
    void renderText(const FontStyleParams& styleParams, StringView utfStr, FontGlyphBuffer& outBuffer, Color color = Color::WHITE) const;

    /// fetch information about single character
    const FontGlyph* renderGlyph(const FontStyleParams& styleParams, uint32_t glyphcode) const;

private:
    Buffer m_packedData; // packed font data
    FT_Face m_face; // FreeType font data

    FontID m_id; // internal font ID

    float m_ascender; // upper part of the font (above baseline)
    float m_descender; // lower part of the font (below baseline)
    float m_lineHeight; // relative line height scalar

    UniquePtr<FontGlyphCache> m_glyphCache; // cache for glyphs created from this font

    void unregisterFont();
    void registerFont();
    virtual void onPostLoad();
};

END_BOOMER_NAMESPACE()
