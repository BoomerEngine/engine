/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: widgets #]
***/

#pragma once

#include "base/ui/include/uiElement.h"

namespace rendering
{
	namespace scene
	{
		struct FrameParams_Capture;
	}
}

namespace ui
{
    //--

    /// ui widget capable of hosting any command buffer based rendering (3D/2D etc), even full scene
    class RENDERING_UI_HOST_API RenderingPanel : public IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(RenderingPanel, IElement);

    public:
        RenderingPanel();
        virtual ~RenderingPanel();

        //--

        // get auto render flag
        INLINE bool autoRender() const { return m_autoRender; }

        // get rendering rate
        INLINE float renderRate() const { return m_renderRate; }

        //--

        // request redraw of the panel
        void requestRedraw();

        // toggle automatic rendering of the panel
        void autoRender(bool flag);

        // set automatic rendering frequency
        void renderRate(float rate);

        //--

		struct ViewportParams
		{
			uint32_t width = 0;
			uint32_t height = 0;

			const rendering::GraphicsPassLayoutObject* passLayout = nullptr;
			const rendering::RenderTargetView* colorBuffer = nullptr;
			const rendering::RenderTargetView* depthBuffer = nullptr;
			const rendering::scene::FrameParams_Capture* capture = nullptr;
		};

		// render the panel, when done submit work to the device
		virtual void renderContent(const ViewportParams& params);

    protected:
        virtual void handleHoverEnter(const Position& pos) override;
        virtual void handleHoverLeave(const Position& pos) override;
        virtual bool handleMouseMovement(const base::input::MouseMovementEvent& evt) override;
        virtual bool handleKeyEvent(const base::input::KeyEvent& evt) override;

        bool calculateCurrentPixelUnderCursor(base::Point& outPixel) const;

    private:
        base::NativeTimePoint m_lastRenderTime;
        float m_renderRate; // default render rate
        bool m_autoRender; // render every frame
        bool m_renderRequest; // rendering was requested

        bool m_currentHoverAbsolutePositionValid = false;
        ui::Position m_currentHoverAbsolutePosition;

        base::Point m_lastClickPosition = base::Point(-1,-1);
        base::Point m_currentHoverPosition = base::Point(-1, -1);

        base::Point m_renderTargetOffset = base::Point(0, 0);
        int m_renderTargetZoom = 0;

        virtual void renderForeground(const ui::ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity) override;

		//--

		rendering::ImageObjectPtr m_colorSurface;
		rendering::ImageObjectPtr m_depthSurface;
		rendering::RenderTargetViewPtr m_colorSurfaceRTV;
		rendering::ImageSampledViewPtr m_colorSurfaceSRV;
		rendering::RenderTargetViewPtr m_depthSurfaceRTV;
		rendering::GraphicsPassLayoutObjectPtr m_passLayout;

		base::canvas::Geometry* m_quadGeometry;

		void parepareRenderTargets(ViewportParams& viewport);

		//--
    };

    ///---

} // ui