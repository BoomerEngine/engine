/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: widgets #]
***/

#include "build.h"
#include "uiRenderingPanel.h"

#include "gpu/device/include/commandBuffer.h"
#include "gpu/device/include/commandWriter.h"
#include "gpu/device/include/object.h"
#include "gpu/device/include/device.h"
#include "gpu/device/include/deviceService.h"
#include "gpu/device/include/output.h"
#include "gpu/device/include/descriptor.h"

#include "engine/canvas/include/batchRenderer.h"
#include "engine/canvas/include/geometry.h"
#include "engine/canvas/include/canvas.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

/// custom rendering handler
class CanvasRenderingPanelIntegrationHandler : public ICanvasSimpleBatchRenderer
{
    RTTI_DECLARE_VIRTUAL_CLASS(CanvasRenderingPanelIntegrationHandler, ICanvasSimpleBatchRenderer);

public:
	virtual gpu::ShaderObjectPtr loadMainShaderFile() override
	{
        return gpu::LoadStaticShaderDeviceObject("editor/canvas_rendering_panel_integration.fx");
	}

	virtual void render(gpu::CommandWriter& cmd, const RenderData& data, uint32_t firstVertex, uint32_t numVertices) const override
    {
        const auto* view = *(const gpu::ImageSampledView**)data.customData;

		gpu::DescriptorEntry desc[1];
		desc[0] = view;
        cmd.opBindDescriptor("CanvasRenderingPanelIntegrationParams"_id, desc);

		TBaseClass::render(cmd, data, firstVertex, numVertices);
    }
};

RTTI_BEGIN_TYPE_CLASS(CanvasRenderingPanelIntegrationHandler);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(RenderingPanel);
    RTTI_METADATA(ElementClassNameMetadata).name("RenderingPanel");
RTTI_END_TYPE();

//--

ConfigProperty<float> cvDefaultRenderingPanelRefreshRate("UI.RenderingPanel", "RefreshRate", 60.0f);
ConfigProperty<bool> cvDefaultRenderingPanelAutoRender("UI.RenderingPanel", "ForceAutoRender", false);
ConfigProperty<int> cvRenderingPanelRenderTargetStep("UI.RenderingPanel", "RenderTargetSizeStep", 256);

//--

RenderingPanel::RenderingPanel()
{
    hitTest(true);
    allowFocusFromClick(true);
    enableAutoExpand(true, true);

    m_renderRate = cvDefaultRenderingPanelRefreshRate.get();
    m_autoRender = cvDefaultRenderingPanelAutoRender.get();
    m_renderRequest = false;

	m_quadGeometry = new CanvasGeometry();
	m_quadGeometry->vertices.resize(4);

	auto& batch = m_quadGeometry->batches.emplaceBack();
	batch.vertexCount = 4;
	batch.packing = CanvasBatchPacking::Quads;
	batch.renderDataSize = sizeof(gpu::RenderTargetView*);
	batch.rendererIndex = GetCanvasHandlerIndex<CanvasRenderingPanelIntegrationHandler>();

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

#if 0
void RenderingPanel::renderContent(const ViewportParams& viewport, Camera* outCameraUsedToRender)
{
    gpu::CommandWriter cmd(TempString("PreviewPanel"));

    {
        gpu::FrameBuffer fb;
        fb.color[0].view(viewport.colorBuffer).clear(Vector4(0.1f, 0.1f, 0.2f, 1.0f));
        fb.depth.view(viewport.depthBuffer).clearDepth(1.0f).clearStencil(0);

        cmd.opBeingPass(fb);
        cmd.opEndPass();
    }

    auto device = GetService<DeviceService>()->device();
    device->submitWork(cmd.release());
}
#endif

bool RenderingPanel::parepareRenderTargets(Point requiredSize, gpu::AcquiredOutput& outOutput)
{
	auto device = GetService<DeviceService>()->device();

	auto prevWidth = 0;
	auto prevHeight = 0;

	if (m_colorSurface && (requiredSize.x > m_colorSurface->width() || requiredSize.y > m_colorSurface->height()))
	{
		prevWidth = m_colorSurface->width();
		prevHeight = m_colorSurface->height();

		m_colorSurfaceRTV.reset();
		m_depthSurfaceRTV.reset();
		m_colorSurface.reset();
		m_depthSurface.reset();
	}

	if (!m_colorSurface || !m_depthSurface)
	{
		const auto alignedWidth = std::max<uint32_t>(prevWidth, Align(requiredSize.x, 256));
		const auto alignedHeight = std::max<uint32_t>(prevHeight, Align(requiredSize.y, 256));

		// create color surface
		gpu::ImageCreationInfo initInfo;
		initInfo.allowShaderReads = true;
		initInfo.allowRenderTarget = true;
		initInfo.allowCopies = true;
		initInfo.format = ImageFormat::RGBA8_UNORM;
		initInfo.width = alignedWidth;
		initInfo.height = alignedHeight;
		m_colorSurface = device->createImage(initInfo);
		m_colorSurfaceRTV = m_colorSurface->createRenderTargetView();
		m_colorSurfaceSRV = m_colorSurface->createSampledView();

		// create depth surface
		initInfo.format = ImageFormat::D24FS8;
		m_depthSurface = device->createImage(initInfo);
		m_depthSurfaceRTV = m_depthSurface->createRenderTargetView();

		// setup quad data
		auto* batchDataPtr = (gpu::ImageSampledView**) m_quadGeometry->customData.data();
		*batchDataPtr = m_colorSurfaceSRV.get();
	}
		
	outOutput.width = requiredSize.x;
	outOutput.height = requiredSize.y;
	outOutput.color = m_colorSurfaceRTV;
	outOutput.depth = m_depthSurfaceRTV;

	return true;
}

void RenderingPanel::renderCaptureScene(const CameraSetup& camera, const FrameParams_Capture* capture)
{
	const auto size = Point(cachedDrawArea().size());

    if (size.x && size.y)
    {
		gpu::AcquiredOutput output;
		if (parepareRenderTargets(size, output))
		{
			gpu::CommandWriter cmd(TempString("PreviewPanelCapture"));

			handleRender(cmd, output, camera, capture);

			if (auto device = GetService<DeviceService>()->device())
			{
				device->submitWork(cmd.release());
				device->sync(true);
				device->sync(true);
				device->sync(true);
			}
		}
    }
}

void RenderingPanel::renderForeground(DataStash& stash, const ElementArea& drawArea, Canvas& canvas, float mergedOpacity)
{
    TBaseClass::renderForeground(stash, drawArea, canvas, mergedOpacity);

    // we need valid render area
    const auto size = Point(cachedDrawArea().size());
    if (size.x && size.y)
    {
		// prepare cached render targets
        gpu::AcquiredOutput output;
		if (parepareRenderTargets(size, output))
		{
			// send to rendering
			{
				gpu::CommandWriter cmd(TempString("PreviewPanel"));

				CameraSetup camera;
				handleCamera(camera);

				handleRender(cmd, output, camera, nullptr);

				if (auto device = GetService<DeviceService>()->device())
					device->submitWork(cmd.release());
			}
		}

		// blit into canvas
		if (m_colorSurface)
		{
			// region to render
			float zoomScale = 1.0f / (1 << m_renderTargetZoom);
			float x0 = m_renderTargetOffset.x;
			float y0 = m_renderTargetOffset.y;
			float x1 = m_renderTargetOffset.x + size.x * zoomScale;
			float y1 = m_renderTargetOffset.y + size.y * zoomScale;

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
			v[0].color = Color::WHITE;
			v[1].color = Color::WHITE;
			v[2].color = Color::WHITE;
			v[3].color = Color::WHITE;
			v[0].pos.x = 0;
			v[0].pos.y = 0;
			v[1].pos.x = size.x;
			v[1].pos.y = 0;
			v[2].pos.x = size.x;
			v[2].pos.y = size.y;
			v[3].pos.x = 0;
			v[3].pos.y = size.y;

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

bool RenderingPanel::handleMouseMovement(const InputMouseMovementEvent& evt)
{
    m_currentHoverAbsolutePositionValid = true;
    m_currentHoverAbsolutePosition = evt.absolutePosition().toVector();
    return TBaseClass::handleMouseMovement(evt);
}

bool RenderingPanel::handleKeyEvent(const InputKeyEvent& evt)
{
    if (evt.pressed()) // NO REPEAT
    {
        int delta = 0;
        if (evt.keyCode() == InputKey::KEY_EQUAL)
            delta = 1;
        else if (evt.keyCode() == InputKey::KEY_MINUS)
            delta = -1;

        if (delta)
        {
            int newLevel = std::clamp<int>(m_renderTargetZoom + delta, 0, 16);
            if (newLevel != m_renderTargetZoom)
            {
                if (newLevel == 0)
                {
                    m_renderTargetOffset = Point(0, 0);
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

bool RenderingPanel::calculateCurrentPixelUnderCursor(Point& outPixel) const
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

END_BOOMER_NAMESPACE_EX(ui)
