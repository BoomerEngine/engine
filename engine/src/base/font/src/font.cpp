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

#include "base/system/include/scopeLock.h"

namespace base
{
    namespace font
    {

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
            RTTI_METADATA(res::ResourceExtensionMetadata).extension("v4font");
            RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("Vector Font");
        RTTI_END_TYPE();

        static std::atomic<uint32_t> GFontID(1);

        Font::Font()
            : m_face(nullptr)
            , m_id(0)
            , m_ascender(8.0f)
            , m_descender(8.0f)
            , m_lineHeight(16.0f)
        {
            m_glyphCache = CreateUniquePtr<GlyphCache>();
        }

        Font::Font(const Buffer& data)
            : m_face(nullptr)
            , m_id(0)
            , m_ascender(8.0f)
            , m_descender(8.0f)
            , m_lineHeight(16.0f)
            , m_packedData(data)
        {
            m_glyphCache = CreateUniquePtr<GlyphCache>();
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

        void Font::measureText(const FontStyleParams& styleParams, const FontAssemblyParams& textParams, const FontInputText& str, FontMetrics& outMetrics) const
        {
            outMetrics.lineHeight = lineSeparation(styleParams.size);
            outMetrics.textWidth = 0;

            // no data to process
            if (str.empty() || !m_face)
                return;

            // compute the hash of the styles
            auto styleHash = styleParams.calcHash();

            // process string
            float xPos = 0.0f;
            float maxXPos = 0.0f;
            for (auto ch : str.chars())
            {
                // get the glyph information
                auto glyph  = m_glyphCache->fetchGlyph(styleParams, styleHash, m_face, m_id, ch);
                if (glyph)
                {
                    xPos += (int)(glyph->advance().x + 0.5f);
                    maxXPos = std::max(maxXPos, xPos);
                }
            }

            outMetrics.textWidth = (uint32_t)maxXPos;
        }

        const Glyph* Font::renderGlyph(const FontStyleParams& styleParams, uint32_t glyphcode) const
        {
            auto styleHash = styleParams.calcHash();
            return m_glyphCache->fetchGlyph(styleParams, styleHash, m_face, m_id, glyphcode);
        }

        void Font::renderText(const FontStyleParams& styleParams, const FontAssemblyParams& textParams, const FontInputText& str, GlyphBuffer& outBuffer) const
        {
            // no data to process
            if (str.empty() || !m_face)
                return;

            // compute vertical placement offset
            float y = 0.0f; // baseline offset
            if (textParams.verticalAlignment == FontAlignmentVertical::Top)
                y = m_ascender * styleParams.size;
            else if (textParams.verticalAlignment == FontAlignmentVertical::Middle)
                y = ((m_ascender + m_descender) / 2.0f) * styleParams.size;
            else if (textParams.verticalAlignment == FontAlignmentVertical::Bottom)
                y = m_descender * styleParams.size;

            // compute horizontal placement offset
            float x = 0.0f;
            if (textParams.horizontalAlignment == FontAlignmentHorizontal::Center)
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
            }

            // compute the hash of the styles
            auto styleHash = styleParams.calcHash();

            // process string
            int textPosition = 0;
            for (auto ch : str.chars())
            {
                // get the glyph information
                auto glyph  = m_glyphCache->fetchGlyph(styleParams, styleHash, m_face, m_id, ch);
                if (glyph)
                {
                    // place glyph
                    outBuffer.addGlyph(ch, glyph, x, y, textPosition);

                    // advance to new position
                    x += (int)(glyph->advance().x + 0.5f);
                }

                // count position in the text
                textPosition++;
            }
        }

        //---

    } // font
} // base
