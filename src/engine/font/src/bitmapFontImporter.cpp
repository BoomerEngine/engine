/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: bitmapFont #]
***/

#include "build.h"

#include "font.h"
#include "bitmapFont.h"
#include "bitmapFontImportConfig.h"

#include "core/image/include/image.h"
#include "core/image/include/imageView.h"
#include "core/image/include/imageUtils.h"
#include "core/resource/include/resource.h"
#include "core/resource/include/metadata.h"
#include "core/containers/include/inplaceArray.h"
#include "engine/font/include/font.h"
#include "engine/font/include/fontGlyph.h"
#include "core/containers/include/rectAllocator.h"

BEGIN_BOOMER_NAMESPACE_EX(font)

/*
//---

class BitmapFontImporter : public IResourceImporter
{
    RTTI_DECLARE_VIRTUAL_CLASS(BitmapFontCooker, IResourceImporter);

public:
    virtual ResourcePtr cook(IResourceCookerInterface& cooker) const override;
};

RTTI_BEGIN_TYPE_CLASS(BitmapFontCooker);
    RTTI_METADATA(ResourceImportedClassMetadata).addClass<BitmapFont>();
    RTTI_METADATA(ResourceSourceFormatMetadata).addSourceExtension("ttf").addSourceExtension("otf");
    RTTI_METADATA(ResourceCookerBakingOnlyMetadata);
RTTI_END_TYPE();

struct BitmapFontCookingGlyph
{
    ImagePtr glyphImage;
    BitmapFontGlyph info;
};

struct BitmapFontCookingAtlasPage
{
    RectAllocator allocator;
    uint32_t maxWidth = 0;
    uint32_t maxHeight = 0;

    BitmapFontCookingAtlasPage(uint32_t atlasSize)
    {
        allocator.reset(atlasSize, atlasSize);
    }
};

ResourcePtr BitmapFontCooker::cook(IResourceCookerInterface& cooker) const
{
    // get size of the font
    auto fontSize = 16;// cooker.queryResourcePath().paramInt("size", 16);

    // limit glyph size 
    if (fontSize > 512)
    {
        TRACE_ERROR("Font size {} is to large, please use vector fonts or a single character texture", fontSize);
        return nullptr;
    }

    // load the raw font
    ResourcePathBuilder fontPath(cooker.queryResourcePath());
    //fontPath.params.remove("size");

    // load the font
    auto baseFont = cooker.loadDependencyResource<font::Font>(fontPath.toPath());
    if (!baseFont)
    {
        TRACE_ERROR("Unable to load font from '{}' so no bitmap font can be cooked");
        return nullptr;
    }

    // gather the characters to process
    Array<uint32_t> charCodes;
    if (auto bitmapFontManifset = cooker.loadManifestFile<BitmapFontManifest>())
        bitmapFontManifset->collectCharCodes(charCodes);

    // nothing to write
    if (charCodes.empty())
    {
        TRACE_ERROR("No characters to export into bitmap font");
        return nullptr;
    }

    // setup font styling
    font::FontStyleParams styleParams;
    styleParams.size = fontSize;

    // generate glyphs
    uint32_t numMissingGlyphs = 0;
    uint64_t totalGlyphArea = 0;
    Array<BitmapFontCookingGlyph> glyphs;
    for (auto code : charCodes)
    {
        // render the glyph into the texture
        auto rawGlyph  = baseFont->renderGlyph(styleParams, code);
        if (!rawGlyph)
        {
            numMissingGlyphs += 1;
            continue;
        }

        auto& entry = glyphs.emplaceBack();
        entry.info.charCode = code;
        entry.glyphImage = rawGlyph->bitmap();
        entry.info.advance = rawGlyph->advance();
        entry.info.offset = rawGlyph->offset();
        entry.info.size = rawGlyph->size();
        //entry.info.m_logicalRect = rawGlyph->logicalRect();
        //entry.info.m_pagePlacement = Point(0, 0);
        //entry.info.pageIndex = 0;

        if (entry.glyphImage)
            totalGlyphArea += (uint32_t)entry.glyphImage->width() * entry.glyphImage->height();
    }

    TRACE_INFO("Generated {} glyphs covering {} pixel area", glyphs.size(), totalGlyphArea);

    uint32_t atlasSize = 1024;
    if (totalGlyphArea < (128 * 128 * 3 / 4))
        atlasSize = 128;
    else if (totalGlyphArea < (256 * 256 * 3 / 4))
        atlasSize = 256;
    else if (totalGlyphArea < (512 * 512 * 3 / 4))
        atlasSize = 512;

    BitmapFontCookingAtlasPage page(atlasSize);
    for (auto& glyph : glyphs)
    {
        // some glyphs may have no image
        if (!glyph.glyphImage)
            continue;

        // place in existing pages
        while (true)
        {
            uint32_t posX = 0, posY = 0;
            if (page.allocator.allocate(glyph.glyphImage->width(), glyph.glyphImage->height(), posX, posY))
            {
                glyph.info.uv.x = posX;
                glyph.info.uv.y = posY;
                glyph.info.uv.z = posX + glyph.glyphImage->width();
                glyph.info.uv.w = posY + glyph.glyphImage->height();

                page.maxWidth = std::max<uint32_t>(page.maxWidth, posX + glyph.glyphImage->width());
                page.maxHeight = std::max<uint32_t>(page.maxHeight, posY + glyph.glyphImage->height());
                break;
            }

            // resize the atlas allocation area, NOTE: this does not determine the actual texture size yet
            if (atlasSize < 8192)
            {
                atlasSize *= 2;
                page.allocator.resize(atlasSize, atlasSize);
            }
            else
            {
                TRACE_ERROR("Bitmap font has to many texture data and would use up to much memory, consider lowering font size or limiting charset");
                return nullptr;
            }
        }
    }

    // max page size
    page.maxWidth = Align<uint32_t>(page.maxWidth, 4);
    page.maxHeight = Align<uint32_t>(page.maxHeight, 4);
    TRACE_INFO("Max size of atlas page: [{},{}]", page.maxWidth, page.maxHeight);

    // create target image to copy all the glyphs into, the image is alpha only
    auto image = RefNew<Image>(ImagePixelFormat::Uint8_Norm, 1, page.maxWidth, page.maxHeight);

    // copy glyphs
    Array<BitmapFontGlyph> finalGlyphs;

    auto invWidth = 1.0f / (float)page.maxWidth;
    auto intHeight = 1.0f / (float)page.maxHeight;
    for (auto& glyph : glyphs)
    {
        if (glyph.glyphImage)
        {
            // copy image
            auto targetView = image->view().subView((int)glyph.info.uv.x, (int)glyph.info.uv.y, glyph.glyphImage->width(), glyph.glyphImage->height());
            Copy(glyph.glyphImage->view(), targetView);

            // update UV
            glyph.info.uv.x *= invWidth;
            glyph.info.uv.y *= intHeight;
            glyph.info.uv.z *= invWidth;
            glyph.info.uv.w *= intHeight;

            //glyph.info.uv.x += invWidth * 0.5f;
            //glyph.info.uv.y += intHeight * 0.5f;
            //glyph.info.uv.z += invWidth * 0.5f;
            //glyph.info.uv.w += intHeight * 0.5f;
        }
    }

    // save the images
    //for (uint32_t i = 0; i < images.size(); ++i)
    //{
    //  auto name = StringBuf(TempString("Z:\\font_atlas{}.png", i));
    //                auto path = AbsolutePath::Build(name.uni_str());
    //Image::Save(path, *images[i]);
    //}

    // font data
    auto ascender = (int)(baseFont->relativeAscender() * fontSize);
    auto descenter = (int)(baseFont->relativeDescender() * fontSize);
    auto lineHeight = (int)(baseFont->relativeLineHeight() * fontSize);

    // we should cook a texture
    return RefNew<BitmapFont>(image, ascender, descenter, lineHeight, std::move(finalGlyphs));
}
*/
//---

END_BOOMER_NAMESPACE_EX(font)
