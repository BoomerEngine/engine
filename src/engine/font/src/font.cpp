/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: font #]
***/

#include "build.h"
#include "font.h"
#include "fontLibrary.h"
#include "fontInputText.h"
#include "fontGlyph.h"
#include "fontGlyphBuffer.h"
#include "fontGlyphCache.h"

#include "core/system/include/scopeLock.h"
#include "core/resource/include/tags.h"
#include "core/containers/include/utf8StringFunctions.h"

BEGIN_BOOMER_NAMESPACE()

//---

FontStyleParams::FontStyleParams(uint32_t size/*= 10*/, bool bold /*= false*/, bool italic /*= false*/, float blur /*= 0.0f*/)
    : size(size)
    , bold(bold)
    , italic(italic)
    , blur(blur)
{}

uint32_t FontStyleParams::calcHash() const
{
    CRC32 hash;
    hash << size;
    hash << (uint32_t)blur;
    return hash.crc();
}

//---

RTTI_BEGIN_TYPE_CLASS(Font);
    RTTI_PROPERTY(m_packedData);
    RTTI_METADATA(ResourceDescriptionMetadata).description("Vector Font");
RTTI_END_TYPE();

static std::atomic<uint32_t> GFontID(1);

Font::Font()
    : m_face(nullptr)
    , m_id(0)
    , m_ascender(8.0f)
    , m_descender(8.0f)
    , m_lineHeight(16.0f)
{
    m_glyphCache = CreateUniquePtr<FontGlyphCache>();
}

Font::Font(const Buffer& data)
    : m_face(nullptr)
    , m_id(0)
    , m_ascender(8.0f)
    , m_descender(8.0f)
    , m_lineHeight(16.0f)
    , m_packedData(data)
{
    m_glyphCache = CreateUniquePtr<FontGlyphCache>();
    registerFont();
}

Font::~Font()
{
    unregisterFont();
}

void Font::registerFont()
{
    if (!m_face)
    {
        // Allocate na ID
        m_id = range_cast<FontID>(++GFontID);

        // Load file
        m_face = FontLibrary::GetInstance().loadFace(m_packedData.data(), m_packedData.size());

        // Get font vertical metrics
        auto ascent = m_face ? m_face->ascender : 10.0f;
        auto descent = m_face ? m_face->descender : 10.0f;
        auto lineGap = m_face ? m_face->height - (ascent - descent) : 12.0f;

        // Store the normalized font height, the real font height can be computed by multiplying with font size
        auto fh = ascent - descent;
        m_ascender = (float) ascent / (float) fh;
        m_descender = (float) descent / (float) fh;
        m_lineHeight = (float) (fh + lineGap) / (float) fh;

        // Reset cache
        m_glyphCache->clear();
    }
}

void Font::unregisterFont()
{
    if (m_face)
    {
        FontLibrary::GetInstance().freeFace(m_face);
        m_face = nullptr;
    }
}

void Font::onPostLoad()
{
    TBaseClass::onPostLoad();
    registerFont();
}

uint32_t Font::lineSeparation(uint32_t fontSize) const
{
    return (uint32_t)(m_lineHeight * fontSize);
}

void Font::measureText(const FontStyleParams& styleParams, StringView utf8str, FontMetrics& outMetrics) const
{
    outMetrics.lineHeight = lineSeparation(styleParams.size);
    outMetrics.textWidth = 0;

    // no data to process
    if (utf8str.empty() || !m_face)
        return;

    // compute the hash of the styles
    auto styleHash = styleParams.calcHash();

    // process string
    float xPos = 0.0f;
    float maxXPos = 0.0f;
    {
        const auto* str = utf8str.data();
        const auto* strEnd = str + utf8str.length();
        while (str < strEnd)
        {
            const auto ch = utf8::NextChar(str, strEnd);
            if (!ch)
                break;

            // get the glyph information
            auto glyph = m_glyphCache->fetchGlyph(styleParams, styleHash, m_face, m_id, ch);
            if (glyph)
            {
                xPos += (int)(glyph->advance().x + 0.5f);
                maxXPos = std::max(maxXPos, xPos);
            }
        }
    }

    outMetrics.textWidth = (uint32_t)maxXPos;
}

const FontGlyph* Font::renderGlyph(const FontStyleParams& styleParams, uint32_t glyphcode) const
{
    auto styleHash = styleParams.calcHash();
    return m_glyphCache->fetchGlyph(styleParams, styleHash, m_face, m_id, glyphcode);
}

void Font::renderText(const FontStyleParams& styleParams, StringView utf8str, FontGlyphBuffer& outBuffer, Color color) const
{
    // no data to process
    if (utf8str.empty() || !m_face)
        return;

    // compute vertical placement offset
    float y = 0.0f; // baseline offset
    /*if (textParams.verticalAlignment == FontAlignmentVertical::Top)
        y = m_ascender * styleParams.size;
    else if (textParams.verticalAlignment == FontAlignmentVertical::Middle)
        y = ((m_ascender + m_descender) / 2.0f) * styleParams.size;
    else if (textParams.verticalAlignment == FontAlignmentVertical::Bottom)
        y = m_descender * styleParams.size;*/

    // compute horizontal placement offset
    float x = 0.0f;
    /*if (textParams.horizontalAlignment == FontAlignmentHorizontal::Center)
    { 
        // measure text to draw
        FontMetrics metrics;
        measureText(styleParams, textParams, str, metrics);
        x = -(float)metrics.textWidth * 0.5f;
    }
    else if (textParams.horizontalAlignment == FontAlignmentHorizontal::Right)
    {
        // measure text to draw
        FontMetrics metrics;
        measureText(styleParams, textParams, str, metrics);
        x = -(float)metrics.textWidth;
    }*/

    // compute the hash of the styles
    auto styleHash = styleParams.calcHash();

    // relative font ascender/descender
    int ascender = (int)(m_ascender * styleParams.size);
    int descender = (int)(m_descender * styleParams.size);

    // process string
    int textPosition = 0;
    {
        const auto* str = utf8str.data();
        const auto* strEnd = str + utf8str.length();
        while (str < strEnd)
        {
            const auto ch = utf8::NextChar(str, strEnd);
            if (!ch)
                break;

            if (ch == 13 || ch == 10)
            {
                // new line ?
                outBuffer.newLine(descender);
                continue;
            }
            else if (ch == 9)
            {
                // tab ?
                outBuffer.m_cursor.x = Align(outBuffer.m_cursor.x, 200); // TODO: proper tabbing
            }
            else if (ch < 32)
            {
                // generic white space
                continue;
            }

            // get the glyph information
            if (auto glyph = m_glyphCache->fetchGlyph(styleParams, styleHash, m_face, m_id, ch))
                outBuffer.push(ch, glyph, textPosition, color);

            // count position in the text
            textPosition++;
        }
    }
}

//---

END_BOOMER_NAMESPACE()
