/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: widgets #]
***/

#pragma once

#include "base/ui/include/uiElement.h"
#include "rendering/driver/include/renderingDeviceObject.h"
#include "rendering/driver/include/renderingImageView.h"

namespace ui
{
    //--

    /// ui widget capable of hosting any command buffer based rendering (3D/2D etc), even full scene
    class RENDERING_UI_API RenderingPanel : public IElement, public rendering::IDeviceObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(RenderingPanel, IElement);

    public:
        RenderingPanel();
        virtual ~RenderingPanel();

        //--

        // get auto render flag
        inline bool autoRender() const { return m_autoRender; }

        // get rendering rate
        inline float renderRate() const { return m_renderRate; }

        //--

        // request redraw of the panel
        void requestRedraw();

        // toggle automatic rendering of the panel
        void autoRender(bool flag);

        // set automatic rendering frequency
        void renderRate(float rate);

        //--

        // render the panel, when done submit work to the device
        virtual void renderScene(const rendering::ImageView& color, const rendering::ImageView& depth, uint32_t width, uint32_t height, const rendering::scene::FrameParams_Capture* capture=nullptr);

    protected:
        virtual void handleHoverEnter(const Position& pos) override;
        virtual void handleHoverLeave(const Position& pos) override;
        virtual bool handleMouseMovement(const base::input::MouseMovementEvent& evt) override;
        virtual bool handleKeyEvent(const base::input::KeyEvent& evt) override;

        bool calculateCurrentPixelUnderCursor(base::Point& outPixel) const;

    private:
        rendering::ImageView m_colorSurface; // TODO: make shared!
        rendering::ImageView m_depthSurface; // TODO: make shared!

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

        void releaseSurfaces();
        bool prepareSurfaces(uint32_t minWidth, uint32_t minHeight);
        void renderInternal(const rendering::scene::FrameParams_Capture* capture = nullptr);


        virtual base::StringBuf describe() const override;
        virtual void handleDeviceReset() override;
        virtual void handleDeviceRelease() override;
    };

    ///---

} // ui