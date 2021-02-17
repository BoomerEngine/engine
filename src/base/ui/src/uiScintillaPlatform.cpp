/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scintilla\platform #]
*
***/

#include "build.h"
#include "uiScintillaPlatform.h"

#include <cstddef>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cstdio>

#include <stdexcept>
#include <string_view>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <numeric>
//#include <sys/time.h>

#include "scintilla/Platform.h"
#include "scintilla/StringCopy.h"
#include "scintilla/XPM.h"
#include "scintilla/UniConversion.h"
#include "scintilla/Scintilla.h"

#include "base/font/include/font.h"
#include "base/font/include/fontGlyph.h"
#include "base/font/include/fontInputText.h"
#include "base/font/include/fontGlyphBuffer.h"
#include "base/canvas/include/canvas.h"
#include "base/canvas/include/canvasGeometry.h"

namespace Scintilla
{

    //--

    FontCache::FontCache()
    {
        m_fonts.reserve(16);
    }

    const FontInfo* FontCache::cacheFont(const FontParameters& fp)
    {
        auto hash = calcFontParamsHash(fp);

        auto lock = base::CreateLock(m_lock);

        const FontInfo* ret = nullptr;
        if (m_fonts.find(hash, ret))
            return ret;

        ret = createFont(fp);
        m_fonts[hash] = ret;
        return ret;
    }

    void FontCache::deinit()
    {
        m_fonts.clearPtr();
    }

    uint64_t FontCache::calcFontParamsHash(const FontParameters& fp)
    {
        base::CRC64 crc;
        crc << fp.faceName;
        crc << fp.size;
        //crc << fp.characterSet;
        //crc << fp.extraFontFlag;
        crc << fp.italic;
        //crc << fp.technology;
        crc << fp.weight;
        return crc.crc();
    }

    static base::RefPtr<base::font::Font> SelectFont(bool bold, bool italic)
    {
        static const auto resFontNormal = base::LoadFontFromDepotPath("/engine/interface/fonts/DejaVuSansMono.ttf");
        static const auto resFontBold = base::LoadFontFromDepotPath("/engine/interface/fonts/DejaVuSansMono-Bold.ttf");
        static const auto resFontItalic = base::LoadFontFromDepotPath("/engine/interface/fonts/DejaVuSansMono-Oblique.ttf");
        static const auto resFontBoldItalic = base::LoadFontFromDepotPath("/engine/interface/fonts/DejaVuSansMono-BoldOblique.ttf");

        if (bold && italic)
            return resFontBoldItalic;
        else if (bold)
            return resFontBold;
        else if (italic)
            return resFontItalic;
        else
            return resFontNormal;
    }

    FontInfo* FontCache::createFont(const FontParameters& fp)
    {
        auto ret = new FontInfo;
        ret->m_font = SelectFont(fp.weight >= 500, fp.italic);
        ret->m_style.size = (uint32_t)fp.size;
        ret->m_style.bold = false;
        ret->m_style.italic = false;
        return ret;
    }

    //--


    Font::Font() noexcept
        : fid(0)
    {
    }

    Font::~Font()
    {
    }

    void Font::Create(const FontParameters &fp)
    {
        fid = (FontID)FontCache::GetInstance().cacheFont(fp);
    }

    void Font::Release()
    {
        // not freed
    }

    static const FontInfo* TextStyleFromFont(const Font &f)
    {
        return static_cast<const FontInfo*>(f.GetID());
    }

	SurfaceImpl::Buffer::Buffer()
		: builder(data)
	{}

	void SurfaceImpl::Buffer::reset()
	{
		data.reset();
		builder.reset();		
	}

    //----------------- SurfaceImpl --------------------------------------------------------------------

    SurfaceImpl::SurfaceImpl()
        : initialized(false)
        , unicodeMode(true)
        , collected(true)
        , codePage(0)
        , verticalDeviceResolution(0)
        , x(0)
        , y(0)
	{
		m_displayBuffer = new Buffer();
		m_buildBuffer = new Buffer();

        Release();
    }

    SurfaceImpl::~SurfaceImpl()
    {
		delete m_displayBuffer;
		delete m_buildBuffer;
    }

    void SurfaceImpl::clear()
    {
		m_buildBuffer->reset();
        collected = true;
        x = 0;
        y = 0;
    }

    void SurfaceImpl::Release()
    {
    }

    bool SurfaceImpl::Initialised()
    {
        return initialized;
    }

    void SurfaceImpl::render(int x, int y, base::canvas::Canvas& canvas)
    {
		if (!collected)
		{
			std::swap(m_buildBuffer, m_displayBuffer);
			m_buildBuffer->reset();
			collected = true;
		}

		canvas.place(base::canvas::Placement(x, y), m_displayBuffer->data);
    }

    void SurfaceImpl::Init(WindowID)
    {
        initialized = true;
        Release();
    }

    void SurfaceImpl::Init(SurfaceID sid, WindowID)
    {
        Release();
    }

    void SurfaceImpl::InitPixMap(int width, int height, Surface *surface_, WindowID /* wid */)
    {
        Release();

        initialized = true;
        if (surface_)
        {
            SurfaceImpl *psurfOther = static_cast<SurfaceImpl *>(surface_);
            unicodeMode = psurfOther->unicodeMode;
            codePage = psurfOther->codePage;
        }
        else
        {
            unicodeMode = true;
            codePage = SC_CP_UTF8;
        }
    }

    void SurfaceImpl::PenColour(ColourDesired fore)
    {
		m_buildBuffer->builder.strokeColor(base::Color(fore.GetRed(), fore.GetGreen(), fore.GetBlue()));
    }

    void SurfaceImpl::FillColour(const ColourDesired &back)
    {
        m_buildBuffer->builder.fillColor(base::Color(back.GetRed(), back.GetGreen(), back.GetBlue()));
    }

    int SurfaceImpl::LogPixelsY()
    {
        return 72;
    }

    int SurfaceImpl::DeviceHeightFont(int points)
    {
        return points;
    }

    void SurfaceImpl::MoveTo(int x_, int y_)
    {
        x = x_;
        y = y_;
    }

    void SurfaceImpl::LineTo(int x_, int y_)
    {
        m_buildBuffer->builder.beginPath();
        m_buildBuffer->builder.moveTo(x, y);
        m_buildBuffer->builder.lineTo(x_, y_);
        m_buildBuffer->builder.stroke();

        collected = false;
        x = x_;
        y = y_;
    }

    void SurfaceImpl::Polygon(Scintilla::Point *pts, size_t npts, ColourDesired fore, ColourDesired back)
    {
        // set colors
        m_buildBuffer->builder.strokeColor(base::Color(fore.GetRed(), fore.GetGreen(), fore.GetBlue()));
        m_buildBuffer->builder.fillColor(base::Color(back.GetRed(), back.GetGreen(), back.GetBlue()));

        // draw path
        {
            m_buildBuffer->builder.beginPath();

            for (size_t i = 0; i < npts; i++)
            {
                if (i == 0)
                    m_buildBuffer->builder.moveTo(pts[i].x, pts[i].y);
                else
                    m_buildBuffer->builder.lineTo(pts[i].x, pts[i].y);
            }

            m_buildBuffer->builder.closePath();
        }


        // draw
        m_buildBuffer->builder.fill();
        m_buildBuffer->builder.stroke();

        // require collection
        collected = false;
    }

    void SurfaceImpl::RectangleDraw(PRectangle rc, ColourDesired fore, ColourDesired back)
    {
        m_buildBuffer->builder.strokeColor(base::Color(fore.GetRed(), fore.GetGreen(), fore.GetBlue()));
        m_buildBuffer->builder.fillColor(base::Color(back.GetRed(), back.GetGreen(), back.GetBlue()));

        m_buildBuffer->builder.beginPath();
        m_buildBuffer->builder.rect(ToRect(rc));

        m_buildBuffer->builder.fill();
        m_buildBuffer->builder.stroke();

        // require collection
        collected = false;
    }

    void SurfaceImpl::FillRectangle(PRectangle rc, ColourDesired back)
    {
        m_buildBuffer->builder.fillColor(base::Color(back.GetRed(), back.GetGreen(), back.GetBlue()));
        m_buildBuffer->builder.beginPath();
        m_buildBuffer->builder.rect(ToRect(rc));
        m_buildBuffer->builder.fill();

        // require collection
        collected = false;
    }

    void SurfaceImpl::FillRectangle(PRectangle rc, Surface &surfacePattern)
    {
        SurfaceImpl &patternSurface = static_cast<SurfaceImpl &>(surfacePattern);

        DEBUG_CHECK(!"TODO");
    }

    void SurfaceImpl::RoundedRectangle(PRectangle rc, ColourDesired fore, ColourDesired back)
    {
        m_buildBuffer->builder.strokeColor(base::Color(fore.GetRed(), fore.GetGreen(), fore.GetBlue()));
        m_buildBuffer->builder.fillColor(base::Color(back.GetRed(), back.GetGreen(), back.GetBlue()));

        m_buildBuffer->builder.beginPath();
        m_buildBuffer->builder.roundedRect(ToRect(rc), 4.0f);

        m_buildBuffer->builder.fill();
        m_buildBuffer->builder.stroke();

        // require collection
        collected = false;
    }

    void Scintilla::SurfaceImpl::AlphaRectangle(PRectangle rc, int cornerSize, ColourDesired fill, int alphaFill, ColourDesired outline, int alphaOutline, int /*flags*/)
    {
        // set colors
        m_buildBuffer->builder.strokeColor(base::Color(outline.GetRed(), outline.GetGreen(), outline.GetBlue(), alphaOutline));
        m_buildBuffer->builder.fillColor(base::Color(fill.GetRed(), fill.GetGreen(), fill.GetBlue(), alphaFill));

        // Set the Fill color to match
        PRectangle rcFill = rc;
        if (cornerSize == 0)
        {
            // A simple rectangle, no rounded corners
            if ((fill == outline) && (alphaFill == alphaOutline))
            {
                m_buildBuffer->builder.beginPath();
                m_buildBuffer->builder.rect(ToRect(rcFill));
                m_buildBuffer->builder.fill();
            }
            else
            {
                rcFill.left += 1.0;
                rcFill.top += 1.0;
                rcFill.right -= 1.0;
                rcFill.bottom -= 1.0;

                m_buildBuffer->builder.beginPath();
                m_buildBuffer->builder.rect(ToRect(rcFill));
                m_buildBuffer->builder.fill();

                m_buildBuffer->builder.beginPath();
                m_buildBuffer->builder.rect(ToRect(rc));
                m_buildBuffer->builder.stroke();
            }
        }
        else
        {
            if ((fill == outline) && (alphaFill == alphaOutline))
            {
                m_buildBuffer->builder.beginPath();
                m_buildBuffer->builder.roundedRect(ToRect(rcFill), (float)cornerSize);
                m_buildBuffer->builder.fill();
            }
            else
            {
                m_buildBuffer->builder.beginPath();
                m_buildBuffer->builder.roundedRect(ToRect(rcFill), (float)cornerSize-1.0f);
                m_buildBuffer->builder.fill();

                m_buildBuffer->builder.beginPath();
                m_buildBuffer->builder.roundedRect(ToRect(rc), (float)cornerSize);
                m_buildBuffer->builder.stroke();
            }
        }

        // require collection
        collected = false;
    }

    void Scintilla::SurfaceImpl::GradientRectangle(PRectangle rc, const std::vector<ColourStop> &stops, GradientOptions options)
    {
        // TODO
    }

    void SurfaceImpl::DrawRGBAImage(PRectangle rc, int width, int height, const unsigned char *pixelsImage)
    {
        // TODO
    }

    void SurfaceImpl::Ellipse(PRectangle rc, ColourDesired fore, ColourDesired back)
    {
        m_buildBuffer->builder.strokeColor(base::Color(fore.GetRed(), fore.GetGreen(), fore.GetBlue()));
        m_buildBuffer->builder.fillColor(base::Color(back.GetRed(), back.GetGreen(), back.GetBlue()));

        m_buildBuffer->builder.beginPath();
        m_buildBuffer->builder.ellipse(ToRect(rc));

        m_buildBuffer->builder.fill();
        m_buildBuffer->builder.stroke();

        // require collection
        collected = false;
    }

    void SurfaceImpl::Copy(PRectangle rc, Scintilla::Point from, Surface &surfaceSource)
    {
        // TODO
    }

    std::unique_ptr<IScreenLineLayout> SurfaceImpl::Layout(const IScreenLine *screenLine)
    {
        return nullptr;
    }

    void SurfaceImpl::DrawTextNoClip(PRectangle rc, Font &font_, XYPOSITION ybase, std::string_view text, ColourDesired fore, ColourDesired back)
    {
        FillRectangle(rc, back);
        DrawTextTransparent(rc, font_, ybase, text, fore);
    }

    void SurfaceImpl::DrawTextClipped(PRectangle rc, Font &font_, XYPOSITION ybase, std::string_view text, ColourDesired fore, ColourDesired back)
    {
        DrawTextNoClip(rc, font_, ybase, text, fore, back);
    }

    void SurfaceImpl::DrawTextTransparent(PRectangle rc, Font &font_, XYPOSITION ybase, std::string_view text, ColourDesired fore)
    {
        const auto *fontInfo = TextStyleFromFont(font_);
        if (nullptr != fontInfo)
        {
            // prepare the text
            auto inputText = base::font::FontInputText(text.data(), text.length());
            base::font::GlyphBuffer glyphs;
            fontInfo->m_font->renderText(fontInfo->m_style, fontInfo->m_assembly, inputText, glyphs);

            // vertical offset
            auto xOffset = floor(rc.left);
            auto yOffest = floor(ybase - Ascent(font_));

            // render
            m_buildBuffer->builder.fillColor(base::Color(fore.GetRed(), fore.GetGreen(), fore.GetBlue()));
            m_buildBuffer->builder.translate(xOffset, yOffest);
            m_buildBuffer->builder.print(glyphs);
            m_buildBuffer->builder.translate(-xOffset, -yOffest);

            // require collection
            collected = false;
        }
    }

    void SurfaceImpl::MeasureWidths(Font &font_, std::string_view text, XYPOSITION *positions)
    {
        const auto *fontInfo = TextStyleFromFont(font_);
        if (nullptr != fontInfo)
        {
            // measure the text
            auto inputText = base::font::FontInputText(text.data(), text.length());
            base::font::GlyphBuffer glyphs;
            fontInfo->m_font->renderText(fontInfo->m_style, fontInfo->m_assembly, inputText, glyphs);

            int lastTextPosition = 0;
            float lastGlyphPos = 0;
            for (uint32_t i=0; i<glyphs.size(); ++i)
            {
                const auto& g = glyphs.glyphs()[i];

                if (g.glyph->size().x > 0.0f)
                    lastGlyphPos = g.pos.x + g.glyph->size().x;
                else
                    lastGlyphPos = g.pos.x + g.glyph->advance().x;

                for (int j=lastTextPosition; j<=g.textPosition; ++j)
                    positions[j] = lastGlyphPos;
                lastTextPosition = g.textPosition + 1;
            }

            // fill till the end
            while (lastTextPosition < text.length())
                positions[lastTextPosition++] = lastGlyphPos;
        }
    }

    XYPOSITION SurfaceImpl::WidthText(Font &font_, std::string_view text)
    {
        const auto *fontInfo = TextStyleFromFont(font_);
        if (nullptr != fontInfo)
        {

            // measure the text
            base::font::FontMetrics metrics;
            auto inputText = base::font::FontInputText(text.data(), text.length());
            fontInfo->m_font->measureText(fontInfo->m_style, fontInfo->m_assembly, inputText, metrics);

            return metrics.textWidth;
        }

        return 1;
    }

    XYPOSITION SurfaceImpl::Ascent(Font &font_)
    {
        const auto *fontInfo = TextStyleFromFont(font_);
        if (nullptr != fontInfo)
            return fontInfo->m_font->relativeAscender() * fontInfo->m_style.size;
            //return (1.0f - fontInfo->m_font->relativeDescender()) * fontInfo->style.size;

        return 1.0f;
    }

    XYPOSITION SurfaceImpl::Descent(Font &font_)
    {
        const auto *fontInfo = TextStyleFromFont(font_);
        if (nullptr != fontInfo)
            //return (1.0f - fontInfo->m_font->relativeAscender()) * fontInfo->style.size;
            return -fontInfo->m_font->relativeDescender() * fontInfo->m_style.size;

        return 1.0f;
    }

    XYPOSITION SurfaceImpl::InternalLeading(Font &)
    {
        return 0;
    }

    XYPOSITION SurfaceImpl::Height(Font &font_)
    {
        const auto *fontInfo = TextStyleFromFont(font_);
        if (nullptr != fontInfo)
            return fontInfo->m_style.size;

        return 1.0f;
    }

    XYPOSITION SurfaceImpl::AverageCharWidth(Font &font_)
    {
        if (!font_.GetID())
            return 1;

        const char sizeString[] = "`~!@#$%^&*()-_=+\\|[]{};:\"\'<,>.?/1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

        XYPOSITION width = WidthText(font_, sizeString);
        return round(width / strlen(sizeString));
    }

    void SurfaceImpl::SetClip(PRectangle rc)
    {
    //    CGContextClipToRect(gc, PRectangleToCGRect(rc));
    }

    void SurfaceImpl::FlushCachedState()
    {
        //CGContextSynchronize(gc);
    }

    void SurfaceImpl::SetUnicodeMode(bool unicodeMode_)
    {
        unicodeMode = unicodeMode_;
    }

    void SurfaceImpl::SetDBCSMode(int codePage_)
    {
        if (codePage_ && (codePage_ != SC_CP_UTF8))
            codePage = codePage_;
    }

    void SurfaceImpl::SetBidiR2L(bool)
    {
    }

    Surface *Surface::Allocate(int)
    {
        return new SurfaceImpl();
    }

    //----------------- Window -------------------------------------------------------------------------

    IWindowInterface::~IWindowInterface()
    {}

    Window::~Window()
    {
    }

    PRectangle Window::GetPosition() const
    {
        auto* window = (IWindowInterface*)GetID();
        return window ? window->GetPosition() : PRectangle(0, 0, 1, 1);
    }

    void Window::SetPosition(PRectangle rc)
    {
        auto* window = (IWindowInterface*)GetID();
        if (window)
            window->SetPosition(rc);
    }

    void Window::SetPositionRelative(PRectangle rc, const Window *window)
    {
        PRectangle rcOther = window->GetPosition();
        rc.left += rcOther.left;
        rc.right += rcOther.left;
        rc.top += rcOther.top;
        rc.bottom += rcOther.top;
        SetPosition(rc);
    }

    PRectangle Window::GetClientPosition() const
    {
        auto* window = (IWindowInterface*)GetID();
        return window ? window->GetClientPosition() : PRectangle(0, 0, 1, 1);
    }

    void Window::Show(bool show)
    {
    }

    void Window::InvalidateAll()
    {
    }

    void Window::InvalidateRectangle(PRectangle rc)
    {
        InvalidateAll();
    }

    void Window::SetFont(Font &)
    {
    }

    void Window::SetCursor(Cursor curs)
    {
    }

    void Window::Destroy()
    {
    }

    PRectangle Window::GetMonitorRect(Point)
    {
        return PRectangle();
    }

    //----------------- ListBox ------------------------------------------------------------------------

    ListBox::ListBox() noexcept
    {
    }

    ListBox::~ListBox()
    {
    }

    ListBox *ListBox::Allocate()
    {
        return nullptr;
    }

    //--------------------------------------------------------------------------------------------------

    Menu::Menu() noexcept
        : mid(0)
    {
    }

    void Menu::CreatePopUp()
    {
        Destroy();
    }

    void Menu::Destroy()
    {
        mid = nullptr;
    }

    void Menu::Show(Point, Window &)
    {
    }

    //----------------- Platform -----------------------------------------------------------------------

    ColourDesired Platform::Chrome()
    {
        return ColourDesired(0xE0, 0xE0, 0xE0);
    }

    ColourDesired Platform::ChromeHighlight()
    {
        return ColourDesired(0xFF, 0xFF, 0xFF);
    }

    const char *Platform::DefaultFont()
    {
        return "dejavu";
    }

    int Platform::DefaultFontSize()
    {
        return 18;
    }

    void Platform::DebugDisplay(const char *s)
    {
        TRACE_ERROR("{}", s);
    }

    void Platform::DebugPrintf(const char *format, ...)
    {
	    static const int BUF_SIZE = 2000;
	    char buffer[BUF_SIZE];

	    va_list pArguments;
	    va_start(pArguments, format);
	    vsnprintf(buffer, BUF_SIZE, format, pArguments);
	    va_end(pArguments);
	    DebugDisplay(buffer);
    }

    unsigned int Platform::DoubleClickTime()
    {
        return 500;
    }

    bool Platform::ShowAssertionPopUps(bool assertionPopUps_)
    {
        return false;
    }

    void Platform::Assert(const char *c, const char *file, int line)
    {
        TRACE_ERROR("Assertion [%s] failed at %s {}", c, file, line);
    }

    //----------------- DynamicLibrary -----------------------------------------------------------------

    DynamicLibrary *DynamicLibrary::Load(const char * /* modulePath */)
    {
        return nullptr;
    }

} // scintilla
