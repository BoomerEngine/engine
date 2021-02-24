/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/

#include "build.h"
#include "renderingTest.h"

#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingCommandWriter.h"

#include "base/image/include/image.h"
#include "base/image/include/imageUtils.h"
#include "../../device/include/renderingDeviceGlobalObjects.h"

BEGIN_BOOMER_NAMESPACE(rendering::test)

/// depth compare test
class RenderingTest_DepthCompareSampler : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_DepthCompareSampler, IRenderingTest);

public:
    virtual void initialize() override final;
    virtual void render(GPUCommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView) override final;

private:
    GraphicsPipelineObjectPtr m_shaderTest;
	GraphicsPipelineObjectPtr m_shaderPreview;

    BufferObjectPtr m_vertexBuffer;

    ImageObjectPtr m_depthBuffer;
	RenderTargetViewPtr m_depthBufferRTV;
	ImageSampledViewPtr m_depthBufferSRV;

    uint32_t m_vertexCount;
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_DepthCompareSampler);
    RTTI_METADATA(RenderingTestOrderMetadata).order(1100);
RTTI_END_TYPE();

//---       

static float RandOne()
{
    return (float)rand() / (float)RAND_MAX;
}

static float RandRange(float min, float max)
{
    return min + (max - min) * RandOne();
}

static void PrepareTestGeometry(float x, float y, float w, float h, base::Array<Simple3DVertex>& outVertices)
{
    static auto NUM_TRIS = 256;

    static const float PHASE_A = 0.0f;
    static const float PHASE_B = DEG2RAD * 120.0f;
    static const float PHASE_C = DEG2RAD * 240.0f;

    srand(0);
    for (uint32_t i = 0; i < 50; ++i)
    {
        auto radius = 0.05f + 0.4f * RandOne();
        auto color = base::Color::FromVectorLinear(base::Vector4(RandRange(0.2f, 1.0f), RandRange(0.2f, 1.0f), RandRange(0.2f, 1.0f), 1.0f));

        auto ox = RandRange(-0.95f + radius, 0.95f - radius);
        auto oy = RandRange(-0.95f + radius, 0.95f - radius);

        auto phase = RandRange(0, 7.0f);
        auto ax = ox + radius * cos(PHASE_A + phase);
        auto ay = oy + radius * sin(PHASE_A + phase);
        auto az = RandRange(0.0f, 1.0f);

        auto bx = ox + radius * cos(PHASE_B + phase);
        auto by = oy + radius * sin(PHASE_B + phase);
        auto bz = RandRange(0.0f, 1.0f);

        auto cx = ox + radius * cos(PHASE_C + phase);
        auto cy = oy + radius * sin(PHASE_C + phase);
        auto cz = RandRange(0.0f, 1.0f);

        outVertices.emplaceBack(ax, ay, az, 0.0f, 0.0f, color);
        outVertices.emplaceBack(bx, by, bz, 0.0f, 0.0f, color);
        outVertices.emplaceBack(cx, cy, cz, 0.0f, 0.0f, color);
    }
}

void RenderingTest_DepthCompareSampler::initialize()
{
    {
        base::Array<Simple3DVertex> vertices;
        PrepareTestGeometry(-0.9f, -0.9f, 1.8f, 1.8f, vertices);
        m_vertexBuffer = createVertexBuffer(vertices);
        m_vertexCount = vertices.size();
    }

    m_shaderTest = loadGraphicsShader("DepthCompareSamplerDraw.csl");
    m_shaderPreview = loadGraphicsShader("DepthCompareSamplerPreview.csl");
}
        
void RenderingTest_DepthCompareSampler::render(GPUCommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView)
{
    // create depth render target
    if (m_depthBuffer.empty())
    {
        ImageCreationInfo info;
        info.allowShaderReads = true;
        info.allowRenderTarget = true;
        info.format = rendering::ImageFormat::D24S8;
        info.width = backBufferView->width();
        info.height = backBufferView->height();
        m_depthBuffer = createImage(info);

		m_depthBufferRTV = m_depthBuffer->createRenderTargetView();
		m_depthBufferSRV = m_depthBuffer->createSampledView();
    }

    // draw the triangles
    {
        FrameBuffer fb;
        fb.depth.view(m_depthBufferRTV).clearDepth(1.0f).clearStencil(0);

        cmd.opBeingPass(fb);
        cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBuffer);
        cmd.opDraw(m_shaderTest, 0, m_vertexCount);
        cmd.opEndPass();
    }

    // transition buffer to allow reading in shader
	cmd.opTransitionLayout(m_depthBuffer, ResourceLayout::DepthWrite, ResourceLayout::ShaderResource);

    // draw the depth compare test
    {
        FrameBuffer fb;
        fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

        cmd.opBeingPass(fb);

        float zRef = 0.5f + 0.5f * sinf(time);

		DescriptorEntry tempParams[3];
		tempParams[0].constants(zRef);
		tempParams[1] = rendering::Globals().SamplerPointDepthLE;
		tempParams[2] = m_depthBufferSRV;
        cmd.opBindDescriptor("TestParams"_id, tempParams);

        drawQuad(cmd, m_shaderPreview, -0.9f, -0.9f, 1.8f, 1.8f);
        cmd.opEndPass();
    }

    //--
}

END_BOOMER_NAMESPACE(rendering::test)
