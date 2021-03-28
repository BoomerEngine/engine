/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: font #]
***/

#include "build.h"
#include "fontGlyph.h"
#include "fontGlyphCache.h"
#include "font.h"

#include "core/image/include/image.h"
#include "core/image/include/imageUtils.h"
#include "core/image/include/imageView.h"

BEGIN_BOOMER_NAMESPACE()

FontGlyphCache::FontGlyphCache()
    : m_generationIndex(0)
{
}

FontGlyphCache::~FontGlyphCache()
{
    clear();
}

void FontGlyphCache::clear()
{
    auto lock = CreateLock(m_lock);

    for (auto& value : m_glyphs.values())
        delete value.glyph;

    m_glyphs.clear();
}

void FontGlyphCache::purge(uint32_t relativeGeneration)
{
    PC_SCOPE_LVL1(PurgeGlyphCache);

    auto lock = CreateLock(m_lock);

    uint32_t cutoff = relativeGeneration >= m_generationIndex ? m_generationIndex - relativeGeneration : 0;

    auto oldGlyphs = std::move(m_glyphs);
    for (auto pair : oldGlyphs.pairs())
        if (pair.value.lastGeneration >= cutoff)
            m_glyphs.set(pair.key, pair.value);
}

FontGlyphCache::GlyphKey FontGlyphCache::ComputeKey(FontID fontId, FontGlyphID glyphId, FontStyleHash styleHash)
{
    GlyphKey key = styleHash;
    key |= (GlyphKey)fontId << 32;
    key |= (GlyphKey)glyphId << 40;
    return key;
}

namespace helper
{
    // Based on Exponential blur, Jani Huhtanen, 2006

#define APREC 16
#define ZPREC 7

    static void BlurCols(uint8_t* dst, uint32_t w, uint32_t h, uint32_t dstStride, int alpha)
    {
        for (uint32_t y = 0; y < h; y++)
        {
            int z = 0; // force zero border
            for (uint32_t x = 1; x < w; x++)
            {
                z += (alpha * (((int)(dst[x]) << ZPREC) - z)) >> APREC;
                dst[x] = (uint8_t)(z >> ZPREC);
            }
            dst[w - 1] = 0; // force zero border
            z = 0;

            for (int x = w-2; x >= 0; x--)
            {
                z += (alpha * (((int)(dst[x]) << ZPREC) - z)) >> APREC;
                dst[x] = (uint8_t)(z >> ZPREC);
            }
            dst[0] = 0; // force zero border
            dst += dstStride;
        }
    }

    static void BlurRows(uint8_t* dst, uint32_t w, uint32_t h, uint32_t dstStride, int alpha)
    {
        for (uint32_t x = 0; x < w; x++)
        {
            int z = 0; // force zero border
            for (uint32_t y = dstStride; y < h*dstStride; y += dstStride)
            {
                z += (alpha * (((int)(dst[y]) << ZPREC) - z)) >> APREC;
                dst[y] = (uint8_t)(z >> ZPREC);
            }
            dst[(h - 1)*dstStride] = 0; // force zero border
            z = 0;
            for (int y = (h - 2)*dstStride; y >= 0; y -= dstStride) {
                z += (alpha * (((int)(dst[y]) << ZPREC) - z)) >> APREC;
                dst[y] = (uint8_t)(z >> ZPREC);
            }
            dst[0] = 0; // force zero border
            dst++;
        }
    }

    static void BlurImage(uint8_t* dst, uint32_t w, uint32_t h, uint32_t dstStride, float blur)
    {
        if (blur <= 1.0f)
            return;

        // Calculate the alpha such that 90% of the kernel is within the radius. (Kernel extends to infinity)
        auto sigma = blur * 0.57735f; // 1 / sqrt(3)
        auto alpha = (int)((1 << APREC) * (1.0f - expf(-2.3f / (sigma + 1.0f))));
        BlurRows(dst, w, h, dstStride, alpha);
        BlurCols(dst, w, h, dstStride, alpha);
        //BlurRows(dst, w, h, dstStride, alpha);
        //BlurCols(dst, w, h, dstStride, alpha);
    }

    static void BlurImage(Image& img, float blur)
    {
        ASSERT(img.format() == ImagePixelFormat::Uint8_Norm);
        ASSERT(img.channels() == 1);
        BlurImage((uint8_t*)img.data(), img.width(), img.height(), img.rowPitch(), blur);
    }

} // helper

FontGlyph* FontGlyphCache::buildGlyph(const FontStyleParams& styleParams, FT_Face faceData, FontID fontId, uint32_t ch)
{
    // set font size
    auto adjustedSize = (FT_UInt)(styleParams.size * (float)faceData->units_per_EM / (float)(faceData->ascender - faceData->descender));
    FT_Error err = FT_Set_Pixel_Sizes(faceData, 0, adjustedSize);
    if (err != 0)
    {
        TRACE_WARNING("Unable to select size {} for font rendering", styleParams.size);
        return nullptr;
    }

    // load the char into the font
    err = FT_Load_Char(faceData, ch, FT_LOAD_RENDER);
    if (err != 0)
    {
        TRACE_WARNING("Unable to load char '%c' for rendering", ch);
        return nullptr;
    }

    // render the glyph
    FT_Render_Glyph(faceData->glyph, FT_RENDER_MODE_NORMAL);// FT_RENDER_MODE_LCD);

    // height
    const float invScale = 1.0f / (faceData->ascender - faceData->descender);
    const int ascender = (int)(faceData->ascender * invScale * styleParams.size);
    const auto descender = (int)(faceData->descender * invScale * styleParams.size);

    // note: whitespace characters have no bitmap
    ImagePtr ptr;
    if (faceData->glyph->bitmap.width > 0 && ch > ' ')
    {
        // compute image padding
        // this depends on the used effects as well
        int imagePadding = 2;
        if (styleParams.blur > 1.0f)
            imagePadding += (uint32_t)(styleParams.blur + 1.0f);

        // compute the glyph image size
        auto width = range_cast<uint16_t>(faceData->glyph->bitmap.width + imagePadding * 2);
        auto height = range_cast<uint16_t>(faceData->glyph->bitmap.rows + imagePadding * 2);

        // create the image for the glyph, note: it has border
        ptr = RefNew<Image>(ImagePixelFormat::Uint8_Norm, 1, width, height);

        // fill to black
        memset(ptr->data(), 0, ptr->view().dataSize());

        // copy data from free type
        {
            ImageView sourceView(NATIVE_LAYOUT, ImagePixelFormat::Uint8_Norm, 1, faceData->glyph->bitmap.buffer, faceData->glyph->bitmap.width, faceData->glyph->bitmap.rows);
            auto destView = ptr->view().subView(imagePadding, imagePadding, faceData->glyph->bitmap.width, faceData->glyph->bitmap.rows);
            Copy(sourceView, destView);
        }

        // apply effects on the glyph
        if (styleParams.blur > 1.0f)
        {
            //Image::Save(AbsolutePath::Build(UTF16StringVector(L"Q:\\glyph_src.png")), *ptr);
            helper::BlurImage(*ptr, styleParams.blur);
            //Image::Save(AbsolutePath::Build(UTF16StringVector(L"Q:\\glyph_dest.png")), *ptr);
        }

        // compute glyph metrics
        auto bitmapOffset = Point(faceData->glyph->bitmap_left, -faceData->glyph->bitmap_top) - Point(imagePadding, imagePadding);
        auto bitmapSize = Point(ptr->width(), ptr->height());
        auto advance = Vector2(faceData->glyph->advance.x / 64.0f, faceData->glyph->advance.y / 64.0f);

        // compute glyph logical rect
        auto rect = Rect(faceData->glyph->bitmap_left, -faceData->glyph->bitmap_top, faceData->glyph->bitmap_left + faceData->glyph->bitmap.width, -faceData->glyph->bitmap_top + faceData->glyph->bitmap.rows);

        // create glyph
        FontGlyphKey glyphId(fontId, ch, styleParams.calcHash());
        return new FontGlyph(glyphId, ptr, bitmapOffset, bitmapSize, advance, rect, ascender, descender);
    }
    else
    {
        // create glyph with no bitmap
        auto advance = Vector2(faceData->glyph->advance.x / 64.0f, faceData->glyph->advance.y / 64.0f);
        FontGlyphKey glyphId(fontId, ch, styleParams.calcHash());
        return new FontGlyph(glyphId, ptr, Point(0,0), Point(0, 0), advance, Rect(0,0,0,0), ascender, descender);
    }
}

const FontGlyph* FontGlyphCache::fetchGlyph(const FontStyleParams& styleParams, uint32_t styleHash, FT_Face faceData, FontID fontId, uint32_t glyph)
{
    // compute the unique glyph key to look up
    auto key = ComputeKey(fontId, (FontGlyphID)glyph, styleHash);

    auto lock = CreateLock(m_lock);

    // look it up
    GlyphEntry* entry = m_glyphs.find(key);
    if (entry)
    {
        entry->lastGeneration = m_generationIndex;
        return entry->glyph;
    }

    // create glyph
    GlyphEntry newEntry;
    newEntry.glyph = buildGlyph(styleParams, faceData, fontId, glyph);
    newEntry.lastGeneration = m_generationIndex;
    m_glyphs.set(key, newEntry); // register in map
    return newEntry.glyph;
}

END_BOOMER_NAMESPACE()
