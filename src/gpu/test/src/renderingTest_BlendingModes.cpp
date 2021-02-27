/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/

#include "build.h"
#include "renderingTest.h"

#include "gpu/device/include/device.h"
#include "gpu/device/include/commandWriter.h"
#include "gpu/device/include/buffer.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::test)

/// basic blending mode test
class RenderingTest_BasicBlendingModes : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_BasicBlendingModes, IRenderingTest);

public:
    virtual void initialize() override final;
    virtual void render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView ) override final;

	virtual void describeSubtest(IFormatStream& f) override
	{
		switch (subTestIndex())
		{
		case 0: f << "Add"; break;
		case 1: f << "Subtract"; break;
		case 2: f << "ReverseSubtract"; break;
		case 3: f << "Min"; break;
		case 4: f << "Max"; break;
		}
	}

private:
    BufferObjectPtr m_backgroundBuffer;
	BufferObjectPtr m_testBuffer;

    static const uint8_t MAX_BLEND_MODES = 15;
    static const uint8_t MAX_BLEND_FUNCS = 5;

    const BlendOp BlendFuncNames[MAX_BLEND_FUNCS] = {
        BlendOp::Add,
        BlendOp::Subtract,
        BlendOp::ReverseSubtract,
        BlendOp::Min,
        BlendOp::Max,
    };

    const BlendFactor BlendModesNames[MAX_BLEND_MODES] = {
        BlendFactor::Zero,
        BlendFactor::One,
        BlendFactor::SrcColor,
        BlendFactor::OneMinusSrcColor,
        BlendFactor::DestColor,
        BlendFactor::OneMinusDestColor,
        BlendFactor::SrcAlpha,
        BlendFactor::OneMinusSrcAlpha,
        BlendFactor::DestAlpha,
        BlendFactor::OneMinusDestAlpha,
        BlendFactor::ConstantColor,
        BlendFactor::OneMinusConstantColor,
        BlendFactor::ConstantAlpha,
        BlendFactor::OneMinusConstantAlpha,
        BlendFactor::SrcAlphaSaturate,
    };

    StringBuf m_blendMode;

	GraphicsPipelineObjectPtr m_backgroundShader;
    GraphicsPipelineObjectPtr m_testShader[MAX_BLEND_MODES][MAX_BLEND_MODES];
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_BasicBlendingModes);
    RTTI_METADATA(RenderingTestOrderMetadata).order(500);
    RTTI_METADATA(RenderingTestSubtestCountMetadata).count(5);
RTTI_END_TYPE();

//---       

void RenderingTest_BasicBlendingModes::initialize()
{
    // allocate background
    {
        Simple3DVertex vertices[6];

        vertices[0].set(-0.9f, -0.9f, 0.5f, 0.0f, 0.0f, Color(128, 128, 128, 255));
        vertices[1].set(+0.9f, -0.9f, 0.5f, 0.0f, 0.0f, Color(128, 128, 128, 0));
        vertices[2].set(+0.9f, +0.9f, 0.5f, 0.0f, 0.0f, Color(255, 255, 255, 0));
        vertices[3].set(-0.9f, -0.9f, 0.5f, 0.0f, 0.0f, Color(128, 128, 128, 255));
        vertices[4].set(+0.9f, +0.9f, 0.5f, 0.0f, 0.0f, Color(255, 255, 255, 0));
        vertices[5].set(-0.9f, +0.9f, 0.5f, 0.0f, 0.0f, Color(255, 255, 255, 255));

        m_backgroundBuffer = createVertexBuffer(sizeof(vertices), vertices);
    }

    // allocate test
    {
        Simple3DVertex vertices[12];

        // quad with dest alpha
        vertices[0].set(-0.9f, -0.7f, 0.5f, 0.0f, 0.0f, Color(255, 255, 255, 255));
        vertices[1].set(+0.9f, -0.7f, 0.5f, 0.0f, 0.0f, Color(0, 255, 0, 255));
        vertices[2].set(+0.9f, -0.1f, 0.5f, 0.0f, 0.0f, Color(255, 0, 0, 0));
        vertices[3].set(-0.9f, -0.7f, 0.5f, 0.0f, 0.0f, Color(255, 255, 255, 255));
        vertices[4].set(+0.9f, -0.1f, 0.5f, 0.0f, 0.0f, Color(255, 0, 0, 0));
        vertices[5].set(-0.9f, -0.1f, 0.5f, 0.0f, 0.0f, Color(0, 0, 255, 0));

        vertices[6].set(-0.9f, +0.1f, 0.5f, 0.0f, 0.0f, Color(0, 0, 0, 0));
        vertices[7].set(+0.9f, +0.1f, 0.5f, 0.0f, 0.0f, Color(255, 0, 255, 0));
        vertices[8].set(+0.9f, +0.7f, 0.5f, 0.0f, 0.0f, Color(0, 255, 255, 255));
        vertices[9].set(-0.9f, +0.1f, 0.5f, 0.0f, 0.0f, Color(0, 0, 0, 0));
        vertices[10].set(+0.9f, +0.7f, 0.5f, 0.0f, 0.0f, Color(0, 255, 255, 255));
        vertices[11].set(-0.9f, +0.7f, 0.5f, 0.0f, 0.0f, Color(255, 255, 0, 255));

        m_testBuffer = createVertexBuffer(sizeof(vertices), vertices);
    }

	// background shader
	m_backgroundShader = loadGraphicsShader("GenericGeometry.csl");

    // load shaders
	for (uint32_t i = 0; i < MAX_BLEND_MODES; ++i)
	{
		for (uint32_t j = 0; j < MAX_BLEND_MODES; ++j)
		{
			GraphicsRenderStatesSetup states;
			states.blend(true);
			states.blendOp(0, BlendFuncNames[subTestIndex()]);
			states.blendFactor(0, BlendModesNames[i], BlendModesNames[j]);

			m_testShader[i][j] = loadGraphicsShader("GenericGeometry.csl", &states);
		}
	}
}

void RenderingTest_BasicBlendingModes::render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView )
{
    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(Vector4(0.0f, 0.0f, 0.2f, 1.0f));

    cmd.opBeingPass(fb);

    auto numBlendModes  = ARRAY_COUNT(BlendModesNames);
    auto viewWidth = backBufferView->width() / numBlendModes;
    auto viewHeight = backBufferView->height() / numBlendModes;

    for (uint32_t y = 0; y < numBlendModes; ++y)
    {
        for (uint32_t x = 0; x < numBlendModes; ++x)
        {
            cmd.opSetViewportRect(0, x * viewWidth, y * viewHeight, viewWidth, viewHeight);

            // background
            {
                cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_backgroundBuffer);
                cmd.opDraw(m_backgroundShader, 0, m_backgroundBuffer->size() / sizeof(Simple3DVertex));
            }

            // test
            {
                cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_testBuffer);
                cmd.opDraw(m_testShader[x][y], 0, m_testBuffer->size() / sizeof(Simple3DVertex));
            }
        }
    }

    cmd.opEndPass();
}

//--

END_BOOMER_NAMESPACE_EX(gpu::test)
