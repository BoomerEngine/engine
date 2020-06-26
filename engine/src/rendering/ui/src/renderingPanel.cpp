/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: widgets #]
***/

#include "build.h"
#include "renderingPanel.h"

#include "rendering/driver/include/renderingCommandBuffer.h"
#include "rendering/driver/include/renderingCommandWriter.h"
#include "rendering/driver/include/renderingObject.h"
#include "rendering/driver/include/renderingShaderLibrary.h"
#include "rendering/driver/include/renderingDriver.h"
#include "rendering/driver/include/renderingDeviceService.h"
#include "rendering/driver/include/renderingOutput.h"
#include "rendering/canvas/include/renderingCanvasRendererCustomBatchHandler.h"

namespace ui
{
    //--

    base::res::StaticResource<rendering::ShaderLibrary> resCanvasCustomRenderingIntegrationHandler("editor/shaders/canvas_rendering_panel_integration.csl");

    /// custom rendering handler
    class CanvasRenderingPanelIntegrationHandler : public rendering::canvas::ICanvasRendererCustomBatchHandler
    {
        RTTI_DECLARE_VIRTUAL_CLASS(CanvasRenderingPanelIntegrationHandler, rendering::canvas::ICanvasRendererCustomBatchHandler);

    public:
        virtual void initialize(rendering::IDriver* drv) override final
        {
        }

        virtual void render(rendering::command::CommandWriter& cmd, const base::canvas::Canvas& canvas, const rendering::canvas::CanvasRenderingParams& params, uint32_t firstIndex, uint32_t numIndices, uint32_t numPayloads, const rendering::canvas::CanvasCustomBatchPayload* payloads) override
        {
            if (auto shader = resCanvasCustomRenderingIntegrationHandler.loadAndGet())
            {
                for (uint32_t i = 0; i < numPayloads; ++i)
                {
                    const auto& payload = payloads[i];

                    const auto& surface = *(const rendering::ImageView*)payload.data;
                    cmd.opBindParametersInline("CanvasRenderingPanelIntegrationParams"_id, surface);
                    cmd.opDrawIndexed(shader, 0, payload.firstIndex, payload.numIndices);
                }
            }
        }
    };

    RTTI_BEGIN_TYPE_CLASS(CanvasRenderingPanelIntegrationHandler);
    RTTI_END_TYPE();

    //--

    RTTI_BEGIN_TYPE_CLASS(RenderingPanel);
        RTTI_METADATA(ElementClassNameMetadata).name("RenderingPanel");
    RTTI_END_TYPE();

    //--

    base::ConfigProperty<float> cvDefaultRenderingPanelRefreshRate("UI.RenderingPanel", "RefreshRate", 60.0f);
    base::ConfigProperty<bool> cvDefaultRenderingPanelAutoRender("UI.RenderingPanel", "ForceAutoRender", false);
    base::ConfigProperty<int> cvRenderingPanelRenderTargetStep("UI.RenderingPanel", "RenderTargetSizeStep", 256);

    //--

    RenderingPanel::RenderingPanel()
    {
        hitTest(true);
        allowFocusFromClick(true);
        enableAutoExpand(true, true);

        m_renderRate = cvDefaultRenderingPanelRefreshRate.get();
        m_autoRender = cvDefaultRenderingPanelAutoRender.get();
        m_renderRequest = false;
    }

    RenderingPanel::~RenderingPanel()
    {
        releaseSurfaces();
    }

    void RenderingPanel::releaseSurfaces()
    {
        m_colorSurface.destroy();
        m_depthSurface.destroy();
    }

    void RenderingPanel::requestRedraw()
    {
        m_renderRequest = true;
    }

    void RenderingPanel::autoRender(bool flag)
    {
        m_autoRender = flag;
    }

    void RenderingPanel::renderRate(float rate)
    {
        m_renderRate = std::clamp<float>(rate, 0.0f, 1000.0f);
    }

    void RenderingPanel::renderScene(const rendering::ImageView& color, const rendering::ImageView& depth, uint32_t width, uint32_t height, const rendering::scene::FrameParams_Capture* capture /*= nullptr*/)
    {
        rendering::command::CommandWriter cmd(base::TempString("PreviewPanel"));

        {
            rendering::FrameBuffer fb;
            fb.color[0].view(color).clear(0.1f, 0.1f, 0.2f, 1.0f);
            fb.depth.view(depth).clearDepth().clearStencil();

            cmd.opBeingPass(fb);
            cmd.opEndPass();
        }

        device()->submitWork(cmd.release());
    }

    bool RenderingPanel::prepareSurfaces(uint32_t minWidth, uint32_t minHeight)
    {
        // no device
        if (!device())
            return false;

        // check if current surfaces are ok
        //if (minWidth <= m_colorSurface.width() && minHeight <= m_colorSurface.height())
            //return true;
        if (minWidth == m_colorSurface.width() && minHeight == m_colorSurface.height())
            return true;

        // round up the render target size
        const auto tileSize = std::clamp<uint32_t>(cvRenderingPanelRenderTargetStep.get(), 64, 1024);
        const auto requestedWidth = minWidth; //((minWidth + tileSize - 1) / tileSize)* tileSize;
        const auto requestedHeight = minHeight;// ((minHeight + tileSize - 1) / tileSize)* tileSize;

        // release current surfaces
        m_colorSurface.destroy();
        m_depthSurface.destroy();

        // create color surface
        rendering::ImageCreationInfo initInfo;
        initInfo.allowShaderReads = true;
        initInfo.allowRenderTarget = true;
        initInfo.allowCopies = true;
        initInfo.format = rendering::ImageFormat::RGBA8_UNORM;
        initInfo.width = requestedWidth;
        initInfo.height = requestedHeight;
        m_colorSurface = device()->createImage(initInfo);
        if (!m_colorSurface)
            return false;

        // create depth surface
        initInfo.format = rendering::ImageFormat::D24FS8;
        m_depthSurface = device()->createImage(initInfo);
        if (!m_depthSurface)
        {
            m_colorSurface.destroy();
            return false;
        }

        return true;
    }

    void RenderingPanel::renderForeground(const ui::ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity)
    {
        TBaseClass::renderForeground(drawArea, canvas, mergedOpacity);

        const auto width = (uint32_t)std::floorf(drawArea.size().x);
        const auto height = (uint32_t)std::floorf(drawArea.size().y);
        if (width && height)
        {
            if (prepareSurfaces(width, height))
            {
                // render the scene to the back buffers
                renderInternal(nullptr);

                // region to render
                float zoomScale = 1.0f / (1 << m_renderTargetZoom);
                float x0 = m_renderTargetOffset.x;
                float y0 = m_renderTargetOffset.y;
                float x1 = m_renderTargetOffset.x + width * zoomScale;
                float y1 = m_renderTargetOffset.y + height * zoomScale;

                const auto invU = 1.0f / (float)m_colorSurface.width();
                const auto invV = 1.0f / (float)m_colorSurface.height();

                // render the tile into the UI canvas
                base::canvas::Canvas::RawVertex v[4];
                v[0].uv.x = x0 * invU;
                v[0].uv.y = y0 * invV;
                v[1].uv.x = x1 * invU;
                v[1].uv.y = y0 * invV;
                v[2].uv.x = x1 * invU;
                v[2].uv.y = y1 * invV;
                v[3].uv.x = x0 * invU;
                v[3].uv.y = y1 * invV;
                v[0].color = base::Color::WHITE;
                v[1].color = base::Color::WHITE;
                v[2].color = base::Color::WHITE;
                v[3].color = base::Color::WHITE;
                v[0].pos.x = 0;
                v[0].pos.y = 0;
                v[1].pos.x = width;
                v[1].pos.y = 0;
                v[2].pos.x = width;
                v[2].pos.y = height;
                v[3].pos.x = 0;
                v[3].pos.y = height;

                uint16_t i[6];
                i[0] = 0;
                i[1] = 1;
                i[2] = 2;
                i[3] = 0;
                i[4] = 2;
                i[5] = 3;

                base::canvas::Canvas::RawGeometry geom;
                geom.indices = i;
                geom.vertices = v;
                geom.numIndices = 6;

                static const auto customDrawerId = rendering::canvas::GetHandlerIndex<CanvasRenderingPanelIntegrationHandler>();

                rendering::ImageView colorSurface = m_colorSurface.createSampledView(rendering::ObjectID::DefaultPointSampler());

                const auto style = base::canvas::SolidColor(base::Color::WHITE);
                canvas.placement(drawArea.left(), drawArea.top());
                canvas.place(style, geom, customDrawerId, canvas.uploadCustomPayloadData(colorSurface));
            }
        }    
    }

    void RenderingPanel::renderInternal(const rendering::scene::FrameParams_Capture* capture /*= nullptr*/)
    {
        const auto width = (uint32_t)std::floorf(cachedDrawArea().size().x);
        const auto height = (uint32_t)std::floorf(cachedDrawArea().size().y);
        if (width && height && m_colorSurface && m_depthSurface)
        {
            renderScene(m_colorSurface, m_depthSurface, width, height, capture);
        }
    }
    
    //--

    base::StringBuf RenderingPanel::describe() const
    {
        return "RenderingPanel";
    }

    void RenderingPanel::handleDeviceReset()
    {
        // nothing, recreated on first use
    }

    void RenderingPanel::handleDeviceRelease()
    {
        releaseSurfaces();
    }

    //--

    void RenderingPanel::handleHoverEnter(const Position& pos)
    {
        m_currentHoverAbsolutePositionValid = true;
        m_currentHoverAbsolutePosition = pos;
        TBaseClass::handleHoverEnter(pos);
    }

    void RenderingPanel::handleHoverLeave(const Position& pos)
    {
        m_currentHoverAbsolutePositionValid = false;
        TBaseClass::handleHoverLeave(pos);
    }

    bool RenderingPanel::handleMouseMovement(const base::input::MouseMovementEvent& evt)
    {
        m_currentHoverAbsolutePositionValid = true;
        m_currentHoverAbsolutePosition = evt.absolutePosition().toVector();
        return TBaseClass::handleMouseMovement(evt);
    }

    bool RenderingPanel::handleKeyEvent(const base::input::KeyEvent& evt)
    {
        if (evt.pressed()) // NO REPEAT
        {
            int delta = 0;
            if (evt.keyCode() == base::input::KeyCode::KEY_EQUAL)
                delta = 1;
            else if (evt.keyCode() == base::input::KeyCode::KEY_MINUS)
                delta = -1;

            if (delta)
            {
                int newLevel = std::clamp<int>(m_renderTargetZoom + delta, 0, 16);
                if (newLevel != m_renderTargetZoom)
                {
                    if (newLevel == 0)
                    {
                        m_renderTargetOffset = base::Point(0, 0);
                        m_renderTargetZoom = 0;
                    }
                    else if (m_currentHoverAbsolutePositionValid)
                    {
                        int localX = (int)std::roundf(m_currentHoverAbsolutePosition.x - cachedDrawArea().left());
                        int localY = (int)std::roundf(m_currentHoverAbsolutePosition.y - cachedDrawArea().top());

                        int pixelX = m_renderTargetOffset.x + (localX >> m_renderTargetZoom);
                        int pixelY = m_renderTargetOffset.y + (localY >> m_renderTargetZoom);

                        m_renderTargetZoom = newLevel;
                        m_renderTargetOffset.x = pixelX - (localX >> m_renderTargetZoom);
                        m_renderTargetOffset.y = pixelY - (localY >> m_renderTargetZoom);
                    }
                }
            }
        }

        return TBaseClass::handleKeyEvent(evt);
    }

    //--

    bool RenderingPanel::calculateCurrentPixelUnderCursor(base::Point& outPixel) const
    {
        if (m_currentHoverAbsolutePositionValid)
        {
            int localX = (int)std::roundf(m_currentHoverAbsolutePosition.x - cachedDrawArea().left());
            int localY = (int)std::roundf(m_currentHoverAbsolutePosition.y - cachedDrawArea().top());

            outPixel.x = m_renderTargetOffset.x + (localX >> m_renderTargetZoom);
            outPixel.y = m_renderTargetOffset.y + (localY >> m_renderTargetZoom);

            return true;
        }

        return false;
    }

    //--

} // rendering