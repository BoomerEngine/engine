/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/

#include "build.h"
#include "renderingTest.h"

#include "gpu/device/include/renderingDeviceApi.h"
#include "gpu/device/include/renderingCommandWriter.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::test)

/// cull mode test
class RenderingTest_CullFrontFacing : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_CullFrontFacing, IRenderingTest);

public:
    virtual void initialize() override final;
    virtual void render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView ) override final;

private:
	static const auto NUM_FACE_MODES = 2;
	static const auto NUM_CULL_MODES = 4;

	static inline const gpu::FrontFace FACE_MODES[NUM_FACE_MODES] = {
		gpu::FrontFace::CW,
		gpu::FrontFace::CCW
	};

	static inline const gpu::CullMode CULL_MODES[NUM_CULL_MODES] = {
		gpu::CullMode::Front,
		gpu::CullMode::Front,
		gpu::CullMode::Back,
		gpu::CullMode::Both
	};

    BufferObjectPtr m_vertexBuffer;

    GraphicsPipelineObjectPtr m_testShader[NUM_FACE_MODES][NUM_CULL_MODES];
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_CullFrontFacing);
    RTTI_METADATA(RenderingTestOrderMetadata).order(31);
RTTI_END_TYPE();

//---       

static void AddQuad(float x, float y, float w, float h, bool cwOrder, Color color, Simple3DVertex*& writePtr)
{
    float hw = w * 0.5f;
    float hh = h * 0.5f;
    if (cwOrder)
    {
        writePtr[0].set(x - hw, y - hh, 0.5f, 0, 0, color);
        writePtr[1].set(x + hw, y - hh, 0.5f, 0, 0, color);
        writePtr[2].set(x + hw, y + hh, 0.5f, 0, 0, color);
        writePtr[3].set(x - hw, y - hh, 0.5f, 0, 0, color);
        writePtr[4].set(x + hw, y + hh, 0.5f, 0, 0, color);
        writePtr[5].set(x - hw, y + hh, 0.5f, 0, 0, color);
    }
    else
    {
        writePtr[0].set(x + hw, y + hh, 0.5f, 0, 0, color);
        writePtr[1].set(x + hw, y - hh, 0.5f, 0, 0, color);
        writePtr[2].set(x - hw, y - hh, 0.5f, 0, 0, color);
        writePtr[3].set(x - hw, y + hh, 0.5f, 0, 0, color);
        writePtr[4].set(x + hw, y + hh, 0.5f, 0, 0, color);
        writePtr[5].set(x - hw, y - hh, 0.5f, 0, 0, color);
    }
    writePtr += 6;
}

void RenderingTest_CullFrontFacing::initialize()
{
    // create vertex buffer with a single triangle
    {
        Simple3DVertex vertices[6*2*4*2];

        Simple3DVertex* writePtr = vertices;
        float x = -0.85f;
        float y = -0.5f;
        AddQuad(x, y, 0.2f, 0.8f, true, Color::WHITE, writePtr); x += 0.2f;
        AddQuad(x, y, 0.2f, 0.8f, false, Color::WHITE, writePtr); x += 0.3f;
        AddQuad(x, y, 0.2f, 0.8f, true, Color::WHITE, writePtr); x += 0.2f;
        AddQuad(x, y, 0.2f, 0.8f, false, Color::WHITE, writePtr); x += 0.3f;
        AddQuad(x, y, 0.2f, 0.8f, true, Color::WHITE, writePtr); x += 0.2f;
        AddQuad(x, y, 0.2f, 0.8f, false, Color::WHITE, writePtr); x += 0.3f;
        AddQuad(x, y, 0.2f, 0.8f, true, Color::WHITE, writePtr); x += 0.2f;
        AddQuad(x, y, 0.2f, 0.8f, false, Color::WHITE, writePtr); x += 0.3f;
        x = -0.85f;
        y = 0.5f;
        AddQuad(x, y, 0.2f, 0.8f, true, Color::WHITE, writePtr); x += 0.2f;
        AddQuad(x, y, 0.2f, 0.8f, false, Color::WHITE, writePtr); x += 0.3f;
        AddQuad(x, y, 0.2f, 0.8f, true, Color::WHITE, writePtr); x += 0.2f;
        AddQuad(x, y, 0.2f, 0.8f, false, Color::WHITE, writePtr); x += 0.3f;
        AddQuad(x, y, 0.2f, 0.8f, true, Color::WHITE, writePtr); x += 0.2f;
        AddQuad(x, y, 0.2f, 0.8f, false, Color::WHITE, writePtr); x += 0.3f;
        AddQuad(x, y, 0.2f, 0.8f, true, Color::WHITE, writePtr); x += 0.2f;
        AddQuad(x, y, 0.2f, 0.8f, false, Color::WHITE, writePtr); x += 0.3f;

        m_vertexBuffer = createVertexBuffer(sizeof(vertices), vertices);
    }

	for (uint32_t i = 0; i < NUM_FACE_MODES; ++i)
	{
		for (uint32_t j = 0; j < NUM_CULL_MODES; ++j)
		{
			GraphicsRenderStatesSetup setup;
			setup.primitiveTopology(PrimitiveTopology::TriangleList);
			setup.cull(j != 0);
			setup.cullFrontFace(FACE_MODES[i]);
			setup.cullMode(CULL_MODES[j]);
			m_testShader[i][j] = loadGraphicsShader("FrontFacing.csl", &setup);
		}
	}
}

void RenderingTest_CullFrontFacing::render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView )
{
    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(Vector4(0.0f, 0.0f, 0.2f, 1.0f));

    cmd.opBeingPass(fb);
    cmd.opBindVertexBuffer("Simple3DVertex"_id, m_vertexBuffer);

	int pos = 0;
	for (uint32_t i = 0; i < NUM_FACE_MODES; ++i)
	{
		for (uint32_t j = 0; j < NUM_CULL_MODES; ++j)
		{
            cmd.opDraw(m_testShader[i][j], 12 * pos, 12);
            pos += 1;
        }
    }

    cmd.opEndPass();
}

END_BOOMER_NAMESPACE_EX(gpu::test)
