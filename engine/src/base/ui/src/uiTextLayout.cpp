/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\text #]
***/

#include "build.h"
#include "uiTextLabel.h"
#include "uiTextLayout.h"
#include "uiGeometryBuilder.h"
#include "uiStyleValue.h"
#include "uiDataStash.h"

#include "base/font/include/fontGlyphBuffer.h"
#include "base/font/include/fontGlyph.h"
#include "base/image/include/image.h"
#include "base/containers/include/stringParser.h"
#include "base/containers/include/utf8StringFunctions.h"

namespace ui
{
    namespace prv
    {

        //--

        class LayutDataParser : public base::NoCopy
        {
        public:
            LayutDataParser(base::StringView txt)
            {
                curPtr = txt.data();
                endPtr = txt.data() + txt.length();
                curRegionStart = curPtr;
            }

            void process(LayoutData& regions)
            {
                sliceText();
                extract(regions);
            }

        private:
            INLINE static bool ValidChar(char ch)
            {
                return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
            }

            INLINE static const char* ParseMarkup(const char* cur, const char* endPtr, base::StringView& outMarkup, base::StringView& outArg)
            {
                if (*cur == '[')
                {
                    cur += 1;

                    auto start = cur;
                    if (cur < endPtr && *cur == '/') cur += 1;

                    while (cur < endPtr)
                    {
                        if (*cur == ':' || *cur == ']')
                            break;
                        if (!ValidChar(*cur))
                            return nullptr;
                        cur += 1;
                    }

                    if (cur < endPtr)
                    {
                        outMarkup = base::StringView(start, cur);

                        if (*cur == ':')
                        {
                            cur += 1;
                            start = cur;

                            while (cur < endPtr)
                            {
                                if (*cur == ']')
                                    break;
                                cur += 1;
                            }

                            if (cur < endPtr)
                            {
                                outArg = base::StringView(start, cur);
                                return cur + 1;
                            }
                        }
                        else
                        {
                            return cur + 1;
                        }
                    }
                }

                return nullptr;
            }

            int boolCount = 0;
            int italicCount = 0;
            LayoutHorizontalAlign lineAlignment = LayoutHorizontalAlign::Default;
            base::InplaceArray<base::Color, 10> colorStack;
            base::InplaceArray<base::Color, 10> tagStack;
            base::InplaceArray<LayoutVerticalAlign, 10> alignStack;
            base::InplaceArray<float, 10> sizeStack;

            base::Array<LayoutRegion> regions;
            base::Array<LayoutLine> lines;
            base::Array<uint32_t> codes;

            const char* curPtr = nullptr;
            const char* endPtr = nullptr;
            const char* curRegionStart = nullptr;
            int lineStartRegion = 0;
            int regionStartChar = 0;

            void finishLine(bool force=false)
            {
                if (regions.size() > lineStartRegion || force)
                {
                    auto& line = lines.emplaceBack();
                    line.firstRegion = lineStartRegion;
                    line.numRegions = regions.size() - lineStartRegion;
                    line.halaign = lineAlignment;
                    lineStartRegion = regions.size();
                    lineAlignment = LayoutHorizontalAlign::Default;
                }
            }

            void finishTextRegion(bool newLine = false)
            {
                if (codes.size() > regionStartChar)
                {
                    auto& region = regions.emplaceBack();
                    region.flagBold = boolCount > 0;
                    region.flagItalic = italicCount > 0;
                    region.flagColor = !colorStack.empty();
                    region.flagTag = !tagStack.empty();
                    region.color = colorStack.empty() ? base::Color::WHITE : colorStack.back();
                    region.tagColor = tagStack.empty() ? base::Color(0, 0, 0, 0) : tagStack.back();
                    region.size = sizeStack.empty() ? 1.0f : sizeStack.back();
                    region.firstCode = regionStartChar;
                    region.numCodes = codes.size() - regionStartChar;
                    regionStartChar = codes.size();
                }

                if (newLine)
                    finishLine(true);
            }

            void extract(LayoutData& outRegions)
            {
                outRegions.lines = std::move(lines);
                outRegions.regions = std::move(regions);
                outRegions.codes = std::move(codes);
            }

            bool parseColor(base::StringView txt, base::Color& outColor) const
            {
                return base::Color::Parse(txt, outColor);
            }

            void pushColor(base::StringView txt)
            {
                base::Color color;
                if (parseColor(txt, color))
                    colorStack.pushBack(color);
            }

            void pushTagColor(base::StringView txt)
            {
                base::Color color;
                if (parseColor(txt, color))
                    tagStack.pushBack(color);

            }

            const float SIZE_MULT = 1.25f;

            bool processMarkup(base::StringView markup, base::StringView arg)
            {
                if (markup == "b")
                {
                    finishTextRegion();
                    boolCount += 1;
                    return true;
                }
                else if (markup == "/b")
                {
                    finishTextRegion();
                    boolCount -= 1;
                    return true;
                }
                else if (markup == "/i")
                {
                    finishTextRegion();
                    italicCount -= 1;
                    return true;
                }
                else if (markup == "i")
                {
                    finishTextRegion();
                    italicCount += 1;
                    return true;
                }
                else if (markup == "color")
                {
                    finishTextRegion();
                    pushColor(arg);
                    return true;
                }
                else if (markup == "/color")
                {
                    finishTextRegion();
                    if (!colorStack.empty())
                        colorStack.popBack();
                    return true;
                }
                else if (markup == "br")
                {
                    finishTextRegion(true);
                    return true;
                }
                else if (markup == "center")
                {
                    lineAlignment = LayoutHorizontalAlign::Center;
                    return true;
                }
                else if (markup == "right")
                {
                    lineAlignment = LayoutHorizontalAlign::Right;
                    return true;
                }
                else if (markup == "left")
                {
                    lineAlignment = LayoutHorizontalAlign::Left;
                    return true;
                }
                else if (markup == "size")
                {
                    finishTextRegion();

                    float curSize = sizeStack.empty() ? 1.0f : sizeStack.back();

                    if (arg.beginsWith("+") || arg.beginsWith("-"))
                    {
                        for (auto ch : arg)
                        {
                            if (ch == '+') curSize *= SIZE_MULT;
                            else if (ch == '-') curSize /= SIZE_MULT;
                        }

                        sizeStack.pushBack(curSize);
                    }
                    else
                    {
                        int mult = 0;
                        arg.match(mult);

                        while (mult-- > 0)
                            curSize *= SIZE_MULT;
                        while (mult++ < 0)
                            curSize /= SIZE_MULT;
                        sizeStack.pushBack(curSize);
                    }
                    return true;
                }
                else if (markup == "/size")
                {
                    finishTextRegion();

                    if (!sizeStack.empty())
                        sizeStack.popBack();
                    return true;
                }
                else if (markup == "img")
                {
                    finishTextRegion();

                    auto& region = regions.emplaceBack();
                    region.imageName = base::StringID(arg);
                    region.valign = alignStack.empty() ? LayoutVerticalAlign::Middle : alignStack.back();
                    region.tagColor = tagStack.empty() ? base::Color(0, 0, 0, 0) : tagStack.back();
                    region.color = colorStack.empty() ? base::Color(255, 255, 255, 255) : colorStack.back();
                    region.flagTag = !tagStack.empty();
                    region.flagColor = !colorStack.empty();
                    return true;
                }
                else if (markup == "valign")
                {
                    if (arg == "top")
                        alignStack.pushBack(LayoutVerticalAlign::Top);
                    else if (arg == "baseline")
                        alignStack.pushBack(LayoutVerticalAlign::Baseline);
                    else if (arg == "bottom")
                        alignStack.pushBack(LayoutVerticalAlign::Bottom);
                    else if (arg == "middle")
                        alignStack.pushBack(LayoutVerticalAlign::Middle);
                    return true;
                }
                else if (markup == "/valign")
                {
                    if (!alignStack.empty())
                        alignStack.popBack();
                    return true;
                }
                else if (markup == "tag")
                {
                    finishTextRegion();
                    pushTagColor(arg);
                    return true;
                }
                else if (markup == "/tag")
                {
                    finishTextRegion();
                    if (!tagStack.empty())
                        tagStack.popBack();
                    return true;
                }

                return false;
            }

            void pushCodes(base::StringView txt)
            {
                for (const char ch : txt)
                    codes.pushBack(ch);
            }

            void sliceText()
            {
                curRegionStart = curPtr;
                while (curPtr < endPtr)
                {
                    // try to parse as markup
                    base::StringView markup, arg;
                    if (auto newCurPtr = ParseMarkup(curPtr, endPtr, markup, arg))
                    {
                        if (processMarkup(markup, arg))
                        {
                            curPtr = newCurPtr;
                            curRegionStart = curPtr;
                            continue;
                        }
                    }

                    // eat UTF8 char
                    auto code = base::utf8::NextChar(curPtr, endPtr);
                    if (code < 32)
                    {
                        if (code == '\n')
                        {
                            finishTextRegion(true);
                        }
                    }
                    else
                    {
                        codes.pushBack(code);
                    }
                }

                finishTextRegion();
                finishLine();
            }    
        };

        LayoutData::LayoutData(const base::StringBuf& txt)
            : text(txt)
        {
            LayutDataParser parser(txt);
            parser.process(*this);
        }

        //--

        struct FontSelector
        {
        public:
            FontSelector(const style::FontFamily& family, float sizeScale, bool bold, bool italic)
                : m_family(family)
                , m_baseSizeScale(sizeScale)
                , m_baseBold(bold)
                , m_baseItalic(italic)
            {
                selectFont(1.0f, false, false);
            }

            void selectFont(float scale, bool bold, bool italic)
            {
                bold |= m_baseBold;
                italic |= m_baseItalic;

                auto size = std::max<uint32_t>(1, std::floorf(m_baseSizeScale * scale));
                if (size <= 2 || !m_family.normal)
                {
                    m_currentFont = nullptr;
                    return;
                }

                m_currentStyleParams.size = size;

                if (!bold && !italic)
                {
                    m_currentFont = m_family.normal;
                    m_currentStyleParams.bold = false;
                    m_currentStyleParams.italic = false;
                }
                else if (bold)
                {
                    if (m_family.bold)
                    {
                        m_currentFont = m_family.bold;
                        m_currentStyleParams.bold = false;
                        m_currentStyleParams.italic = false;
                    }
                    else
                    {
                        m_currentFont = m_family.normal;
                        m_currentStyleParams.bold = true;
                        m_currentStyleParams.italic = false;
                    }
                }
                else if (italic)
                {
                    if (m_family.italic)
                    {
                        m_currentFont = m_family.italic;
                        m_currentStyleParams.bold = false;
                        m_currentStyleParams.italic = false;
                    }
                    else
                    {
                        m_currentFont = m_family.normal;
                        m_currentStyleParams.bold = false;
                        m_currentStyleParams.italic = true;
                    }
                }
                else
                {
                    if (m_family.boldItalic)
                    {
                        m_currentFont = m_family.boldItalic;
                        m_currentStyleParams.bold = false;
                        m_currentStyleParams.italic = false;
                    }
                    else if (m_family.italic)
                    {
                        m_currentFont = m_family.italic;
                        m_currentStyleParams.bold = true;
                        m_currentStyleParams.italic = false;
                    }
                    else if (m_family.bold)
                    {
                        m_currentFont = m_family.bold;
                        m_currentStyleParams.bold = false;
                        m_currentStyleParams.italic = true;
                    }
                    else
                    {
                        m_currentFont = m_family.normal;
                        m_currentStyleParams.bold = true;
                        m_currentStyleParams.italic = true;
                    }
                }
            }

            INLINE void updateLineMetrics(float& outAscender, float& outDescender)
            {
                if (m_currentFont)
                {
                    const auto fontAscender = std::round(m_currentFont->relativeAscender() * m_currentStyleParams.size);
                    const auto fontDescender = m_currentStyleParams.size - fontAscender;

                    outAscender = std::max<float>(outAscender, fontAscender);
                    outDescender = std::max<float>(outDescender, fontDescender);
                }
            }

            INLINE const base::font::Glyph* findGlyph(uint32_t code) const
            {
                return m_currentFont ? m_currentFont->renderGlyph(m_currentStyleParams, code) : nullptr;
            }

        private:
            const base::font::Font* m_currentFont = nullptr;
            base::font::FontStyleParams m_currentStyleParams;

            const style::FontFamily& m_family;
            float m_baseSizeScale;
            bool m_baseBold;
            bool m_baseItalic;
        };

        static const auto TAG_MARGIN = 4.0f; // experimental
        static const auto TAG_PADDING = 4.0f; // experimental

        void LayoutData::render(DataStash& stash, const style::FontFamily& fonts, float pixelScale, float fontSize, bool bold, bool italic, base::Color baseColor, float wrapWidth, LayoutDisplayData& outData) const
        {
            FontSelector fontSelector(fonts, fontSize, bold, italic);
            
            outData.glyphs.reset();
            outData.images.reset();
            outData.lines.reset();
            outData.tags.reset();

            float baseY = 0.0f;
            for (const auto& line : lines)
            {
                float lineTextAscender = 0.0f;
                float lineTextDescender = 0.0f;
                float lineImageMaxHeight = 0.0f;
                float lineX = 0.0f;
                float lineY = baseY;

                auto firstDisplayGlyph = outData.glyphs.size();
                auto firstDisplayImage = outData.images.size();
                auto firstDisplayTag = outData.tags.size();
                bool hasTag = false;
                base::Color tagColor;
                float tagStartX = 0.0f;

                for (uint32_t i = 0; i<line.numRegions; ++i)
                {
                    const auto& region = regions[i + line.firstRegion];

                    if (region.flagTag != hasTag)
                    {
                        if (hasTag)
                        {
                            lineX += TAG_MARGIN * pixelScale;
                            auto& displayTag = outData.tags.emplaceBack();
                            displayTag.color = tagColor;
                            displayTag.x0 = tagStartX;
                            displayTag.x1 = lineX;
                            displayTag.r = TAG_MARGIN * pixelScale;
                            lineX += TAG_PADDING * pixelScale;
                        }
                        else
                        {
                            lineX += TAG_PADDING * pixelScale;
                            tagColor = region.tagColor;
                            tagStartX = lineX;
                            lineX += TAG_MARGIN * pixelScale;
                        }

                        hasTag = region.flagTag;
                    }

                    auto color = baseColor;
                    if (region.flagColor)
                    {
                        color.r = ((uint32_t)region.color.r * (uint32_t)baseColor.r) / 255;
                        color.g = ((uint32_t)region.color.g * (uint32_t)baseColor.g) / 255;
                        color.b = ((uint32_t)region.color.b * (uint32_t)baseColor.b) / 255;
                        color.a = ((uint32_t)region.color.a * (uint32_t)baseColor.a) / 255;
                    }

                    if (region.imageName)
                    {
                        if (auto image = stash.loadImage(region.imageName).acquire())
                        {
                            auto& displayImage = outData.images.emplaceBack();
                            displayImage.image = image;
                            displayImage.pos.x = std::floorf(lineX);
                            displayImage.pos.y = baseY - image->height();
                            displayImage.color = color;
                            displayImage.valaign = region.valign;
                            lineX += image->width();

                            if (region.valign == LayoutVerticalAlign::Baseline)
                                lineTextAscender = std::max<float>(lineTextAscender, image->height());
                            else
                                lineImageMaxHeight = std::max<float>(lineImageMaxHeight, image->height());
                        }
                    }
                    else
                    {
                        fontSelector.selectFont(region.size, region.flagBold, region.flagItalic);
                        fontSelector.updateLineMetrics(lineTextAscender, lineTextDescender);

                        for (uint32_t j = 0; j < region.numCodes; ++j)
                        {
                            const auto code = codes[region.firstCode + j];
                            if (const auto* glyph = fontSelector.findGlyph(code))
                            {
                                auto& displayGlyph = outData.glyphs.emplaceBack();
                                displayGlyph.data.color = color;
                                displayGlyph.data.glyph = glyph;
                                displayGlyph.data.pos.x = std::floorf(lineX + glyph->offset().x);
                                displayGlyph.data.pos.y = std::floorf(lineY + glyph->offset().y);
                                lineX += glyph->advance().x;
                                lineY += glyph->advance().y;
                            }
                        }
                    }
                }

                // finish tag
                if (hasTag)
                {
                    lineX += TAG_MARGIN * pixelScale;
                    auto& displayTag = outData.tags.emplaceBack();
                    displayTag.color = tagColor;
                    displayTag.x0 = tagStartX;
                    displayTag.x1 = lineX;
                    displayTag.r = TAG_MARGIN * pixelScale;
                    lineX += TAG_PADDING * pixelScale;
                }

                float lineTextHeight = lineTextAscender + lineTextDescender;
                float lineTotalHeight = std::max<float>(lineImageMaxHeight, lineTextHeight);
                float lineTextVerticalShift = std::floorf(lineTextAscender + std::max<float>(0.0f, (lineImageMaxHeight - lineTextHeight) / 2));

                // shift text glyphs to proper place in line
                {
                    for (uint32_t i = firstDisplayGlyph; i < outData.glyphs.size(); ++i)
                        outData.glyphs[i].data.pos.y += lineTextVerticalShift;
                }

                // update tags - shift them to proper place
                {
                    for (uint32_t i = firstDisplayTag; i < outData.tags.size(); ++i)
                    {
                        auto& tag = outData.tags[i];
                        //tag.x0 += lineTextVerticalShift;
                        //tag.x1 += lineTextVerticalShift;
                        tag.y0 = baseY;
                        tag.y1 = baseY + lineTotalHeight;
                    }
                }

                // shift images according to the align
                {
                    for (uint32_t i = firstDisplayImage; i < outData.images.size(); ++i)
                    {
                        auto& image = outData.images[i];
                        if (image.valaign == LayoutVerticalAlign::Baseline)
                            image.pos.y += lineTextVerticalShift;
                        else if (image.valaign == LayoutVerticalAlign::Top)
                            image.pos.y = baseY;
                        else if (image.valaign == LayoutVerticalAlign::Middle)
                            image.pos.y = baseY + (lineTotalHeight - image.image->height()) / 2.0f;
                        else if (image.valaign == LayoutVerticalAlign::Bottom)
                            image.pos.y = baseY + lineTotalHeight - image.image->height();
                    }
                }

                // finish line
                auto& displayLine = outData.lines.emplaceBack();
                displayLine.y = baseY;
                displayLine.firstGlyph = firstDisplayGlyph;
                displayLine.firstImage = firstDisplayImage;
                displayLine.firstTag = firstDisplayTag;
                displayLine.numGlyphs = outData.glyphs.size() - firstDisplayGlyph;
                displayLine.numImages = outData.images.size() - firstDisplayImage;
                displayLine.numTags = outData.tags.size() - firstDisplayTag;
                displayLine.width = lineX;
                displayLine.height = lineTotalHeight;
                displayLine.alignment = line.halaign;

                // go to next line
                baseY += lineTotalHeight;

                // if we had tags leave some extra space after the line (HACK)
                if (&line != &lines.back())
                    baseY += 2.0f;
            }
        }

        //--

        void LayoutDisplayData::measure(Size& outSize) const
        {
            for (const auto& line : lines)
            {
                outSize.x = std::max<float>(outSize.x, line.width);
                outSize.y = std::max<float>(outSize.y, line.y + line.height);
            }
        }

        void LayoutDisplayData::render(base::canvas::GeometryBuilder& b, const ElementArea& area, LayoutHorizontalAlign defaultAlignment /*= LayoutHorizontalAlign::Left*/) const
        {
            for (const auto& line : lines)
            {
                // line alignment
                auto lineAlignment = (line.alignment == LayoutHorizontalAlign::Default) ? defaultAlignment : line.alignment;
                auto lineShiftX = 0.0f;
                if (lineAlignment == LayoutHorizontalAlign::Center)
                {
                    lineShiftX = std::floor((area.size().x - line.width) / 2.0f);

                    b.pushTransform();
                    b.translate(lineShiftX, 0.0f);
                }
                else if (lineAlignment == LayoutHorizontalAlign::Right)
                {
                    lineShiftX = std::floor(area.size().x - line.width);

                    b.pushTransform();
                    b.translate(lineShiftX, 0.0f);
                }
                
                // render the tags
                if (line.numTags)
                {
                    for (uint32_t i = 0; i < line.numTags; ++i)
                    {
                        const auto& tag = tags.typedData()[i + line.firstTag];

                        b.beginPath();
                        b.fillColor(tag.color);
                        b.roundedRect(tag.x0, tag.y0, tag.x1 - tag.x0, tag.y1 - tag.y0, tag.r);
                        b.fill();
                    }
                }

                // render glyphs
                if (line.numGlyphs)
                {
                    const auto* glyphData = glyphs.typedData() + line.firstGlyph;
                    b.print(glyphData, line.numGlyphs, sizeof(LayoutDisplayGlyph));
                }

                // render images
                for (uint32_t i = 0; i < line.numImages; ++i)
                {
                    const auto& icon = images.typedData()[line.firstImage + i];
                    b.pushTransform();
                    b.translate(icon.pos.x, icon.pos.y);

                    auto width = icon.image->width();
                    auto height = icon.image->height();
                    
                    auto imageStyle = base::canvas::ImagePattern(icon.image, base::canvas::ImagePatternSettings());
                    imageStyle.innerColor = imageStyle.outerColor = icon.color;
                    b.fillPaint(imageStyle);
                    b.beginPath();
                    b.compositeOperation(base::canvas::CompositeOperation::SourceOver);
                    b.rect(0, 0, width, height);
                    b.fill();

                    b.popTransform();
                }

                // restore transform
                if (lineAlignment == LayoutHorizontalAlign::Center || lineAlignment == LayoutHorizontalAlign::Right)
                    b.popTransform();
            }
        }

        //--
    
    } // prv
} // ui
