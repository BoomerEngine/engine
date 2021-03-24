/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/

#include "build.h"
#include "renderingTest.h"
#include "renderingTestScene.h"

#include "gpu/device/include/device.h"
#include "gpu/device/include/commandWriter.h"
#include "core/image/include/imageUtils.h"
#include "core/image/include/imageView.h"
#include "core/image/include/image.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::test)

class TestDownloadSink : public IDownloadDataSink
{
public:
	TestDownloadSink(int x, int y)
		: m_target(x,y)
	{}

	//--

	INLINE const Point& target() const { return m_target; }

	INLINE bool ready() const { return m_ready.load(); }

	//--

	ImageView retrieveImage() const
	{
		ASSERT(m_ready.load());

		auto lock = CreateLock(m_lock);

		ImageView srcView(NATIVE_LAYOUT, ImagePixelFormat::Uint8_Norm, 4,
			m_state.data.data(), m_state.region.image.sizeX, m_state.region.image.sizeY);

		ASSERT(srcView.dataSize() == m_state.data.size());
		return srcView;
	}

	virtual void processRetreivedData(const void* dataPtr, uint32_t dataSize, const ResourceCopyRange& info) override final
	{
		{
			auto lock = CreateLock(m_lock);
			m_state.data = Buffer::Create(POOL_TEMP, dataSize, 16, dataPtr);
			m_state.region = info;
		}

		m_ready.exchange(true);
	}

private:
	SpinLock m_lock;

	struct
	{
		ResourceCopyRange region;
		Buffer data;
	} m_state;

	std::atomic<bool> m_ready = false;

	Point m_target;
};


/// test of the render to texture functionality
class RenderingTest_DownloadImage : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_DownloadImage, IRenderingTest);

public:
    virtual void initialize() override final;
    virtual void render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView) override final;

	virtual void describeSubtest(IFormatStream& f) override
	{
		if (subTestIndex() == 0)
			f.append("DownloadColor");
		else
			f.append("DownloadDepth");
	}

private:
    SimpleScenePtr m_scene;
    SceneCamera m_camera;
    bool m_showDepth = false;
	bool m_externalClear = false;
	bool m_firstFrame = true;

    GraphicsPipelineObjectPtr m_shaderDraw;
	GraphicsPipelineObjectPtr m_shaderPreview;

    ImageObjectPtr m_colorBuffer;
	RenderTargetViewPtr m_colorBufferRTV;
	ImageSampledViewPtr m_colorBufferSRV;

	ImageObjectPtr m_depthBuffer;
	RenderTargetViewPtr m_depthBufferRTV;
	ImageSampledViewPtr m_depthBufferSRV;

	ImageObjectPtr m_displayImage;
	ImageSampledViewPtr m_displayImageSRV;

	RefPtr<TestDownloadSink> m_downloadSink;

	FastRandState m_rnd;

	ImagePtr m_stage;

    static const uint32_t SIZE = 512;
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_DownloadImage);
    RTTI_METADATA(RenderingTestOrderMetadata).order(2510);
    RTTI_METADATA(RenderingTestSubtestCountMetadata).count(2);
RTTI_END_TYPE();

//---

void RenderingTest_DownloadImage::initialize()
{
	m_showDepth = subTestIndex() & 1;

	GraphicsRenderStatesSetup render;
	render.depth(true);
	render.depthWrite(true);
	render.depthFunc(CompareOp::LessEqual);

    // load shaders
    m_shaderDraw = loadGraphicsShader("GenericScene.csl", &render);
	m_shaderPreview = loadGraphicsShader("GenericGeometryWithTexture.csl");

    // load scene
    m_scene = CreateTeapotScene(*this);

    // create render targets
    gpu::ImageCreationInfo colorBuffer;
    colorBuffer.allowRenderTarget = true;
    colorBuffer.allowShaderReads = true;
	colorBuffer.allowCopies = true;
    colorBuffer.format = ImageFormat::RGBA8_UNORM;
    colorBuffer.width = SIZE;
    colorBuffer.height = SIZE;
    m_colorBuffer = createImage(colorBuffer);
	m_colorBufferSRV = m_colorBuffer->createSampledView();
	m_colorBufferRTV = m_colorBuffer->createRenderTargetView();
            
    gpu::ImageCreationInfo depthBuffer;
    depthBuffer.allowRenderTarget = true;
    depthBuffer.allowShaderReads = true;
	depthBuffer.allowCopies = true;
    depthBuffer.format = ImageFormat::D24S8;
    depthBuffer.width = SIZE;
    depthBuffer.height = SIZE;
    m_depthBuffer = createImage(depthBuffer);
	m_depthBufferSRV = m_depthBuffer->createSampledView();
	m_depthBufferRTV = m_depthBuffer->createRenderTargetView();

	ImageCreationInfo info;
	info.allowDynamicUpdate = true; // important for dynamic update
	info.allowShaderReads = true;
	info.width = SIZE;
	info.height = SIZE;
	info.format = ImageFormat::RGBA8_UNORM;
	m_displayImage = createImage(info);
	m_displayImageSRV = m_displayImage->createSampledView();

	m_stage = RefNew<Image>(ImagePixelFormat::Uint8_Norm, 4, SIZE, SIZE);
	Fill(m_stage->view(), &Vector4::ZERO());
}

void RenderingTest_DownloadImage::render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView)
{
	// rotate the teapot
	{
		auto& teapot = m_scene->m_objects[1];
		teapot.m_params.LocalToWorld = Matrix::BuildRotation(Angles(0.0f, -30.0f + time * 20.0f, 0.0f));
	}

	// clear render targets
	if (m_externalClear)
	{
		int frameIndex = (int)(time * 100.0f);

		auto w = 16 + m_rnd.range(SIZE / 4);
		auto h = 16 + m_rnd.range(SIZE / 4);
		auto x = m_rnd.range(SIZE - w - 1);
		auto y = m_rnd.range(SIZE - h - 1);

		Rect rect;
		rect.min.x = x;
		rect.min.y = y;
		rect.max.x = x + w;
		rect.max.y = y + h;

		if (frameIndex % 60 == 0)
		{
			float clearDepth = m_rnd.range(0.996f, 1.0f);
			cmd.opClearDepthStencil(m_depthBufferRTV, true, true, clearDepth, 0, &rect, 1);
		}
		else if (frameIndex % 10 == 0)
		{
			Vector4 color;
			color.x = m_rnd.unit();
			color.y = m_rnd.unit();
			color.z = m_rnd.unit();
			color.w = 1.0f;

			cmd.opClearRenderTarget(m_colorBufferRTV, color, &rect, 1);
		}
	}

	// render scene
	{
		// setup scene camera
		SceneCamera camera;
		camera.position = Vector3(-4.5f, 0.5f, 1.5f);
		camera.rotation = Angles(10.0f, 0.0f, 0.0f).toQuat();
		camera.calcMatrices();

		// render shit to render targets
		FrameBuffer fb;
		if (m_externalClear && !m_firstFrame)
		{
			fb.color[0].view(m_colorBufferRTV);
			fb.depth.view(m_depthBufferRTV);
		}
		else
		{
			fb.color[0].view(m_colorBufferRTV).clear(Vector4(0.2f, 0.2f, 0.2f, 1.0f));
			fb.depth.view(m_depthBufferRTV).clearDepth(1.0f).clearStencil(0.0f);
		}

		cmd.opBeingPass(fb);
		m_scene->draw(cmd, m_shaderDraw, camera);
		cmd.opEndPass();
	}

	// transition resources
	cmd.opTransitionLayout(m_colorBuffer, ResourceLayout::RenderTarget, ResourceLayout::ShaderResource);
	cmd.opTransitionLayout(m_depthBuffer, ResourceLayout::DepthWrite, ResourceLayout::ShaderResource);

	// get copy ?
	if (m_downloadSink && m_downloadSink->ready())
	{
		// copy image into stage (demo)
		auto srcView = m_downloadSink->retrieveImage();
		auto targetView = m_stage->view().subView(m_downloadSink->target().x, m_downloadSink->target().y, srcView.width(), srcView.height());
		Copy(srcView, targetView);

		// update preview
		{
			cmd.opTransitionLayout(m_displayImage, ResourceLayout::ShaderResource, ResourceLayout::CopyDest);
			cmd.opUpdateDynamicImage(m_displayImage, targetView, 0, 0, m_downloadSink->target().x, m_downloadSink->target().y);
			cmd.opTransitionLayout(m_displayImage, ResourceLayout::CopyDest, ResourceLayout::ShaderResource);
		}

		// delete the download sink (not the area though)
		m_downloadSink.reset();
	}

	// start new copy ?
	if (!m_downloadSink)
	{
		auto w = 16 + m_rnd.range(64);
		auto h = 16 + m_rnd.range(64);
		auto sx = m_rnd.range(m_colorBufferRTV->width() - w);
		auto sy = m_rnd.range(m_colorBufferRTV->height() - h);
		auto dx = sx;// rnd.range(m_colorBufferRTV->width() - w);
		auto dy = sy;// rnd.range(m_colorBufferRTV->height() - h);

		m_downloadSink = RefNew<TestDownloadSink>(dx, dy);

		ResourceCopyRange srcRange;
		srcRange.image.mip = 0;
		srcRange.image.slice = 0;
		srcRange.image.offsetX = sx;
		srcRange.image.offsetY = sy;
		srcRange.image.offsetZ = 0;
		srcRange.image.sizeX = w;
		srcRange.image.sizeY = h;
		srcRange.image.sizeZ = 1;

		if (m_showDepth)
		{
			cmd.opTransitionLayout(m_depthBuffer, ResourceLayout::ShaderResource, ResourceLayout::CopySource);
			cmd.opDownloadData(m_depthBuffer, srcRange, m_downloadSink);
			cmd.opTransitionLayout(m_depthBuffer, ResourceLayout::CopySource, ResourceLayout::ShaderResource);
		}
		else
		{
			cmd.opTransitionLayout(m_colorBuffer, ResourceLayout::ShaderResource, ResourceLayout::CopySource);
			cmd.opDownloadData(m_colorBuffer, srcRange, m_downloadSink);
			cmd.opTransitionLayout(m_colorBuffer, ResourceLayout::CopySource, ResourceLayout::ShaderResource);
		}
	}

	// render preview
    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(Vector4(0.0f, 0.0f, 0.2f, 1.0f));
    cmd.opBeingPass(fb);
    {
		DescriptorEntry desc[1];
		desc[0] = m_displayImageSRV;
        cmd.opBindDescriptor("TestParams"_id, desc);

        drawQuad(cmd, m_shaderPreview, -0.8f, -0.8f, 1.6f, 1.6f);
    }
    cmd.opEndPass();

	m_firstFrame = false;
}

END_BOOMER_NAMESPACE_EX(gpu::test)
