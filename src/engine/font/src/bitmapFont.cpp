/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: bitmapFont #]
***/

#include "build.h"
#include "bitmapFont.h"

#include "core/containers/include/utf8StringFunctions.h"
#include "core/image/include/image.h"
#include "core/resource/include/tags.h"

BEGIN_BOOMER_NAMESPACE_EX(font)

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
    RTTI_METADATA(ResourceDescriptionMetadata).description("Bitmap Font");
    RTTI_PROPERTY(m_image);
    RTTI_PROPERTY(m_glyphs);
    RTTI_PROPERTY(m_ascender);
    RTTI_PROPERTY(m_descender);
    RTTI_PROPERTY(m_lineHeight);
RTTI_END_TYPE();

BitmapFont::BitmapFont()
{}

BitmapFont::BitmapFont(const ImagePtr& imageData, int ascender, int descender, int lineHeight, Array<BitmapFontGlyph>&& glyphs)
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

Point BitmapFont::measure(StringView txt) const
{
    auto pos  = txt.data();
    auto end  = pos + txt.length();
    auto state  = 0;

    auto maxX  = 0;
    auto x  = 0;
    auto y  = 0;

    while (pos < end)
    {
        auto code  = utf8::NextChar(pos, end);

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

    return Point(maxX, y + m_lineHeight);
}

Point BitmapFont::render(StringView txt, Color initialColor, Array<BitmapFontPrintableGlyph>& outGlyphs) const
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
        auto code  = utf8::NextChar(pos, end);

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
                    case '0': color = Color::BLACK; break;
                    case '1': color = Color::RED; break;
                    case '2': color = Color::GREEN; break;
                    case '3': color = Color::YELLOW; break;
                    case '4': color = Color::BLUE; break;
                    case '5': color = Color::PURPLE; break;
                    case '6': color = Color::CYAN; break;
                    case '7': color = Color::GRAY; break;
                    case '8': color = Color::LIGHTGREY; break;
                    case 'F': color = Color::WHITE; break;
                }

                state = 0;
                break;
            }
        }
    }

    return Point(maxX, y + m_lineHeight);
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

END_BOOMER_NAMESPACE_EX(font)
