/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/


#include "build.h"
#include "renderingCanvasTest.h"

#include "base/canvas/include/canvasGeometryBuilder.h"
#include "base/canvas/include/canvasGeometry.h"
#include "base/canvas/include/canvas.h"
#include "base/canvas/include/canvasStyle.h"
#include "base/font/include/font.h"
#include "base/font/include/fontGlyphBuffer.h"
#include "base/font/include/fontInputText.h"
#include "base/font/include/fontGlyphCache.h"
#include "base/containers/include/utf8StringFunctions.h"
#include "base/font/include/fontGlyph.h"
#include "base/input/include/inputStructures.h"

#ifdef PLATFORM_MSVC
#pragma warning (disable: 4706) // assignment within conditional expression
#endif

namespace rendering
{
    namespace test
    {

        static const wchar_t* ICON_SEARCH = L"\uF50D";
        static const wchar_t* ICON_CIRCLED_CROSS = L"\u2716";
        static const wchar_t* ICON_CHEVRON_RIGHT = L"\uE75E";
        static const wchar_t* ICON_CHECK = L"\u2713";
        static const wchar_t* ICON_LOGIN = L"\uE740";
        static const wchar_t* ICON_TRASH = L"\uE729";

        enum FontHelperFlags
        {
            NVG_ALIGN_LEFT = 1 << 0,
            NVG_ALIGN_CENTER = 1 << 1,
            NVG_ALIGN_RIGHT = 1 << 2,
            NVG_ALIGN_TOP = 1 << 3,
            NVG_ALIGN_MIDDLE = 1 << 4,
            NVG_ALIGN_BOTTOM = 1 << 5,
            NVG_ALIGN_BASELINE = 1 << 6,
        };

        struct NVGglyphPosition {
            const char* str;	// Position of the glyph in the input string.
            float x;			// The x-coordinate of the logical glyph position.
            float minx, maxx;	// The bounds of the glyph shape.
        };

        struct NVGtextRow {
            const char* start;	// Pointer to the input text where the row starts.
            const char* end;	// Pointer to the input text where the row ends (one past the last character).
            const char* next;	// Pointer to the beginning of the next row.
            float width;		// Logical width of the row.
            float minx, maxx;	// Actual bounds of the row. Logical with and bounds can differ because of kerning and some parts over extending.
        };


        struct FontHelper
        {
        public:
            FontHelper()
                : m_size(10)
                , m_verticalAlignment(base::font::FontAlignmentVertical::Top)
                , m_horizontalAlignment(base::font::FontAlignmentHorizontal::Left)
            {
            }

            void init(ICanvasTest& test)
            {
                m_sansFont = test.loadFont("roboto_regular.ttf");
                m_sansBoldFont = test.loadFont("roboto_bold.ttf");
                m_iconFont = test.loadFont("icons1.ttf");
                m_emojiFont = test.loadFont("emoji.ttf");
                m_currentFace = m_sansFont;
            }

            void fontSize(float size)
            {
                m_size = (uint32_t)size;
            }

            float lineHeight() const
            {
                return m_currentFace->lineSeparation(m_size);
            }

            void fontFace(const base::StringBuf& ff)
            {
                if (ff == "sans-bold")
                    m_currentFace = m_sansBoldFont;
                else  if (ff == "icons")
                    m_currentFace = m_iconFont;
                else  if (ff == "emoji")
                    m_currentFace = m_emojiFont;
                else
                    m_currentFace = m_sansFont;
            }

            void textAlign(uint32_t flags)
            {
                if (flags & NVG_ALIGN_LEFT)
                    m_horizontalAlignment = base::font::FontAlignmentHorizontal::Left;
                else if (flags & NVG_ALIGN_CENTER)
                    m_horizontalAlignment = base::font::FontAlignmentHorizontal::Center;
                else if (flags & NVG_ALIGN_RIGHT)
                    m_horizontalAlignment = base::font::FontAlignmentHorizontal::Right;

                if (flags & NVG_ALIGN_TOP)
                    m_verticalAlignment = base::font::FontAlignmentVertical::Top;
                else if (flags & NVG_ALIGN_MIDDLE)
                    m_verticalAlignment = base::font::FontAlignmentVertical::Middle;
                else if (flags & NVG_ALIGN_BOTTOM)
                    m_verticalAlignment = base::font::FontAlignmentVertical::Bottom;
                else if (flags & NVG_ALIGN_BASELINE)
                    m_verticalAlignment = base::font::FontAlignmentVertical::Baseline;
            }

            void fontBlur(uint32_t blur)
            {
            }

            float textBounds(float x, float y, const char* string, const char* end, float* bounds)
            {
                base::font::FontStyleParams params;
                params.size = m_size;

                base::font::GlyphBuffer glyphs;
                base::font::FontAssemblyParams assemblyParams;
                base::font::FontMetrics metrics;
                if (m_currentFace)
                    m_currentFace->measureText(params, assemblyParams, base::font::FontInputText(string), metrics);

                return (float)metrics.textWidth;
            }

            float textBounds(float x, float y, const wchar_t* string, const wchar_t* end, float* bounds)
            {
                base::font::FontStyleParams params;
                params.size = m_size;

                base::font::GlyphBuffer glyphs;
                base::font::FontAssemblyParams assemblyParams;
                base::font::FontMetrics metrics;
                if (m_currentFace)
                    m_currentFace->measureText(params, assemblyParams, base::font::FontInputText(string), metrics);

                return (float)metrics.textWidth;
            }

            void text(base::canvas::GeometryBuilder& vg, float x, float y, const char* txt, void* ptr)
            {
                base::font::FontStyleParams params;
                params.size = m_size;

                base::font::GlyphBuffer glyphs;
                base::font::FontAssemblyParams assemblyParams;
                assemblyParams.verticalAlignment = m_verticalAlignment;
                assemblyParams.horizontalAlignment = m_horizontalAlignment;
                if (m_currentFace)
                    m_currentFace->renderText(params, assemblyParams, base::font::FontInputText(txt), glyphs);

                vg.pushTransform();
                vg.translate(x, y);
                vg.print(glyphs);
                vg.popTransform();
            }

            void text2(base::canvas::GeometryBuilder& vg, float x, float y, const char* txt, const char* endTxt)
            {
                base::font::FontStyleParams params;
                params.size = m_size;

                base::font::GlyphBuffer glyphs;
                base::font::FontAssemblyParams assemblyParams;
                assemblyParams.verticalAlignment = m_verticalAlignment;
                assemblyParams.horizontalAlignment = m_horizontalAlignment;
                if (m_currentFace)
                    m_currentFace->renderText(params, assemblyParams, base::font::FontInputText(txt, endTxt - txt), glyphs);

                vg.pushTransform();
                vg.translate(x, y);
                vg.print(glyphs);
                vg.popTransform();
            }

            void text(base::canvas::GeometryBuilder& vg, float x, float y, const wchar_t* txt, void* ptr)
            {
                base::font::FontStyleParams params;
                params.size = m_size;

                base::font::GlyphBuffer glyphs;
                base::font::FontAssemblyParams assemblyParams;
                assemblyParams.verticalAlignment = m_verticalAlignment;
                assemblyParams.horizontalAlignment = m_horizontalAlignment;
                if (m_currentFace)
                    m_currentFace->renderText(params, assemblyParams, base::font::FontInputText(txt), glyphs);

                vg.pushTransform();
                vg.translate(x, y);
                vg.print(glyphs);
                vg.popTransform();
            }

            enum NVGcodepointType {
                NVG_SPACE,
                NVG_NEWLINE,
                NVG_CHAR,
                NVG_CJK_CHAR,
            };

            struct FONStextIter {
                float x, y, nextx, nexty, scale, spacing;
                unsigned int codepoint;
                short isize, iblur;
                const base::font::Font* font;
                int ch;
                const char* str;
                const char* next;
                const char* end;
                unsigned int utf8state;
                int bitmapOption;
            };

            struct FONSquad
            {
                float x0, y0, s0, t0;
                float x1, y1, s1, t1;
            };

            static void fons__getQuad(const base::font::Font* font, int prevGlyphIndex, const base::font::Glyph* glyph, float scale, float spacing, float* x, float* y, FONSquad* q)
            {
                q->x0 = *x + glyph->offset().x;
                q->y0 = *y + glyph->offset().y;
                q->x1 = q->x0 + glyph->size().x;
                q->y1 = q->x1 + glyph->size().y;

                *x += glyph->advance().x;
            }

            int fonsTextIterNext(FONStextIter* iter, FONSquad* quad)
            {
                const base::font::Glyph* glyph = NULL;
                const char* str = iter->next;
                iter->str = iter->next;

                if (str >= iter->end)
                    return 0;

                base::font::FontStyleParams style;
                style.size = iter->isize;

                while (str < iter->end)
                {
                    auto ch = base::utf8::NextChar(str, iter->end);
                    // Get glyph and quad
                    iter->x = iter->nextx;
                    iter->y = iter->nexty;
                    glyph = iter->font->renderGlyph(style, ch);
                    // If the iterator was initialized with FONS_GLYPH_BITMAP_OPTIONAL, then the UV coordinates of the quad will be invalid.
                    if (glyph != NULL)
                        fons__getQuad(iter->font, 0, glyph, iter->scale, iter->spacing, &iter->nextx, &iter->nexty, quad);
                    iter->ch = ch;
                    break;
                }
                iter->next = str;

                return 1;
            }


            int fonsTextIterInit(FONStextIter* iter, float x, float y, const char* str, const char* end, int bitmapOption)
            {
                //float width;

                memset(iter, 0, sizeof(*iter));

                //if (stash == NULL) return 0;
                //if (state->font < 0 || state->font >= stash->nfonts) return 0;
                iter->font = m_currentFace.get();
                //if (iter->font->data == NULL) return 0;

                iter->isize = (short)(m_size);
                iter->iblur = 0; //(short)state->blur;
                iter->scale = 1.0f;// fons__tt_getPixelHeightScale(&iter->font->font, (float)iter->isize / 10.0f);

                // Align horizontally
                /*if (horizontalAlignment & FONS_ALIGN_LEFT) {
                    // empty
                }
                else if (state->align & FONS_ALIGN_RIGHT) {
                    width = fonsTextBounds(stash, x, y, str, end, NULL);
                    x -= width;
                }
                else if (state->align & FONS_ALIGN_CENTER) {
                    width = fonsTextBounds(stash, x, y, str, end, NULL);
                    x -= width * 0.5f;
                }
                // Align vertically.
                y += fons__getVertAlign(stash, iter->font, state->align, iter->isize);*/

                if (end == NULL)
                    end = str + strlen(str);

                iter->x = iter->nextx = x;
                iter->y = iter->nexty = y;
                iter->spacing = 0;// state->spacing;
                iter->str = str;
                iter->next = str;
                iter->end = end;
                iter->codepoint = 0;
                iter->bitmapOption = bitmapOption;

                return 1;
            }

            void textBox(base::canvas::GeometryBuilder& vg, float x, float y, float breakRowWidth, const char* string, const char* end)
            {
                NVGtextRow rows[2];
                int nrows = 0, i;
                //int oldAlign = state->textAlign;
                int haling = NVG_ALIGN_LEFT;// state->textAlign& (NVG_ALIGN_LEFT | NVG_ALIGN_CENTER | NVG_ALIGN_RIGHT);
                //int valign = state->textAlign & (NVG_ALIGN_TOP | NVG_ALIGN_MIDDLE | NVG_ALIGN_BOTTOM | NVG_ALIGN_BASELINE);
                float lineh = 0;

                //if (state->fontId == FONS_INVALID) return;

                lineh = lineHeight();

                while ((nrows = textBreakLines(string, end, breakRowWidth, rows, 2))) {
                    for (i = 0; i < nrows; i++) {
                        NVGtextRow* row = &rows[i];
                        if (haling & NVG_ALIGN_LEFT)
                            text2(vg, x, y, row->start, row->end);
                        else if (haling & NVG_ALIGN_CENTER)
                            text2(vg, x + breakRowWidth * 0.5f - row->width * 0.5f, y, row->start, row->end);
                        else if (haling & NVG_ALIGN_RIGHT)
                            text2(vg, x + breakRowWidth - row->width, y, row->start, row->end);
                        y += lineh;// *state->lineHeight;
                    }
                    string = rows[nrows - 1].next;
                }

                //state->textAlign = oldAlign;
            }


            void textBoxBounds(float x, float y, float breakRowWidth, const char* string, const char* end, float* bounds)
            {
                NVGtextRow rows[2];
                float scale = 1.0f;
                float invscale = 1.0f / scale;
                int nrows = 0, i;
                int oldAlign = 0;// state->textAlign;
                int haling = 0;// state->textAlign& (NVG_ALIGN_LEFT | NVG_ALIGN_CENTER | NVG_ALIGN_RIGHT);
                int valign = 0;// state->textAlign& (NVG_ALIGN_TOP | NVG_ALIGN_MIDDLE | NVG_ALIGN_BOTTOM | NVG_ALIGN_BASELINE);
                float lineh = 0, rminy = 0, rmaxy = 0;
                float minx, miny, maxx, maxy;

                /*if (state->fontId == FONS_INVALID) {
                    if (bounds != NULL)
                        bounds[0] = bounds[1] = bounds[2] = bounds[3] = 0.0f;
                    return;
                }*/

                lineh = lineHeight();

                //state->textAlign = NVG_ALIGN_LEFT | valign;

                minx = maxx = x;
                miny = maxy = y;

                /*fonsSetSize(ctx->fs, state->fontSize * scale);
                fonsSetSpacing(ctx->fs, state->letterSpacing * scale);
                fonsSetBlur(ctx->fs, state->fontBlur * scale);
                fonsSetAlign(ctx->fs, state->textAlign);
                fonsSetFont(ctx->fs, state->fontId);
                fonsLineBounds(ctx->fs, 0, &rminy, &rmaxy);*/
                rminy = invscale * m_currentFace->relativeDescender() * m_size;
                rmaxy = invscale * m_currentFace->relativeAscender() * m_size;

                while (nrows = textBreakLines(string, end, breakRowWidth, rows, 2)) {
                    for (i = 0; i < nrows; i++) {
                        NVGtextRow* row = &rows[i];
                        float rminx, rmaxx, dx = 0;
                        // Horizontal bounds
                        if (haling & NVG_ALIGN_LEFT)
                            dx = 0;
                        else if (haling & NVG_ALIGN_CENTER)
                            dx = breakRowWidth * 0.5f - row->width * 0.5f;
                        else if (haling & NVG_ALIGN_RIGHT)
                            dx = breakRowWidth - row->width;
                        rminx = x + row->minx + dx;
                        rmaxx = x + row->maxx + dx;
                        minx = std::min<float>(minx, rminx);
                        maxx = std::max<float>(maxx, rmaxx);
                        // Vertical bounds.
                        miny = std::min<float>(miny, y + rminy);
                        maxy = std::max<float>(maxy, y + rmaxy);

                        y += lineh;// *state->lineHeight;
                    }
                    string = rows[nrows - 1].next;
                }

                //state->textAlign = oldAlign;

                if (bounds != NULL) {
                    bounds[0] = minx;
                    bounds[1] = miny;
                    bounds[2] = maxx;
                    bounds[3] = maxy;
                }
            }

            int textBreakLines(const char* string, const char* end, float breakRowWidth, NVGtextRow* rows, int maxRows)
            {
                float scale = 1.0f;//
                float invscale = 1.0f / scale;
                int nrows = 0;
                float rowStartX = 0;
                float rowWidth = 0;
                float rowMinX = 0;
                float rowMaxX = 0;
                const char* rowStart = NULL;
                const char* rowEnd = NULL;
                const char* wordStart = NULL;
                float wordStartX = 0;
                float wordMinX = 0;
                const char* breakEnd = NULL;
                float breakWidth = 0;
                float breakMaxX = 0;
                int type = NVG_SPACE, ptype = NVG_SPACE;
                unsigned int pchar = 0;
                FONStextIter iter;// , prevIter;
                FONSquad q;

                if (maxRows == 0) return 0;

                if (end == NULL)
                    end = string + strlen(string);

                if (string == end) return 0;

                breakRowWidth *= scale;

                fonsTextIterInit( &iter, 0, 0, string, end, 0);
                //prevIter = iter;
                while (fonsTextIterNext(&iter, &q)) {
                    switch (iter.ch) {
                    case 9:			// \t
                    case 11:		// \v
                    case 12:		// \f
                    case 32:		// space
                    case 0x00a0:	// NBSP
                        type = NVG_SPACE;
                        break;
                    case 10:		// \n
                        type = pchar == 13 ? NVG_SPACE : NVG_NEWLINE;
                        break;
                    case 13:		// \r
                        type = pchar == 10 ? NVG_SPACE : NVG_NEWLINE;
                        break;
                    case 0x0085:	// NEL
                        type = NVG_NEWLINE;
                        break;
                    default:
                        if ((iter.ch >= 0x4E00 && iter.ch <= 0x9FFF) ||
                            (iter.ch >= 0x3000 && iter.ch <= 0x30FF) ||
                            (iter.ch >= 0xFF00 && iter.ch <= 0xFFEF) ||
                            (iter.ch >= 0x1100 && iter.ch <= 0x11FF) ||
                            (iter.ch >= 0x3130 && iter.ch <= 0x318F) ||
                            (iter.ch >= 0xAC00 && iter.ch <= 0xD7AF))
                            type = NVG_CJK_CHAR;
                        else
                            type = NVG_CHAR;
                        break;
                    }

                    if (type == NVG_NEWLINE) {
                        // Always handle new lines.
                        rows[nrows].start = rowStart != NULL ? rowStart : iter.str;
                        rows[nrows].end = rowEnd != NULL ? rowEnd : iter.str;
                        rows[nrows].width = rowWidth * invscale;
                        rows[nrows].minx = rowMinX * invscale;
                        rows[nrows].maxx = rowMaxX * invscale;
                        rows[nrows].next = iter.next;
                        nrows++;
                        if (nrows >= maxRows)
                            return nrows;
                        // Set null break point
                        breakEnd = rowStart;
                        breakWidth = 0.0;
                        breakMaxX = 0.0;
                        // Indicate to skip the white space at the beginning of the row.
                        rowStart = NULL;
                        rowEnd = NULL;
                        rowWidth = 0;
                        rowMinX = rowMaxX = 0;
                    }
                    else {
                        if (rowStart == NULL) {
                            // Skip white space until the beginning of the line
                            if (type == NVG_CHAR || type == NVG_CJK_CHAR) {
                                // The current char is the row so far
                                rowStartX = iter.x;
                                rowStart = iter.str;
                                rowEnd = iter.next;
                                rowWidth = iter.nextx - rowStartX; // q.x1 - rowStartX;
                                rowMinX = q.x0 - rowStartX;
                                rowMaxX = q.x1 - rowStartX;
                                wordStart = iter.str;
                                wordStartX = iter.x;
                                wordMinX = q.x0 - rowStartX;
                                // Set null break point
                                breakEnd = rowStart;
                                breakWidth = 0.0;
                                breakMaxX = 0.0;
                            }
                        }
                        else {
                            float nextWidth = iter.nextx - rowStartX;

                            // track last non-white space character
                            if (type == NVG_CHAR || type == NVG_CJK_CHAR) {
                                rowEnd = iter.next;
                                rowWidth = iter.nextx - rowStartX;
                                rowMaxX = q.x1 - rowStartX;
                            }
                            // track last end of a word
                            if (((ptype == NVG_CHAR || ptype == NVG_CJK_CHAR) && type == NVG_SPACE) || type == NVG_CJK_CHAR) {
                                breakEnd = iter.str;
                                breakWidth = rowWidth;
                                breakMaxX = rowMaxX;
                            }
                            // track last beginning of a word
                            if ((ptype == NVG_SPACE && (type == NVG_CHAR || type == NVG_CJK_CHAR)) || type == NVG_CJK_CHAR) {
                                wordStart = iter.str;
                                wordStartX = iter.x;
                                wordMinX = q.x0 - rowStartX;
                            }

                            // Break to new line when a character is beyond break width.
                            if ((type == NVG_CHAR || type == NVG_CJK_CHAR) && nextWidth > breakRowWidth) {
                                // The run length is too long, need to break to new line.
                                if (breakEnd == rowStart) {
                                    // The current word is longer than the row length, just break it from here.
                                    rows[nrows].start = rowStart;
                                    rows[nrows].end = iter.str;
                                    rows[nrows].width = rowWidth * invscale;
                                    rows[nrows].minx = rowMinX * invscale;
                                    rows[nrows].maxx = rowMaxX * invscale;
                                    rows[nrows].next = iter.str;
                                    nrows++;
                                    if (nrows >= maxRows)
                                        return nrows;
                                    rowStartX = iter.x;
                                    rowStart = iter.str;
                                    rowEnd = iter.next;
                                    rowWidth = iter.nextx - rowStartX;
                                    rowMinX = q.x0 - rowStartX;
                                    rowMaxX = q.x1 - rowStartX;
                                    wordStart = iter.str;
                                    wordStartX = iter.x;
                                    wordMinX = q.x0 - rowStartX;
                                }
                                else {
                                    // Break the line from the end of the last word, and start new line from the beginning of the new.
                                    rows[nrows].start = rowStart;
                                    rows[nrows].end = breakEnd;
                                    rows[nrows].width = breakWidth * invscale;
                                    rows[nrows].minx = rowMinX * invscale;
                                    rows[nrows].maxx = breakMaxX * invscale;
                                    rows[nrows].next = wordStart;
                                    nrows++;
                                    if (nrows >= maxRows)
                                        return nrows;
                                    rowStartX = wordStartX;
                                    rowStart = wordStart;
                                    rowEnd = iter.next;
                                    rowWidth = iter.nextx - rowStartX;
                                    rowMinX = wordMinX;
                                    rowMaxX = q.x1 - rowStartX;
                                    // No change to the word start
                                }
                                // Set null break point
                                breakEnd = rowStart;
                                breakWidth = 0.0;
                                breakMaxX = 0.0;
                            }
                        }
                    }

                    pchar = iter.ch;
                    ptype = type;
                }

                // Break the line from the end of the last word, and start new line from the beginning of the new.
                if (rowStart != NULL) {
                    rows[nrows].start = rowStart;
                    rows[nrows].end = rowEnd;
                    rows[nrows].width = rowWidth * invscale;
                    rows[nrows].minx = rowMinX * invscale;
                    rows[nrows].maxx = rowMaxX * invscale;
                    rows[nrows].next = end;
                    nrows++;
                }

                return nrows;
            }

            int textGlyphPositions(float x, float y, const char* string, const char* end, NVGglyphPosition* positions, int maxPositions)
            {
                float scale = 1.0f;
                float invscale = 1.0f / scale;
                FONStextIter iter, prevIter;
                FONSquad q;
                int npos = 0;

                if (end == NULL)
                    end = string + strlen(string);

                if (string == end)
                    return 0;

                /*fonsSetSize(ctx->fs, state->fontSize * scale);
                fonsSetSpacing(ctx->fs, state->letterSpacing * scale);
                fonsSetBlur(ctx->fs, state->fontBlur * scale);
                fonsSetAlign(ctx->fs, state->textAlign);
                fonsSetFont(ctx->fs, state->fontId);*/

                fonsTextIterInit( &iter, x * scale, y * scale, string, end, 0);
                prevIter = iter;
                while (fonsTextIterNext(&iter, &q)) {
                    prevIter = iter;
                    positions[npos].str = iter.str;
                    positions[npos].x = iter.x * invscale;
                    positions[npos].minx = std::min<float>(iter.x, q.x0) * invscale;
                    positions[npos].maxx = std::max<float>(iter.nextx, q.x1) * invscale;
                    npos++;
                    if (npos >= maxPositions)
                        break;
                }

                return npos;
            }

        private:
            base::FontPtr m_sansFont;
            base::FontPtr m_sansBoldFont;
            base::FontPtr m_iconFont;
            base::FontPtr m_emojiFont;

            base::font::GlyphCache m_glyphCache;

            uint32_t m_size;
            base::FontPtr m_currentFace;

            base::font::FontAlignmentVertical m_verticalAlignment;
            base::font::FontAlignmentHorizontal m_horizontalAlignment;
        };

        /// test of all of the canvas feature
        class SceneTest_CanvasEverything : public ICanvasTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_CanvasEverything, ICanvasTest);

        public:
            virtual void initialize() override;
            virtual void render(base::canvas::Canvas& canvas) override;
            virtual void processInput(const base::input::BaseEvent& evt) override;

        private:
            base::Array<base::image::ImagePtr> m_images;
            FontHelper m_fontHelper;

            base::canvas::Geometry m_staticGeometry;

            float m_mouseX;
            float m_mouseY;
        };

        //---       

        void SceneTest_CanvasEverything::initialize()
        {
            m_images.pushBack(loadImage("image1.png"));
            m_images.pushBack(loadImage("image2.png"));
            m_images.pushBack(loadImage("image3.png"));
            m_images.pushBack(loadImage("image4.png"));
            m_images.pushBack(loadImage("image5.png"));
            m_images.pushBack(loadImage("image6.png"));
            m_images.pushBack(loadImage("image7.png"));
            m_images.pushBack(loadImage("image8.png"));
            m_images.pushBack(loadImage("image9.png"));
            m_images.pushBack(loadImage("image10.png"));
            m_images.pushBack(loadImage("image11.png"));
            m_images.pushBack(loadImage("image12.png"));

            m_fontHelper.init(*this);

            m_mouseX = 100.0f;
            m_mouseY = 100.0f;
        }

        void SceneTest_CanvasEverything::processInput(const base::input::BaseEvent& evt)
        {
            if (auto moveEvent  = evt.toMouseMoveEvent())
            {
                m_mouseX = moveEvent->windowPosition().x;
                m_mouseY = moveEvent->windowPosition().y;
            }
        }

        INLINE float SRGBToLinear(float srgb)
        {
            if (srgb <= 0.04045f)
                return srgb / 12.92f;
            else
                return powf((srgb + 0.055f) / 1.055f, 2.4f);
        }

        static base::Color MakeColorLinear(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
        {
            return base::Color(r,g,b,a);
        }

        static void DrawGraph(base::canvas::GeometryBuilder& vg, float x, float y, float w, float h, float t)
        {
            float samples[6];
            samples[0] = (1 + std::sin(t*1.2345f + std::cos(t*0.33457f)*0.44f))*0.5f;
            samples[1] = (1 + std::sin(t*0.68363f + std::cos(t*1.3f)*1.55f))*0.5f;
            samples[2] = (1 + std::sin(t*1.1642f + std::cos(t*0.33457f)*1.24f))*0.5f;
            samples[3] = (1 + std::sin(t*0.56345f + std::cos(t*1.63f)*0.14f))*0.5f;
            samples[4] = (1 + std::sin(t*1.6245f + std::cos(t*0.254f)*0.3f))*0.5f;
            samples[5] = (1 + std::sin(t*0.345f + std::cos(t*0.03f)*0.6f))*0.5f;

            float sx[6], sy[6];
            float dx = w / 5.0f;
            for (int i = 0; i < 6; i++)
            {
                sx[i] = x + i*dx;
                sy[i] = y + h*samples[i] * 0.8f;
            }

            // Graph background
            auto bg = base::canvas::LinearGradient(x, y, x, y + h, MakeColorLinear(0, 160, 192, 0), MakeColorLinear(0, 160, 192, 64));          
            vg.beginPath();         
            vg.moveTo(sx[0], sy[0]);
            for (int i = 1; i < 6; i++)             
                vg.bezierTo(sx[i - 1] + dx*0.5f, sy[i - 1], sx[i] - dx*0.5f, sy[i], sx[i], sy[i]);
            vg.lineTo(x + w, y + h);
            vg.lineTo(x, y + h);
            vg.fillPaint(bg);
            vg.fill();

            // Graph line
            vg.beginPath();
            vg.moveTo(sx[0], sy[0] + 2);
            for (int i = 1; i < 6; i++)
                vg.bezierTo(sx[i - 1] + dx*0.5f, sy[i - 1] + 2, sx[i] - dx*0.5f, sy[i] + 2, sx[i], sy[i] + 2);
            vg.strokeColor(MakeColorLinear(0, 0, 0, 32));
            vg.strokeWidth(3.0f);
            vg.stroke();

            vg.beginPath();
            vg.moveTo(sx[0], sy[0]);
            for (int i = 1; i < 6; i++)
                vg.bezierTo(sx[i - 1] + dx*0.5f, sy[i - 1], sx[i] - dx*0.5f, sy[i], sx[i], sy[i]);
            vg.strokeColor(MakeColorLinear(0, 160, 192, 255));
            vg.strokeWidth(3.0f);
            vg.stroke();

            // Graph sample pos
            for (int i = 0; i < 6; i++) {
                bg = base::canvas::RadialGradient(sx[i], sy[i] + 2, 3.0f, 8.0f, MakeColorLinear(0, 0, 0, 32), MakeColorLinear(0, 0, 0, 0));
                vg.beginPath();
                vg.rect(sx[i] - 10, sy[i] - 10 + 2, 20.0f, 20.0f);
                vg.fillPaint(bg);
                vg.fill();
            }

            vg.beginPath();
            for (int i = 0; i < 6; i++)
                vg.circle(sx[i], sy[i], 4.0f);
            vg.fillColor(MakeColorLinear(0, 160, 192, 255));
            vg.fill();
            vg.beginPath();
            for (int i = 0; i < 6; i++)
                vg.circle(sx[i], sy[i], 2.0f);
            vg.fillColor(MakeColorLinear(220, 220, 220, 255));
            vg.fill();

            vg.strokeWidth(1.0f);
        }

        static void DrawEyes(base::canvas::GeometryBuilder& vg, float x, float y, float w, float h, float mx, float my, float t)
        {
            base::canvas::RenderStyle gloss, bg;
            float ex = w *0.23f;
            float ey = h * 0.5f;
            float lx = x + ex;
            float ly = y + ey;
            float rx = x + w - ex;
            float ry = y + ey;
            float dx, dy, d;
            float br = (ex < ey ? ex : ey) * 0.5f;
            float blink = 1 - pow(sin(t*0.5f), 200)*0.8f;

            bg = base::canvas::LinearGradient( x, y + h*0.5f, x + w*0.1f, y + h, MakeColorLinear(0, 0, 0, 32), MakeColorLinear(0, 0, 0, 16));
            vg.beginPath();
            vg.ellipse(lx + 3.0f, ly + 16.0f, ex, ey);
            vg.ellipse(rx + 3.0f, ry + 16.0f, ex, ey);
            vg.fillPaint(bg);
            vg.fill();

            bg = base::canvas::LinearGradient( x, y + h*0.25f, x + w*0.1f, y + h, MakeColorLinear(220, 220, 220, 255), MakeColorLinear(128, 128, 128, 255));
            vg.beginPath();
            vg.ellipse(lx, ly, ex, ey);
            vg.ellipse(rx, ry, ex, ey);
            vg.fillPaint(bg);
            vg.fill();

            dx = (mx - rx) / (ex * 10);
            dy = (my - ry) / (ey * 10);
            d = sqrtf(dx*dx + dy*dy);
            if (d > 1.0f) {
                dx /= d; dy /= d;
            }
            dx *= ex*0.4f;
            dy *= ey*0.5f;
            vg.beginPath();
            vg.ellipse(lx + dx, ly + dy + ey*0.25f*(1 - blink), br, br*blink);
            vg.fillColor(MakeColorLinear(32, 32, 32, 255));
            vg.fill();

            dx = (mx - rx) / (ex * 10);
            dy = (my - ry) / (ey * 10);
            d = sqrtf(dx*dx + dy*dy);
            if (d > 1.0f) {
                dx /= d; dy /= d;
            }
            dx *= ex*0.4f;
            dy *= ey*0.5f;
            vg.beginPath();
            vg.ellipse(rx + dx, ry + dy + ey*0.25f*(1 - blink), br, br*blink);
            vg.fillColor(MakeColorLinear(32, 32, 32, 255));
            vg.fill();

            gloss = base::canvas::RadialGradient(lx - ex*0.25f, ly - ey*0.5f, ex*0.1f, ex*0.75f, MakeColorLinear(255, 255, 255, 128), MakeColorLinear(255, 255, 255, 0));
            vg.beginPath();
            vg.ellipse(lx, ly, ex, ey);
            vg.fillPaint(gloss);
            vg.fill();

            gloss = base::canvas::RadialGradient(rx - ex*0.25f, ry - ey*0.5f, ex*0.1f, ex*0.75f, MakeColorLinear(255, 255, 255, 128), MakeColorLinear(255, 255, 255, 0));
            vg.beginPath();
            vg.ellipse(rx, ry, ex, ey);
            vg.fillPaint(gloss);
            vg.fill();
        }

        static void DrawCaps(base::canvas::GeometryBuilder& vg, float x, float y, float width)
        {
            int i;
            base::canvas::LineCap caps[3] = { base::canvas::LineCap::Butt, base::canvas::LineCap::Round, base::canvas::LineCap::Square };
            float lineWidth = 8.0f;

            vg.pushTransform();

            vg.beginPath();
            vg.rect(x - lineWidth / 2, y, width + lineWidth, 40.0f);
            vg.fillColor(MakeColorLinear(255, 255, 255, 32));
            vg.fill();

            vg.beginPath();
            vg.rect(x, y, width, 40.0f);
            vg.fillColor(MakeColorLinear(255, 255, 255, 32));
            vg.fill();

            vg.strokeWidth(lineWidth);
            for (i = 0; i < 3; i++) {
                vg.lineCap(caps[i]);
                vg.strokeColor(MakeColorLinear(0, 0, 0, 255));
                vg.beginPath();
                vg.moveTo(x, y + i * 10 + 5);
                vg.lineTo(x + width, y + i * 10 + 5);
                vg.stroke();
            }

            vg.popTransform();
        }

        void DrawWindow(FontHelper& fh, base::canvas::GeometryBuilder& vg, const char* title, float x, float y, float w, float h)
        {
            float cornerRadius = 3.0f;
            base::canvas::RenderStyle shadowPaint;
            base::canvas::RenderStyle headerPaint;

            vg.pushTransform();
            //  nvgClearState(vg);

            // Window
            vg.beginPath();
            vg.roundedRect(x, y, w, h, cornerRadius);
            vg.fillColor(MakeColorLinear(28, 30, 34, 192));
            //  vg.fillColor(MakeColorLinear(0,0,0,128));
            vg.fill();

            // Drop shadow
            shadowPaint = base::canvas::BoxGradient(x, y + 2, w, h, cornerRadius * 2, 10.0f, MakeColorLinear(0, 0, 0, 128), MakeColorLinear(0, 0, 0, 0));
            vg.beginPath();
            vg.rect(x - 10, y - 10, w + 20, h + 30);
            vg.roundedRect(x, y, w, h, cornerRadius);           
            vg.pathWinding(base::canvas::Winding::CW);
            vg.fillPaint(shadowPaint);
            vg.fill();

            // Header
            headerPaint = base::canvas::LinearGradient(x, y, x, y + 15, MakeColorLinear(255, 255, 255, 8), MakeColorLinear(0, 0, 0, 16));
            vg.beginPath();
            vg.roundedRect(x + 1, y + 1, w - 2, 30.0f, cornerRadius - 1);
            vg.fillPaint(headerPaint);
            vg.fill();
            vg.beginPath();
            vg.moveTo(x + 0.5f, y + 0.5f + 30);
            vg.lineTo(x + 0.5f + w - 1, y + 0.5f + 30);
            vg.strokeColor(MakeColorLinear(0, 0, 0, 32));
            vg.stroke();

            fh.fontSize(18.0f);
            fh.fontFace("sans-bold");
            fh.textAlign(NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

            fh.fontBlur(2);
            vg.fillColor(MakeColorLinear(0, 0, 0, 128));
            fh.text(vg,x + w / 2, y + 16 + 1, title, NULL);

            fh.fontBlur(0);
            vg.fillColor(MakeColorLinear(220, 220, 220, 160));
            fh.text(vg,x + w / 2, y + 16, title, NULL);

            vg.popTransform();
        }
            
        static float nvg__hue(float h, float m1, float m2)
        {
            if (h < 0) h += 1;
            if (h > 1) h -= 1;
            if (h < 1.0f / 6.0f)
                return m1 + (m2 - m1) * h * 6.0f;
            else if (h < 3.0f / 6.0f)
                return m2;
            else if (h < 4.0f / 6.0f)
                return m1 + (m2 - m1) * (2.0f / 3.0f - h) * 6.0f;
            return m1;
        }

        static base::Color nvgHSLA(float h, float s, float l, unsigned char a)
        {
            float m1, m2;
            base::Vector4 col;
            h = base::Frac(h);
            if (h < 0.0f) h += 1.0f;
            s = std::clamp(s, 0.0f, 1.0f);
            l = std::clamp(l, 0.0f, 1.0f);
            m2 = l <= 0.5f ? (l * (1 + s)) : (l + s - l * s);
            m1 = 2 * l - m2;
            col.x = std::clamp(nvg__hue(h + 1.0f / 3.0f, m1, m2), 0.0f, 1.0f);
            col.y = std::clamp(nvg__hue(h, m1, m2), 0.0f, 1.0f);
            col.z = std::clamp(nvg__hue(h - 1.0f / 3.0f, m1, m2), 0.0f, 1.0f);
            col.w = a / 255.0f;
            return base::Color::FromVectorLinear(col);
        }

        static base::Color nvgHSL(float h, float s, float l)
        {
            return nvgHSLA(h, s, l, 255);
        }

        static void DrawColorwheel(base::canvas::GeometryBuilder& vg, float x, float y, float w, float h, float t)
        {
            int i;
            float r0, r1, ax, ay, bx, by, cx, cy, aeps, r;
            float hue = sin(t * 0.12f);
            base::canvas::RenderStyle paint;

            vg.pushTransform();

            /*  vg.beginPath();
            vg.rect(x,y,w,h);
            vg.fillColor(MakeColorLinear(255,0,0,128));
            vg.fill();*/

            cx = x + w*0.5f;
            cy = y + h*0.5f;
            r1 = (w < h ? w : h) * 0.5f - 5.0f;
            r0 = r1 - 20.0f;
            aeps = 0.5f / r1;   // half a pixel arc length in radians (2pi cancels out).

            for (i = 0; i < 6; i++) {
                float a0 = (float)i / 6.0f * PI * 2.0f - aeps;
                float a1 = (float)(i + 1.0f) / 6.0f * PI * 2.0f + aeps;
                vg.beginPath();
                vg.arc(cx, cy, r0, a0, a1, base::canvas::Winding::CW);
                vg.arc(cx, cy, r1, a1, a0, base::canvas::Winding::CCW);
                vg.closePath();
                ax = cx + cos(a0) * (r0 + r1)*0.5f;
                ay = cy + sin(a0) * (r0 + r1)*0.5f;
                bx = cx + cos(a1) * (r0 + r1)*0.5f;
                by = cy + sin(a1) * (r0 + r1)*0.5f;
                paint = base::canvas::LinearGradient(ax, ay, bx, by, nvgHSLA(a0 / (PI * 2), 1.0f, 0.55f, 255), nvgHSLA(a1 / (PI * 2), 1.0f, 0.55f, 255));
                vg.fillPaint(paint);
                vg.fill();
            }

            vg.beginPath();
            vg.circle(cx, cy, r0 - 0.5f);
            vg.circle(cx, cy, r1 + 0.5f);
            vg.strokeColor(MakeColorLinear(0, 0, 0, 64));
            vg.strokeWidth(1.0f);
            vg.stroke();

            // Selector
            vg.pushTransform();
            vg.translate(cx, cy);
            vg.rotate(hue*PI * 2);

            // Marker on
            vg.strokeWidth(2.0f);
            vg.beginPath();
            vg.rect(r0 - 1.0f, -3.0f, r1 - r0 + 2.0f, 6.0f);
            vg.strokeColor(MakeColorLinear(255, 255, 255, 192));
            vg.stroke();

            paint = base::canvas::BoxGradient(r0 - 3.0f, -5.0f, r1 - r0 + 6.0f, 10.0f, 2.0f, 4.0f, MakeColorLinear(0, 0, 0, 128), MakeColorLinear(0, 0, 0, 0));
            vg.beginPath();
            vg.rect(r0 - 2.0f - 10.0f, -4.0f - 10.0f, r1 - r0 + 4.0f + 20.0f, 8.0f + 20.0f);
            vg.rect(r0 - 2.0f, -4.0f, r1 - r0 + 4.0f, 8.0f);
            vg.pathWinding(base::canvas::Winding::CW);
            vg.fillPaint(paint);
            vg.fill();

            // Center triangle
            r = r0 - 6;
            ax = cos(120.0f * DEG2RAD) * r;
            ay = sin(120.0f * DEG2RAD) * r;
            bx = cos(-120.0f * DEG2RAD) * r;
            by = sin(-120.0f * DEG2RAD) * r;
            vg.beginPath();
            vg.moveTo(r, 0.0f);
            vg.lineTo(ax, ay);
            vg.lineTo(bx, by);
            vg.closePath();
            paint = base::canvas::LinearGradient(r, 0.0f, ax, ay, nvgHSLA(hue, 1.0f, 0.5f, 255), MakeColorLinear(255, 255, 255, 255));
            vg.fillPaint(paint);
            vg.fill();
            paint = base::canvas::LinearGradient((r + ax)*0.5f, (0 + ay)*0.5f, bx, by, MakeColorLinear(0, 0, 0, 0), MakeColorLinear(0, 0, 0, 255));
            vg.fillPaint(paint);
            vg.fill();
            vg.strokeColor(MakeColorLinear(0, 0, 0, 64));
            vg.stroke();

            // Select circle on triangle
            ax = cos(120.0f * DEG2RAD) * r*0.3f;
            ay = sin(120.0f * DEG2RAD) * r*0.4f;
            vg.strokeWidth(2.0f);
            vg.beginPath();
            vg.circle(ax, ay, 5.0f);
            vg.strokeColor(MakeColorLinear(255, 255, 255, 192));
            vg.stroke();

            paint = base::canvas::RadialGradient(ax, ay, 7.0f, 9.0f, MakeColorLinear(0, 0, 0, 64), MakeColorLinear(0, 0, 0, 0));
            vg.beginPath();
            vg.rect(ax - 20.0f, ay - 20.0f, 40.0f, 40.0f);
            vg.circle(ax, ay, 7.0f);
            vg.pathWinding(base::canvas::Winding::CW);
            vg.fillPaint(paint);
            vg.fill();

            vg.popTransform();

            vg.popTransform();
        }

        static void DrawLines(base::canvas::GeometryBuilder& vg, float x, float y, float w, float h, float t)
        {
            int i, j;
            float pad = 5.0f, s = w / 9.0f - pad * 2;
            float pts[4 * 2], fx, fy;
            base::canvas::LineJoin joins[3] = { base::canvas::LineJoin::Miter, base::canvas::LineJoin::Round, base::canvas::LineJoin::Bevel };
            base::canvas::LineCap caps[3] = { base::canvas::LineCap::Butt, base::canvas::LineCap::Round, base::canvas::LineCap::Square };
            
            vg.pushTransform();
            pts[0] = -s*0.25f + cos(t*0.3f) * s*0.5f;
            pts[1] = sin(t*0.3f) * s*0.5f;
            pts[2] = -s*0.25f;
            pts[3] = 0;
            pts[4] = s*0.25f;
            pts[5] = 0;
            pts[6] = s*0.25f + cos(-t*0.3f) * s*0.5f;
            pts[7] = sin(-t*0.3f) * s*0.5f;

            for (i = 0; i < 3; i++) {
                for (j = 0; j < 3; j++) {
                    fx = x + s*0.5f + (i * 3 + j) / 9.0f*w + pad;
                    fy = y - s*0.5f + pad;

                    vg.lineCap(caps[i]);
                    vg.lineJoin(joins[0]);

                    vg.strokeWidth(s*0.3f);
                    vg.strokeColor(MakeColorLinear(0, 0, 0, 160));
                    vg.beginPath();
                    vg.moveTo(fx + pts[0], fy + pts[1]);
                    vg.lineTo(fx + pts[2], fy + pts[3]);
                    vg.lineTo(fx + pts[4], fy + pts[5]);
                    vg.lineTo(fx + pts[6], fy + pts[7]);
                    vg.stroke();

                    vg.lineCap(base::canvas::LineCap::Butt);
                    vg.lineJoin(base::canvas::LineJoin::Bevel);

                    vg.strokeWidth(1.0f);
                    vg.strokeColor(MakeColorLinear(0, 192, 255, 255));
                    vg.beginPath();
                    vg.moveTo(fx + pts[0], fy + pts[1]);
                    vg.lineTo(fx + pts[2], fy + pts[3]);
                    vg.lineTo(fx + pts[4], fy + pts[5]);
                    vg.lineTo(fx + pts[6], fy + pts[7]);
                    vg.stroke();
                    
                }
            }

            vg.popTransform();
        }

        static void DrawSearchBox(FontHelper& fh, base::canvas::GeometryBuilder& vg, const char* text, float x, float y, float w, float h)
        {
            base::canvas::RenderStyle bg;
            float cornerRadius = h / 2 - 1;

            // Edit
            bg = base::canvas::BoxGradient(x, y + 1.5f, w, h, h / 2, 5.0f, MakeColorLinear(0, 0, 0, 16), MakeColorLinear(0, 0, 0, 92));
            vg.beginPath();
            vg.roundedRect(x, y, w, h, cornerRadius);
            vg.fillPaint(bg);
            vg.fill();

            /*  vg.beginPath();
            vg.roundedRect(x+0.5f,y+0.5f, w-1,h-1, cornerRadius-0.5f);
            vg.strokeColor(MakeColorLinear(0,0,0,48));
            vg.stroke();*/

            fh.fontSize(h*1.3f);
            fh.fontFace("icons");
            vg.fillColor(MakeColorLinear(255, 255, 255, 64));
            fh.textAlign(NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            fh.text(vg, x + h*0.55f, y + h*0.55f, ICON_SEARCH, NULL);

            fh.fontSize(20.0f);
            fh.fontFace("sans");
            vg.fillColor(MakeColorLinear(255, 255, 255, 32));

            fh.textAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            fh.text(vg, x + h*1.05f, y + h*0.5f, text, NULL);

            fh.fontSize(h*1.3f);
            fh.fontFace("icons");
            vg.fillColor(MakeColorLinear(255, 255, 255, 32));
            fh.textAlign(NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            fh.text(vg, x + w - h*0.55f, y + h*0.55f, ICON_CIRCLED_CROSS, NULL);
        }

        static void DrawDropDown(FontHelper& fh, base::canvas::GeometryBuilder& vg, const char* text, float x, float y, float w, float h)
        {
            base::canvas::RenderStyle bg;
            float cornerRadius = 4.0f;

            bg = base::canvas::LinearGradient(x, y, x, y + h, MakeColorLinear(255, 255, 255, 16), MakeColorLinear(0, 0, 0, 16));
            vg.beginPath();
            vg.roundedRect(x + 1, y + 1, w - 2, h - 2, cornerRadius - 1);
            vg.fillPaint(bg);
            vg.fill();

            vg.beginPath();
            vg.roundedRect(x + 0.5f, y + 0.5f, w - 1, h - 1, cornerRadius - 0.5f);
            vg.strokeColor(MakeColorLinear(0, 0, 0, 48));
            vg.stroke();

            fh.fontSize(20.0f);
            fh.fontFace("sans");
            vg.fillColor(MakeColorLinear(255, 255, 255, 160));
            fh.textAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            fh.text(vg, x + h*0.3f, y + h*0.5f, text, NULL);

            fh.fontSize(h*1.3f);
            fh.fontFace("icons");
            vg.fillColor(MakeColorLinear(255, 255, 255, 64));
            fh.textAlign(NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            fh.text(vg, x + w - h*0.5f, y + h*0.5f, ICON_CHEVRON_RIGHT, NULL);
        }

        static void DrawLabel(FontHelper& fh, base::canvas::GeometryBuilder& vg, const char* text, float x, float y, float w, float h)
        {
            fh.fontSize(18.0f);
            fh.fontFace("sans");
            vg.fillColor(MakeColorLinear(255, 255, 255, 128));

            fh.textAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            fh.text(vg, x, y + h*0.5f, text, NULL);
        }

        static void DrawEditBoxBase(FontHelper& fh, base::canvas::GeometryBuilder& vg, float x, float y, float w, float h)
        {
            base::canvas::RenderStyle bg;
            // Edit
            bg = base::canvas::BoxGradient(x + 1, y + 1 + 1.5f, w - 2, h - 2, 3.0f, 4.0f, MakeColorLinear(255, 255, 255, 32), MakeColorLinear(32, 32, 32, 32));
            vg.beginPath();
            vg.roundedRect(x + 1.0f, y + 1.0f, w - 2.0f, h - 2.0f, 4 - 1.0f);
            vg.fillPaint(bg);
            vg.fill();

            vg.beginPath();
            vg.roundedRect(x + 0.5f, y + 0.5f, w - 1, h - 1, 4 - 0.5f);
            vg.strokeColor(MakeColorLinear(0, 0, 0, 48));
            vg.stroke();
        }

        static void DrawEditBox(FontHelper& fh, base::canvas::GeometryBuilder& vg, const char* text, float x, float y, float w, float h)
        {
            DrawEditBoxBase(fh, vg, x, y, w, h);

            fh.fontSize(20.0f);
            fh.fontFace("sans");
            vg.fillColor(MakeColorLinear(255, 255, 255, 64));
            fh.textAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            fh.text(vg, x + h*0.3f, y + h*0.5f, text, NULL);
        }

        static void DrawEditBoxNum(FontHelper& fh, base::canvas::GeometryBuilder& vg, const char* text, const char* units, float x, float y, float w, float h)
        {
            float uw;
            DrawEditBoxBase(fh, vg, x, y, w, h);

            uw = fh.textBounds(0, 0, units, NULL, NULL);

            fh.fontSize(18.0f);
            fh.fontFace("sans");
            vg.fillColor(MakeColorLinear(255, 255, 255, 64));
            fh.textAlign(NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
            fh.text(vg, x + w - h*0.3f, y + h*0.5f, units, NULL);

            fh.fontSize(20.0f);
            fh.fontFace("sans");
            vg.fillColor(MakeColorLinear(255, 255, 255, 128));
            fh.textAlign(NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
            fh.text(vg, x + w - uw - h*0.5f, y + h*0.5f, text, NULL);
        }

        static void DrawCheckBox(FontHelper& fh, base::canvas::GeometryBuilder& vg, const char* text, float x, float y, float w, float h)
        {
            base::canvas::RenderStyle bg;

            fh.fontSize(18.0f);
            fh.fontFace("sans");
            vg.fillColor(MakeColorLinear(255, 255, 255, 160));

            fh.textAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            fh.text(vg, x + 28, y + h*0.5f, text, NULL);

            bg = base::canvas::BoxGradient(x + 1, y + (int)(h*0.5f) - 9 + 1, 18.0f, 18.0f, 3.0f, 3.0f, MakeColorLinear(0, 0, 0, 32), MakeColorLinear(0, 0, 0, 92));
            vg.beginPath();
            vg.roundedRect(x + 1, y + (int)(h*0.5f) - 9, 18.0f, 18.0f, 3.0f);
            vg.fillPaint(bg);
            vg.fill();

            fh.fontSize(40.0f);
            fh.fontFace("icons");
            vg.fillColor(MakeColorLinear(255, 255, 255, 128));
            fh.textAlign(NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            fh.text(vg, x + 9 + 2, y + h*0.5f, ICON_CHECK, NULL);
        }

        static void DrawButton(FontHelper& fh, base::canvas::GeometryBuilder& vg, const wchar_t* preicon, const char* text, float x, float y, float w, float h, base::Color col)
        {
            base::canvas::RenderStyle bg;
            float cornerRadius = 4.0f;
            float tw = 0, iw = 0;

            bg = base::canvas::LinearGradient(x, y, x, y + h, MakeColorLinear(255, 255, 255, 16), MakeColorLinear(0, 0, 0, 16));
            vg.beginPath();
            vg.roundedRect(x + 1, y + 1, w - 2, h - 2, cornerRadius - 1);
            if (col != base::Color::BLACK) {
                vg.fillColor(col);
                vg.fill();
            }
            vg.fillPaint(bg);
            vg.fill();

            vg.beginPath();
            vg.roundedRect(x + 0.5f, y + 0.5f, w - 1, h - 1, cornerRadius - 0.5f);
            vg.strokeColor(MakeColorLinear(0, 0, 0, 48));
            vg.stroke();

            fh.fontSize(20.0f);
            fh.fontFace("sans-bold");
            tw = fh.textBounds(0, 0, text, NULL, NULL);
            if (preicon != 0) {
                fh.fontSize(h*1.3f);
                fh.fontFace("icons");
                iw = fh.textBounds(0, 0, preicon, NULL, NULL);
                iw += h*0.15f;
            }

            if (preicon != 0) {
                fh.fontSize(h*1.3f);
                fh.fontFace("icons");
                vg.fillColor(MakeColorLinear(255, 255, 255, 96));
                fh.textAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
                fh.text(vg, x + w*0.5f - tw*0.5f - iw*0.75f, y + h*0.5f, preicon, NULL);
            }

            fh.fontSize(20.0f);
            fh.fontFace("sans-bold");
            fh.textAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            vg.fillColor(MakeColorLinear(0, 0, 0, 160));
            fh.text(vg, x + w*0.5f - tw*0.5f + iw*0.25f, y + h*0.5f - 1, text, NULL);
            vg.fillColor(MakeColorLinear(255, 255, 255, 160));
            fh.text(vg, x + w*0.5f - tw*0.5f + iw*0.25f, y + h*0.5f, text, NULL);
        }

        static void DrawSlider(base::canvas::GeometryBuilder& vg, float pos, float x, float y, float w, float h)
        {
            base::canvas::RenderStyle bg, knob;
            float cy = y + (int)(h*0.5f);
            float kr = (float)(int)(h*0.25f);

            vg.pushTransform();
            //  nvgClearState(vg);

            // Slot
            bg = base::canvas::BoxGradient(x, cy - 2 + 1, w, 4.0f, 2.0f, 2.0f, MakeColorLinear(0, 0, 0, 32), MakeColorLinear(0, 0, 0, 128));
            vg.beginPath();
            vg.roundedRect(x, cy - 2, w, 4.0f, 2.0f);
            vg.fillPaint(bg);
            vg.fill();

            // Knob Shadow
            bg = base::canvas::RadialGradient(x + (int)(pos*w), cy + 1, kr - 3, kr + 3, MakeColorLinear(0, 0, 0, 64), MakeColorLinear(0, 0, 0, 0));
            vg.beginPath();
            vg.rect(x + (int)(pos*w) - kr - 5, cy - kr - 5, kr * 2 + 5 + 5, kr * 2 + 5 + 5 + 3);
            vg.circle(x + (int)(pos*w), cy, kr);
            vg.pathWinding(base::canvas::Winding::CW);
            vg.fillPaint(bg);
            vg.fill();

            // Knob
            knob = base::canvas::LinearGradient(x, cy - kr, x, cy + kr, MakeColorLinear(255, 255, 255, 16), MakeColorLinear(0, 0, 0, 16));
            vg.beginPath();
            vg.circle(x + (int)(pos*w), cy, kr - 1);
            vg.fillColor(MakeColorLinear(40, 43, 48, 255));
            vg.fill();
            vg.fillPaint(knob);
            vg.fill();

            vg.beginPath();
            vg.circle(x + (int)(pos*w), cy, kr - 0.5f);
            vg.strokeColor(MakeColorLinear(0, 0, 0, 92));
            vg.stroke();

            vg.popTransform();
        }

        static void DrawSpinner(base::canvas::GeometryBuilder& vg, float cx, float cy, float r, float t)
        {
            float a0 = 0.0f + t * 6;
            float a1 = PI + t * 6;
            float r0 = r;
            float r1 = r * 0.75f;
            float ax, ay, bx, by;
            base::canvas::RenderStyle paint;

            vg.pushTransform();

            vg.beginPath();
            vg.arc(cx, cy, r0, a0, a1, base::canvas::Winding::CW);
            vg.arc(cx, cy, r1, a1, a0, base::canvas::Winding::CCW);
            vg.closePath();
            ax = cx + cos(a0) * (r0 + r1)*0.5f;
            ay = cy + sin(a0) * (r0 + r1)*0.5f;
            bx = cx + cos(a1) * (r0 + r1)*0.5f;
            by = cy + sin(a1) * (r0 + r1)*0.5f;
            paint = base::canvas::LinearGradient(ax, ay, bx, by, MakeColorLinear(0, 0, 0, 0), MakeColorLinear(0, 0, 0, 128));
            //paint = base::canvas::LinearGradient(ax, ay, bx, by, MakeColorLinear(255, 0, 0, 255), MakeColorLinear(0, 255, 0, 255));
            vg.fillPaint(paint);
            vg.fill();

            vg.popTransform();
        }

        static void DrawWidths(base::canvas::GeometryBuilder& vg, float x, float y, float width)
        {
            int i;

            vg.pushTransform();

            vg.strokeColor(MakeColorLinear(0, 0, 0, 255));

            for (i = 0; i < 20; i++) {
                float w = (i + 0.5f)*0.1f;
                vg.strokeWidth(w);
                vg.beginPath();
                vg.moveTo(x, y);
                vg.lineTo(x + width, y + width*0.3f);
                vg.stroke();
                y += 10;
            }

            vg.popTransform();
        }
#pragma warning (disable: 4101)

        static void DrawThumbnailsUnclip(base::canvas::GeometryBuilder& vg, float x, float y, float w, float h, const base::Array<base::image::ImagePtr>& images, float t)
        {
            base::canvas::RenderStyle shadowPaint;
            float cornerRadius = 3.0f;

            vg.pushTransform();

            // Drop shadow
            shadowPaint = base::canvas::BoxGradient(x, y + 4, w, h, cornerRadius * 2, 20, MakeColorLinear(0, 0, 0, 128), MakeColorLinear(0, 0, 0, 0));
            vg.beginPath();
            vg.rect(x - 10, y - 10, w + 20, h + 30);
            vg.roundedRect(x, y, w, h, cornerRadius);
            vg.pathWinding(base::canvas::Winding::CW);
            vg.fillPaint(shadowPaint);
            vg.fill();

            vg.popTransform();
        }

        static void DrawThumbnails(base::canvas::GeometryBuilder& vg, float x, float y, float w, float h, const base::Array<base::image::ImagePtr>& images, float t)
        {
            //t *= 0.05f;

            int nimages = (int)images.size();
            float cornerRadius = 3.0f;
            base::canvas::RenderStyle shadowPaint, imgPaint, fadePaint;
            float ix, iy, iw, ih;
            float thumb = 60.0f;
            float arry = 30.5f;
            int imgw, imgh;
            float stackh = (nimages / 2) * (thumb + 10) + 10;
            int i;
            float u = (1 + cos(t*0.5f))*0.5f;
            float u2 = (1 - cos(t*0.2f))*0.5f;
            float dv;

            vg.pushTransform();
            //  nvgClearState(vg);


            // Window
            vg.beginPath();
            vg.roundedRect(x, y, w, h, cornerRadius);
            vg.moveTo(x - 10, y + arry);
            vg.lineTo(x + 1, y + arry - 11);
            vg.lineTo(x + 1, y + arry + 11);
            vg.fillColor(MakeColorLinear(200, 200, 200, 255));
            vg.fill();

            vg.pushTransform();
            vg.pushState();
            vg.translate(0.0f, -(stackh - h)*u);

            dv = 1.0f / (float)(nimages - 1);

            for (i = 0; i < nimages; i++) {
                float tx, ty, v, a;
                tx = x + 10;
                ty = y + 10;
                tx += (i % 2) * (thumb + 10);
                ty += (i / 2) * (thumb + 10);
                imgw = images[i]->width();
                imgh = images[i]->height();
                if (imgw < imgh) {
                    iw = thumb;
                    ih = iw * (float)imgh / (float)imgw;
                    ix = 0;
                    iy = -(ih - thumb)*0.5f;
                }
                else {
                    ih = thumb;
                    iw = ih * (float)imgw / (float)imgh;
                    ix = -(iw - thumb)*0.5f;
                    iy = 0;
                }

                v = i * dv;
                a = std::clamp<float>((u2 - v) / dv, 0, 1);

                if (a < 1.0f)
                    DrawSpinner(vg, tx + thumb / 2, ty + thumb / 2, thumb*0.25f, t);

                auto imgPaint = base::canvas::ImagePattern(images[i], base::canvas::ImagePatternSettings().alpha(base::FloatTo255(a)).scale(1.5f));// /* 0.0f / 180.0f*PI, images[i], a*/);
                //imgPaint = base::canvas::SolidColor(base::Color::YELLOW);
                vg.pushTransform();
                vg.translate(tx, ty);
                vg.beginPath();
                vg.roundedRect(0, 0, thumb, thumb, 5);
                vg.fillPaint(imgPaint);
                vg.fill();
                vg.popTransform();

                shadowPaint = base::canvas::BoxGradient(tx - 1, ty, thumb + 2, thumb + 2, 5.0f, 3.0f, MakeColorLinear(0, 0, 0, 128), MakeColorLinear(0, 0, 0, 0));
                vg.beginPath();
                vg.rect(tx - 5, ty - 5, thumb + 10, thumb + 10);
                vg.roundedRect(tx, ty, thumb, thumb, 6);
                vg.pathWinding(base::canvas::Winding::CW);
                vg.fillPaint(shadowPaint);
                vg.fill();

                vg.beginPath();
                //vg.roundedRect((int)tx + 0.5f, (int)ty + 0.5f, thumb - 1, thumb - 1, 4 - 0.5f);
                vg.roundedRect((int)tx + 0.5f, (int)ty + 0.5f, thumb - 1, thumb - 1, 4 - 0.5f);
                vg.strokeWidth(1.0f);
                vg.strokeColor(MakeColorLinear(255, 255, 255, 192));
                vg.stroke();
            }
            vg.popState();
            vg.popTransform();

            // Hide fades
            fadePaint = base::canvas::LinearGradient(x, y, x, y + 6, MakeColorLinear(200, 200, 200, 255), MakeColorLinear(200, 200, 200, 0));
            vg.beginPath();
            vg.rect(x + 4, y, w - 8, 6.0f);
            vg.fillPaint(fadePaint);
            vg.fill();

            fadePaint = base::canvas::LinearGradient(x, y + h, x, y + h - 6, MakeColorLinear(200, 200, 200, 255), MakeColorLinear(200, 200, 200, 0));
            vg.beginPath();
            vg.rect(x + 4, y + h - 6, w - 8, 6);
            vg.fillPaint(fadePaint);
            vg.fill();

            // Scroll bar
            shadowPaint = base::canvas::BoxGradient(x + w - 12 + 1.0f, y + 4 + 1.0f, 8.0f, h - 8.0f, 3.0f, 4.0f, MakeColorLinear(0, 0, 0, 32), MakeColorLinear(0, 0, 0, 92));
            vg.beginPath();
            vg.roundedRect(x + w - 12, y + 4, 8, h - 8, 3);
            vg.fillPaint(shadowPaint);
            //  vg.fillColor(MakeColorLinear(255,0,0,128));
            vg.fill();

            float scrollh = (h / stackh) * (h - 8);
            shadowPaint = base::canvas::BoxGradient(x + w - 12 - 1.0f, y + 4 + (h - 8 - scrollh)*u - 1, 8.0f, scrollh, 3.0f, 4.0f, MakeColorLinear(220, 220, 220, 255), MakeColorLinear(128, 128, 128, 255));
            vg.beginPath();
            vg.roundedRect(x + w - 12 + 1, y + 4 + 1 + (h - 8 - scrollh)*u, 8 - 2, scrollh - 2, 2);
            vg.fillPaint(shadowPaint);
            //  vg.fillColor(MakeColorLinear(0,0,0,128));
            vg.fill();
            vg.popTransform();
        }



        void DrawParagraph(base::canvas::GeometryBuilder& vg, FontHelper& fh, float x, float y, float width, float height, float mx, float my)
        {
            NVGtextRow rows[3];
            NVGglyphPosition glyphs[100];
            const char* text = "This is longer chunk of text.\n  \n  Would have used lorem ipsum but she    was busy jumping over the lazy dog with the fox and all the men who came to the aid of the party.";// \U0001F389";
            const char* start;
            const char* end;
            int nrows, i, nglyphs, j, lnum = 0;
            float lineh;
            float caretx, px;
            float bounds[4];
            float a;
            float gx = 0.0f, gy = 0.0f;
            int gutter = 0;

            vg.pushTransform();

            fh.fontSize(18.0f);
            fh.fontFace("sans");
            fh.textAlign(NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
            lineh = fh.lineHeight();
            
            // The text break API can be used to fill a large buffer of rows,
            // or to iterate over the text just few lines (or just one) at a time.
            // The "next" variable of the last returned item tells where to continue.
            start = text;
            end = text + strlen(text);
            while ((nrows = fh.textBreakLines(start, end, width, rows, 3))) {
                for (i = 0; i < nrows; i++) {
                    NVGtextRow* row = &rows[i];
                    int hit = mx > x && mx < (x + width) && my >= y && my < (y + lineh);

                    vg.beginPath();
                    vg.fillColor(MakeColorLinear(255, 255, 255, hit ? 64 : 16));
                    vg.rect(x, y, row->width, lineh);
                    vg.fill();

                    vg.fillColor(MakeColorLinear(255, 255, 255, 255));
                    fh.text2(vg, x, y, row->start, row->end);

                    if (hit) {
                        caretx = (mx < x + row->width / 2) ? x : x + row->width;
                        px = x;
                        nglyphs = fh.textGlyphPositions(x, y, row->start, row->end, glyphs, 100);
                        for (j = 0; j < nglyphs; j++) {
                            float x0 = glyphs[j].x;
                            float x1 = (j + 1 < nglyphs) ? glyphs[j + 1].x : x + row->width;
                            float gx = x0 * 0.3f + x1 * 0.7f;
                            if (mx >= px && mx < gx)
                                caretx = glyphs[j].x;
                            px = gx;
                        }
                        vg.beginPath();
                        vg.fillColor(MakeColorLinear(255, 192, 0, 255));
                        vg.rect(caretx, y, 1, lineh);
                        vg.fill();

                        gutter = lnum + 1;
                        gx = x - 10;
                        gy = y + lineh / 2;
                    }
                    lnum++;
                    y += lineh;
                }
                // Keep going...
                start = rows[nrows - 1].next;
            }

            if (gutter) {
                char txt[16];
                snprintf(txt, sizeof(txt), "%d", gutter);
                fh.fontSize(13.0f);
                fh.textAlign(NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);

                fh.textBoxBounds(gx, gy, 100, txt, NULL, bounds);


                vg.beginPath();
                vg.fillColor(MakeColorLinear(255, 192, 0, 255));
                vg.roundedRect((int)bounds[0] - 9, (int)bounds[1] - 7, (int)(bounds[2] - bounds[0]) + 8, (int)(bounds[3] - bounds[1]) + 4, ((int)(bounds[3] - bounds[1]) + 4) / 2 - 1);
                vg.fill();

                vg.fillColor(MakeColorLinear(32, 32, 32, 255));
                fh.text(vg,gx, gy, txt, NULL);
            }

            y += 20.0f;

            fh.fontSize(13.0f);
            fh.textAlign(NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
            //nvgTextLineHeight(vg, 1.2f);

            fh.textBoxBounds(x, y, 150, "Hover your mouse over the text to see calculated caret position.", NULL, bounds);

            // Fade the tooltip out when close to it.
            gx = fabsf((mx - (bounds[0] + bounds[2])*0.5f) / (bounds[0] - bounds[2]));
            gy = fabsf((my - (bounds[1] + bounds[3])*0.5f) / (bounds[1] - bounds[3]));
            a = std::max<float>(gx, gy) - 0.5f;
            a = std::clamp<float>(a, 0, 1);
            vg.globalAlpha(a);
            //nvgGlobalAlpha(vg, a);

            vg.beginPath();
            vg.fillColor(MakeColorLinear(220, 220, 220, 255));
            vg.roundedRect(bounds[0] - 2, bounds[1] - 2, (int)(bounds[2] - bounds[0]) + 4, (int)(bounds[3] - bounds[1]) + 4, 3);
            px = (int)((bounds[2] + bounds[0]) / 2);
            vg.moveTo(px, bounds[1] - 10);
            vg.lineTo(px + 7, bounds[1] + 1);
            vg.lineTo(px - 7, bounds[1] + 1);
            vg.fill();

            vg.fillColor(MakeColorLinear(0, 0, 0, 220));
            fh.textBox(vg, x, y, 150, "Hover your mouse over the text to see calculated caret position.", NULL);

            vg.popTransform();
        }

        void DrawScissor(base::canvas::GeometryBuilder& vg, float x, float y, float t)
        {
            vg.pushTransform();

            // Draw first rect and set scissor to it's area.
            vg.translate(x, y);
            vg.rotate(5.0f);
            vg.beginPath();
            vg.rect(-20, -20, 60, 40);
            vg.fillColor(MakeColorLinear(255, 0, 0, 255));
            vg.fill();
            //nvgScissor(vg, -20, -20, 60, 40);

            // Draw second rectangle with offset and rotation.
            vg.translate(40, 0);
            vg.rotate(t);

            // Draw the intended second rectangle without any scissoring.
            vg.pushTransform();
            //nvgResetScissor(vg);
            vg.beginPath();
            vg.rect(-20, -10, 60, 30);
            vg.fillColor(MakeColorLinear(255, 128, 0, 64));
            vg.fill();
            vg.popTransform();

            // Draw second rectangle with combined scissoring.
            //nvgIntersectScissor(vg, -20, -10, 60, 30);
            vg.beginPath();
            vg.rect(-20, -10, 60, 30);
            vg.fillColor(MakeColorLinear(255, 128, 0, 255));
            vg.fill();

            vg.popTransform();
        }

        void SceneTest_CanvasEverything::render(base::canvas::Canvas& c)
        {
            static uint32_t frameIndex = 0;
            float t = (frameIndex++ / 60.0f);
            float w = (float)c.width();
            float h = (float)c.height();

            float mx = m_mouseX;
            float my = m_mouseY;

            {
                float popy = 100.0f;

                if (m_staticGeometry.empty())
                {
                    base::canvas::GeometryBuilder b;

                    // Widgets
                    DrawWindow(m_fontHelper, b, "Widgets `n Stuff", 50, 50, 300, 400);
                    float x = 60;
                    float y = 95;
                    DrawSearchBox(m_fontHelper, b, "Search", x, y, 280, 25);
                    y += 40;
                    DrawDropDown(m_fontHelper, b, "Effects", x, y, 280, 28);
                    popy = y + 14;
                    y += 45;

                    // Form
                    DrawLabel(m_fontHelper, b, "Login", x, y, 280, 20);
                    y += 25;
                    DrawEditBox(m_fontHelper, b, "Email", x, y, 280, 28);
                    y += 35;
                    DrawEditBox(m_fontHelper, b, "Password", x, y, 280, 28);
                    y += 38;
                    DrawCheckBox(m_fontHelper, b, "Remember me", x, y, 140, 28);
                    DrawButton(m_fontHelper, b, ICON_LOGIN, "Sign in", x + 138, y, 140, 28, MakeColorLinear(0, 96, 128, 255));
                    y += 45;

                    // Slider
                    DrawLabel(m_fontHelper, b, "Diameter", x, y, 280, 20);
                    y += 25;
                    DrawEditBoxNum(m_fontHelper, b, "123.00", "px", x + 180, y, 100, 28);
                    DrawSlider(b, 0.4f, x, y, 170, 28);
                    y += 55;

                    DrawButton(m_fontHelper, b, ICON_TRASH, "Delete", x, y, 160, 28, MakeColorLinear(128, 16, 8, 255));
                    DrawButton(m_fontHelper, b, 0, "Cancel", x + 170, y, 110, 28, MakeColorLinear(0, 0, 0, 0));

                    DrawWidths(b, 10, 50, 30);

                    b.extract(m_staticGeometry);
                }

                c.place(m_staticGeometry);

                {
                    base::canvas::GeometryBuilder b;
//
                    DrawGraph(b, 0, h / 2, w, h / 2, t);
                    DrawEyes(b, w - 250.0f, 50.0f, 150.0f, 100.0f, mx, my, t);
                    DrawCaps(b, 10, 300, 30);
                    DrawColorwheel(b, w - 300.0f, h - 300.0f, 250.0f, 250.0f, t);
                    DrawLines(b, 120.0f, h - 50.0f, 600, 50, t);

                    c.place(b);
                }

                {
                    base::canvas::GeometryBuilder b;
                    DrawThumbnails(b, 365, popy - 30, 160, 300, m_images, t);
                    base::canvas::GeometryBuilder b2;
                    DrawThumbnailsUnclip(b2, 365, popy - 30, 160, 300, m_images, t);

                    c.place(b2);
                    c.scissorRect(365, popy - 30, 160, 300);
                    c.place(b);
                    c.resetScissorRect();
                }

                {
                    base::canvas::GeometryBuilder b;
                    DrawParagraph(b, m_fontHelper, w - 450, 50, 150, 100, mx, my);
                    c.place(b);
                }
            }
        }

    } // test
} // rendering

namespace rendering
{
    namespace test
    {

        RTTI_BEGIN_TYPE_CLASS(SceneTest_CanvasEverything);
        RTTI_METADATA(CanvasTestOrderMetadata).order(200);
        RTTI_END_TYPE();

    } // test
} // rendering
