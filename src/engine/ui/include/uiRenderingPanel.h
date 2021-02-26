/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: widgets #]
***/

#pragma once

#include "uiElement.h"

namespace boomer::rendering
{
    struct FrameParams_Capture;
}

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

/// ui widget capable of hosting any command buffer based rendering (3D/2D etc), even full scene
class ENGINE_UI_API RenderingPanel : public IElement
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

		const gpu::RenderTargetView* colorBuffer = nullptr;
		const gpu::RenderTargetView* depthBuffer = nullptr;
		const rendering::FrameParams_Capture* capture = nullptr;
	};

	// render the panel, when done submit work to the device
	virtual void renderContent(const ViewportParams& params, Camera* outCameraUsedToRender = nullptr);

protected:
    virtual void handleHoverEnter(const Position& pos) override;
    virtual void handleHoverLeave(const Position& pos) override;
    virtual bool handleMouseMovement(const input::MouseMovementEvent& evt) override;
    virtual bool handleKeyEvent(const input::KeyEvent& evt) override;

    bool calculateCurrentPixelUnderCursor(Point& outPixel) const;

    Point m_renderTargetOffset = Point(0, 0);
    int m_renderTargetZoom = 0;

    void renderCaptureScene(const rendering::FrameParams_Capture* capture, Camera* outCameraUsedToRender = nullptr);

private:
    NativeTimePoint m_lastRenderTime;
    float m_renderRate; // default render rate
    bool m_autoRender; // render every frame
    bool m_renderRequest; // rendering was requested

    bool m_currentHoverAbsolutePositionValid = false;
    Position m_currentHoverAbsolutePosition;

    Point m_lastClickPosition = Point(-1,-1);
    Point m_currentHoverPosition = Point(-1, -1);

    virtual void renderForeground(DataStash& stash, const ElementArea& drawArea, canvas::Canvas& canvas, float mergedOpacity) override;

	//--

    gpu::ImageObjectPtr m_colorSurface;
    gpu::ImageObjectPtr m_depthSurface;
    gpu::RenderTargetViewPtr m_colorSurfaceRTV;
    gpu::ImageSampledViewPtr m_colorSurfaceSRV;
    gpu::RenderTargetViewPtr m_depthSurfaceRTV;

	canvas::Geometry* m_quadGeometry;

	void parepareRenderTargets(ViewportParams& viewport);

	//--
};

///---

END_BOOMER_NAMESPACE_EX(ui)
