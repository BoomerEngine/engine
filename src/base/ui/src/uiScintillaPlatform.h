/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scintilla\platform #]
*
***/

#pragma once

#include "scintilla/Platform.h"

#include "uiElement.h"

#include "base/canvas/include/canvasGeometry.h"
#include "base/canvas/include/canvasGeometryBuilder.h"
#include "base/canvas/include/canvas.h"
#include "base/font/include/font.h"

namespace Scintilla
{
    //---

    // A wrapper for our canvas
    class SurfaceImpl : public Surface
    {
    public:
        SurfaceImpl();
        ~SurfaceImpl() override;

        //--

        // clear existing content
        void clear();

        // render into canvas collector
        void render(int x, int y, base::canvas::Canvas& canvas);

        //--

        virtual void Init(WindowID wid) override;
        virtual void Init(SurfaceID sid, WindowID wid) override;
        virtual void InitPixMap(int width, int height, Surface *surface_, WindowID wid) override;

        virtual void Release() override;
        virtual bool Initialised() override;
        virtual void PenColour(ColourDesired fore) override;

        virtual int LogPixelsY() override;
        virtual int DeviceHeightFont(int points) override;
        virtual void MoveTo(int x_, int y_) override;
        virtual void LineTo(int x_, int y_) override;
        virtual void Polygon(Scintilla::Point *pts, size_t npts, ColourDesired fore, ColourDesired back) override;
        virtual void RectangleDraw(PRectangle rc, ColourDesired fore, ColourDesired back) override;
        virtual void FillRectangle(PRectangle rc, ColourDesired back) override;
        virtual void FillRectangle(PRectangle rc, Surface &surfacePattern) override;
        virtual void RoundedRectangle(PRectangle rc, ColourDesired fore, ColourDesired back) override;
        virtual void AlphaRectangle(PRectangle rc, int cornerSize, ColourDesired fill, int alphaFill, ColourDesired outline, int alphaOutline, int flags) override;
        virtual void GradientRectangle(PRectangle rc, const std::vector<ColourStop> &stops, GradientOptions options) override;
        virtual void DrawRGBAImage(PRectangle rc, int width, int height, const unsigned char *pixelsImage) override;
        virtual void Ellipse(PRectangle rc, ColourDesired fore, ColourDesired back) override;
        virtual void Copy(PRectangle rc, Scintilla::Point from, Surface &surfaceSource) override;
        virtual std::unique_ptr<IScreenLineLayout> Layout(const IScreenLine *screenLine) override;
        virtual void DrawTextNoClip(PRectangle rc, Font &font_, XYPOSITION ybase, std::string_view text, ColourDesired fore, ColourDesired back) override;
        virtual void DrawTextClipped(PRectangle rc, Font &font_, XYPOSITION ybase, std::string_view text, ColourDesired fore, ColourDesired back) override;
        virtual void DrawTextTransparent(PRectangle rc, Font &font_, XYPOSITION ybase, std::string_view text, ColourDesired fore) override;
        virtual void MeasureWidths(Font &font_, std::string_view text, XYPOSITION *positions) override;
        virtual XYPOSITION WidthText(Font &font_, std::string_view text) override;
        virtual XYPOSITION Ascent(Font &font_) override;
        virtual XYPOSITION Descent(Font &font_) override;
        virtual XYPOSITION InternalLeading(Font &font_) override;
        virtual XYPOSITION Height(Font &font_) override;
        virtual XYPOSITION AverageCharWidth(Font &font_) override;

        virtual void SetClip(PRectangle rc) override;
        virtual void FlushCachedState() override;

        virtual void SetUnicodeMode(bool unicodeMode_) override;
        virtual void SetDBCSMode(int codePage_) override;
        virtual void SetBidiR2L(bool bidiR2L_) override;

    private:
        bool unicodeMode;
        bool initialized;
        bool collected;
        float x;
        float y;

        int codePage;
        int verticalDeviceResolution;

		struct Buffer
		{
			base::canvas::Geometry data;
			base::canvas::GeometryBuilder builder;

			Buffer();

			void reset();
		};

		Buffer* m_displayBuffer = nullptr;
		Buffer* m_buildBuffer = nullptr;

        void ResetStyles();
        void FillColour(const ColourDesired &back);
    };

    //--

    struct FontInfo : public base::NoCopy
    {
        RTTI_DECLARE_POOL(POOL_FONTS)

    public:
        base::font::FontStyleParams m_style;
        base::font::FontAssemblyParams m_assembly;
        base::FontPtr m_font;
    };

    class FontCache : public base::ISingleton
    {
        DECLARE_SINGLETON(FontCache);

    public:
        FontCache();

        // get font from cache
        const FontInfo* cacheFont(const FontParameters& fp);

    private:
        virtual void deinit() override;

        uint64_t calcFontParamsHash(const FontParameters& fp);
        FontInfo* createFont(const FontParameters& fp);

        base::HashMap<uint64_t, const FontInfo*> m_fonts;
        base::SpinLock m_lock;
    };

    //--

    static INLINE base::Rect ToRect(const PRectangle &rc)
    {
        return base::Rect(rc.left, rc.top, rc.right, rc.bottom);
    }

    static INLINE PRectangle FromRect(const base::Rect &rc)
    {
        return PRectangle(static_cast<XYPOSITION>(rc.min.x), static_cast<XYPOSITION>(rc.min.y),
                          static_cast<XYPOSITION>(rc.max.x), static_cast<XYPOSITION>(rc.max.y));
    }

    static INLINE PRectangle FromRect(const ui::ElementArea& rc)
    {
        return PRectangle(rc.absolutePosition().x, rc.absolutePosition().y,
            rc.absolutePosition().x + rc.size().x, rc.absolutePosition().y + rc.size().y);
    }

    static INLINE base::Color ToColor(const ColourDesired& color, int alpha = 255)
    {
        return base::Color(color.GetRed(), color.GetGreen(), color.GetBlue(), alpha);
    }

    //--

    class IWindowInterface : public base::NoCopy
    {
    public:
        virtual ~IWindowInterface();

        virtual PRectangle GetPosition() const = 0;
        virtual void SetPosition(PRectangle rc) = 0;
        virtual PRectangle GetClientPosition() const = 0;
    };

    //--

} // scintilla
