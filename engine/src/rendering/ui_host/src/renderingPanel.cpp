/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: widgets #]
***/

#include "build.h"
#include "renderingPanel.h"

#include "rendering/device/include/renderingCommandBuffer.h"
#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/device/include/renderingObject.h"
#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingDeviceService.h"
#include "rendering/device/include/renderingOutput.h"
#include "rendering/canvas/include/renderingCanvasBatchRenderer.h"
#include "rendering/device/include/renderingDescriptor.h"
#include "rendering/device/include/renderingShaderFile.h"

namespace ui
{
    //--

    base::res::StaticResource<rendering::ShaderFile> resCanvasCustomRenderingIntegrationHandler("/editor/shaders/canvas_rendering_panel_integration.csl");

    /// custom rendering handler
    class CanvasRenderingPanelIntegrationHandler : public rendering::canvas::ICanvasSimpleBatchRenderer
    {
        RTTI_DECLARE_VIRTUAL_CLASS(CanvasRenderingPanelIntegrationHandler, rendering::canvas::ICanvasSimpleBatchRenderer);

    public:
		virtual rendering::ShaderFilePtr loadMainShaderFile() override
		{
			return resCanvasCustomRenderingIntegrationHandler.loadAndGet();
		}

		virtual void render(rendering::command::CommandWriter& cmd, const RenderData& data, uint32_t firstVertex, uint32_t numVertices) const override
        {
            const auto* view = *(const rendering::ImageSampledView**)data.customData;

			rendering::DescriptorEntry desc[1];
			desc[0] = view;
            cmd.opBindDescriptor("CanvasRenderingPanelIntegrationParams"_id, desc);

			TBaseClass::render(cmd, data, firstVertex, numVertices);
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

		m_quadGeometry = new base::canvas::Geometry();
		m_quadGeometry->vertices.resize(4);

		auto& batch = m_quadGeometry->batches.emplaceBack();
		batch.vertexCount = 4;
		batch.packing = base::canvas::BatchPacking::Quads;
		batch.renderDataSize = sizeof(rendering::RenderTargetView*);
		batch.rendererIndex = rendering::canvas::GetHandlerIndex<CanvasRenderingPanelIntegrationHandler>();

		m_quadGeometry->customData.allocateWith(batch.renderDataSize, 0);
    }

    RenderingPanel::~RenderingPanel()
    {
		m_colorSurface.reset();
		m_depthSurface.reset();
		m_colorSurfaceRTV.reset();
		m_depthSurfaceRTV.reset();

		delete m_quadGeometry;
		m_quadGeometry = nullptr;
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

    void RenderingPanel::renderContent(const ViewportParams& viewport)
    {
        rendering::command::CommandWriter cmd(base::TempString("PreviewPanel"));

        {
            rendering::FrameBuffer fb;
            fb.color[0].view(viewport.colorBuffer).clear(base::Vector4(0.1f, 0.1f, 0.2f, 1.0f));
            fb.depth.view(viewport.depthBuffer).clearDepth(1.0f).clearStencil(0);

            cmd.opBeingPass(fb);
            cmd.opEndPass();
        }

        auto device = base::GetService<rendering::DeviceService>()->device();
        device->submitWork(cmd.release());
    }
    
	void RenderingPanel::parepareRenderTargets(ViewportParams& viewport)
	{
		auto device = base::GetService<rendering::DeviceService>()->device();

		if (m_colorSurface && (viewport.width > m_colorSurface->width() || viewport.height > m_colorSurface->height()))
		{
			m_colorSurfaceRTV.reset();
			m_depthSurfaceRTV.reset();
			m_colorSurface.reset();
			m_depthSurface.reset();
		}

		if (!m_colorSurface || !m_depthSurface)
		{
			if (viewport.width && viewport.height)
			{
				// create color surface
				rendering::ImageCreationInfo initInfo;
				initInfo.allowShaderReads = true;
				initInfo.allowRenderTarget = true;
				initInfo.allowCopies = true;
				initInfo.format = rendering::ImageFormat::RGBA8_UNORM;
				initInfo.width = viewport.width;
				initInfo.height = viewport.height;
				m_colorSurface = device->createImage(initInfo);
				m_colorSurfaceRTV = m_colorSurface->createRenderTargetView();
				m_colorSurfaceSRV = m_colorSurface->createSampledView();

				// create depth surface
				initInfo.format = rendering::ImageFormat::D24FS8;
				m_depthSurface = device->createImage(initInfo);
				m_depthSurfaceRTV = m_depthSurface->createRenderTargetView();

				// setup quad data
				auto* batchDataPtr = (rendering::ImageSampledView**) m_quadGeometry->customData.data();
				*batchDataPtr = m_colorSurfaceSRV.get();
			}
		}
		
		viewport.colorBuffer = m_colorSurfaceRTV;
		viewport.depthBuffer = m_depthSurfaceRTV;
	}

    void RenderingPanel::renderForeground(DataStash& stash, const ui::ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity)
    {
        TBaseClass::renderForeground(stash, drawArea, canvas, mergedOpacity);

		// render area size
		const auto renderWidth = (int)std::ceil(drawArea.size().x);
		const auto renderHeight = (int)std::ceil(drawArea.size().y);
		if (renderWidth && renderHeight)
		{
			// render content
			{
				ViewportParams params;
				params.width = renderWidth;
				params.height = renderHeight;

				parepareRenderTargets(params);
				renderContent(params);
			}

			// blit into canvas
			{
				// region to render
				float zoomScale = 1.0f / (1 << m_renderTargetZoom);
				float x0 = m_renderTargetOffset.x;
				float y0 = m_renderTargetOffset.y;
				float x1 = m_renderTargetOffset.x + renderWidth * zoomScale;
				float y1 = m_renderTargetOffset.y + renderHeight * zoomScale;

				const auto invU = 1.0f / (float)m_colorSurface->width();
				const auto invV = 1.0f / (float)m_colorSurface->height();

				// render the tile into the UI canvas
				auto* v = m_quadGeometry->vertices.typedData();
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
				v[1].pos.x = renderWidth;
				v[1].pos.y = 0;
				v[2].pos.x = renderWidth;
				v[2].pos.y = renderHeight;
				v[3].pos.x = 0;
				v[3].pos.y = renderHeight;

                /*if (m_colorSurface->flippedY())
                {
                    float deltaY = (m_colorSurface->height() - renderHeight) * invV;
                    v[0].uv.y += deltaY;
                    v[1].uv.y += deltaY;
                    v[2].uv.y += deltaY;
                    v[3].uv.y += deltaY;
                }*/

				canvas.place(drawArea.absolutePosition(), *m_quadGeometry);
			}
		}
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