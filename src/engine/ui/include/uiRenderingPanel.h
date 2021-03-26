/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: widgets #]
***/

#pragma once

#include "uiElement.h"

namespace boomer
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

    // calculate camera camera
    virtual void handleCamera(CameraSetup& outCamera) const = 0;

	// render the panel, when done submit work to the device
	virtual void handleRender(gpu::CommandWriter& cmd, const gpu::AcquiredOutput& output, const CameraSetup& camera, const FrameParams_Capture* capture = nullptr) = 0;

protected:
    virtual void handleHoverEnter(const Position& pos) override;
    virtual void handleHoverLeave(const Position& pos) override;
    virtual bool handleMouseMovement(const InputMouseMovementEvent& evt) override;
    virtual bool handleKeyEvent(const InputKeyEvent& evt) override;

    bool calculateCurrentPixelUnderCursor(Point& outPixel) const;

    Point m_renderTargetOffset = Point(0, 0);
    int m_renderTargetZoom = 0;

    void renderCaptureScene(const CameraSetup& camera, const FrameParams_Capture* capture = nullptr);

private:
    NativeTimePoint m_lastRenderTime;
    float m_renderRate; // default render rate
    bool m_autoRender; // render every frame
    bool m_renderRequest; // rendering was requested

    bool m_currentHoverAbsolutePositionValid = false;
    Position m_currentHoverAbsolutePosition;

    Point m_lastClickPosition = Point(-1,-1);
    Point m_currentHoverPosition = Point(-1, -1);

    virtual void renderForeground(DataStash& stash, const ElementArea& drawArea, Canvas& canvas, float mergedOpacity) override;

	//--

    gpu::ImageObjectPtr m_colorSurface;
    gpu::ImageObjectPtr m_depthSurface;
    gpu::RenderTargetViewPtr m_colorSurfaceRTV;
    gpu::ImageSampledViewPtr m_colorSurfaceSRV;
    gpu::RenderTargetViewPtr m_depthSurfaceRTV;

	CanvasGeometry* m_quadGeometry;

	bool parepareRenderTargets(Point requiredSize, gpu::AcquiredOutput& outOutput);

	//--
};

///---

END_BOOMER_NAMESPACE_EX(ui)
