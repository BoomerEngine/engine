/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: bitmapFont #]
***/

#include "build.h"
#include "bitmapFont.h"

#include "base/containers/include/utf8StringFunctions.h"
#include "base/image/include/image.h"
#include "base/resource/include/resourceTags.h"

namespace base
{
    namespace font
    {
        //--

        RTTI_BEGIN_TYPE_STRUCT(BitmapFontGlyph);
            RTTI_PROPERTY(charCode);
            RTTI_PROPERTY(offset);
            RTTI_PROPERTY(size);
            RTTI_PROPERTY(advance);
            RTTI_PROPERTY(uv);
        RTTI_END_TYPE();

        //--

        RTTI_BEGIN_TYPE_CLASS(BitmapFont);
            RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4bf");
            RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("Bitmap Font");
            RTTI_PROPERTY(m_image);
            RTTI_PROPERTY(m_glyphs);
            RTTI_PROPERTY(m_ascender);
            RTTI_PROPERTY(m_descender);
            RTTI_PROPERTY(m_lineHeight);
        RTTI_END_TYPE();

        BitmapFont::BitmapFont()
        {}

        BitmapFont::BitmapFont(const image::ImagePtr& imageData, int ascender, int descender, int lineHeight, base::Array<BitmapFontGlyph>&& glyphs)
            : m_image(imageData)
            , m_glyphs(std::move(glyphs))
            , m_ascender(ascender)
            , m_descender(descender)
            , m_lineHeight(lineHeight)
        {
            if (m_image)
                m_image->parent(this);

            // update runtime object
            invalidateRuntimeVersion();
            rebuildGlyphMap();
        }

        BitmapFont::~BitmapFont()
        {}

        base::Point BitmapFont::measure(base::StringView txt) const
        {
            auto pos  = txt.data();
            auto end  = pos + txt.length();
            auto state  = 0;

            auto maxX  = 0;
            auto x  = 0;
            auto y  = 0;

            while (pos < end)
            {
                auto code  = base::utf8::NextChar(pos, end);

                switch (state)
                {
                    case 0:
                    {
                        if (code == '^')
                        {
                            state = 1;
                            break;
                        }
                        else if (code == '\n')
                        {
                            x = 0;
                            y += m_lineHeight;
                        }
                        else if (code >= 32)
                        {
                            if (auto glyph  = m_glyphMap.find(code))
                            {
                                x += (*glyph)->advance.x;
                                maxX = std::max<int>(maxX, x);
                            }
                        }

                        break;
                    }

                    case 1:
                    {
                        state = 0;
                        break;
                    }
                }
            }

            return base::Point(maxX, y + m_lineHeight);
        }

        base::Point BitmapFont::render(base::StringView txt, base::Color initialColor, base::Array<BitmapFontPrintableGlyph>& outGlyphs) const
        {
            int x = 0;
            int y = 0;
            int rowMaxHeight = 0;

            auto pos  = txt.data();
            auto end  = pos + txt.length();
            auto state  = 0;
            auto color  = initialColor;
            auto maxX  = 0;

            while (pos < end)
            {
                auto code  = base::utf8::NextChar(pos, end);

                switch (state)
                {
                    case 0:
                    {
                        if (code == '^')
                        {
                            state = 1;
                            break;
                        }
                        else if (code == '\n')
                        {
                            x = 0;
                            y += m_lineHeight;
                        }
                        else if (code >= 32)
                        {
                            if (auto glyph  = m_glyphMap.find(code))
                            {
                                auto& outEntry = outGlyphs.emplaceBack();
                                outEntry.glyph = *glyph;
                                outEntry.color = color;
                                outEntry.placement.x = x;
                                outEntry.placement.y = y;
                                x += (*glyph)->advance.x;
                                maxX = std::max<int>(maxX, x);
                            }
                        }

                        break;
                    }

                    case 1:
                    {
                        switch (code)
                        {
                            case '0': color = base::Color::BLACK; break;
                            case '1': color = base::Color::RED; break;
                            case '2': color = base::Color::GREEN; break;
                            case '3': color = base::Color::YELLOW; break;
                            case '4': color = base::Color::BLUE; break;
                            case '5': color = base::Color::PURPLE; break;
                            case '6': color = base::Color::CYAN; break;
                            case '7': color = base::Color::GRAY; break;
                            case '8': color = base::Color::LIGHTGREY; break;
                            case 'F': color = base::Color::WHITE; break;
                        }

                        state = 0;
                        break;
                    }
                }
            }

            return base::Point(maxX, y + m_lineHeight);
        }

        void BitmapFont::onPostLoad()
        {
            TBaseClass::onPostLoad();

            rebuildGlyphMap();
        }

        void BitmapFont::rebuildGlyphMap()
        {
            m_glyphMap.clear();
            m_glyphMap.reserve(m_glyphs.size());

            for (auto& glyph : m_glyphs)
                m_glyphMap[glyph.charCode] = &glyph;
        }

        //--

        // RenderMesh,  v4mesh baked from Mesh 
        // ProcRenderMesh, v4procmesh - graph in manifest
        // Texture, v4tex - baked and compressed from image 
        // ProcTexture, v4proctex - graph in manifest
        // CSL - shader library, contains data only in final cooked form
        
        //--

        // v4texture - 

    } // content
} // rendering
