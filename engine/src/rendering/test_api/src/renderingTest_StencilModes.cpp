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

namespace rendering
{
    namespace test
    {
        /// basic blending mode test
        class RenderingTest_StencilModes : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_StencilModes, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView) override final;

			virtual void describeSubtest(base::IFormatStream& f) override
			{
				switch (subTestIndex())
				{
				case 0: f << "Always"; break;
				case 1: f << "GreaterEqual"; break;
				case 2: f << "Greater"; break;
				case 3: f << "Less"; break;
				case 4: f << "LessEqual"; break;
				case 5: f << "Equal"; break;
				case 6: f << "NotEqual"; break;
				case 7: f << "Never"; break;
				}
			}

        private:
            BufferObjectPtr m_testBuffer;
			BufferObjectPtr m_fillBuffer;

            static const uint32_t NUM_OPS = 8;
            static const uint32_t NUM_CMP = 8;

            CompareOp StencilFuncNames[NUM_CMP] = {
                CompareOp::Always,
                CompareOp::GreaterEqual,
                CompareOp::Greater,
                CompareOp::Less,
                CompareOp::LessEqual,
                CompareOp::Equal,
                CompareOp::NotEqual,
                CompareOp::Never
            };

            StencilOp StencilOpNames[NUM_OPS] = {
                StencilOp::Keep,
                StencilOp::Zero,
                StencilOp::Replace,
                StencilOp::IncrementAndClamp,
                StencilOp::DecrementAndClamp,
                StencilOp::IncrementAndWrap,
                StencilOp::DecrementAndWrap
            };

            GraphicsPipelineObjectPtr m_shaderBackground;
			GraphicsPipelineObjectPtr m_shaderPreview;
			GraphicsPipelineObjectPtr m_shaderTest[NUM_OPS][NUM_OPS];
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_StencilModes);
            RTTI_METADATA(RenderingTestOrderMetadata).order(600);
            RTTI_METADATA(RenderingTestSubtestCountMetadata).count(8);
        RTTI_END_TYPE();

        //---       


        static void AddQuad(Simple3DVertex* vertices, float x0, float y0, float x1, float y1, base::Color color = base::Color::WHITE)
        {
            vertices[0].set(x0, y0, 0.5f, 0.0f, 0.0f, color);
            vertices[1].set(x1, y0, 0.5f, 0.0f, 0.0f, color);
            vertices[2].set(x1, y1, 0.5f, 0.0f, 0.0f, color);
            vertices[3].set(x0, y0, 0.5f, 0.0f, 0.0f, color);
            vertices[4].set(x1, y1, 0.5f, 0.0f, 0.0f, color);
            vertices[5].set(x0, y1, 0.5f, 0.0f, 0.0f, color);
        }

        void RenderingTest_StencilModes::initialize()
        {
            // allocate test pattern
            {
                Simple3DVertex vertices[5 * 6];
                AddQuad(vertices + 0, -0.5f, -0.5f, 0.5f, 0.5f); // center
                AddQuad(vertices + 6, -0.9f, -0.9f, -0.1f, -0.1f); // top left
                AddQuad(vertices + 12, 0.1f, -0.9f, 0.9f, -0.1f); // top right
                AddQuad(vertices + 18, -0.9f, 0.1f, -0.1f, 0.9f); // bottom left
                AddQuad(vertices + 24, 0.1f, 0.1f, 0.9f, 0.9f); // bottom right
                m_testBuffer = createVertexBuffer(sizeof(vertices), vertices);
            }

            // allocate fill pattern to resolve the stencil value
            {
                // generate the colors
                base::Vector4 colors[256];
                for (uint32_t i = 0; i < 256; ++i)
                {
                    colors[i].x = 0.25f + 0.125f * (i % 5);
                    colors[i].y = 0.25f + 0.125f * ((i/5) % 5);
                    colors[i].z = 0.25f + 0.125f * (i / 25);
                    colors[i].w = 0.0f;
                }

                colors[0] = base::Vector4(0, 0, 0, 0);
                colors[1] = base::Vector4(1, 0, 0, 0);
                colors[2] = base::Vector4(0, 1, 0, 0);
                colors[3] = base::Vector4(1, 1, 0, 0);
                colors[4] = base::Vector4(0, 0, 1, 0);
                colors[5] = base::Vector4(1, 0, 1, 0);
                colors[6] = base::Vector4(0, 1, 1, 0);
                colors[7] = base::Vector4(1, 1, 1, 0);

                colors[255] = base::Vector4(0.5f, 0, 0, 0);
                colors[254] = base::Vector4(0, 0.5f, 0, 0);
                colors[253] = base::Vector4(0.5f, 0.5f, 0, 0);
                colors[252] = base::Vector4(0, 0, 0.5f, 0);
                colors[251] = base::Vector4(0.5f, 0, 0.5f, 0);
                colors[250] = base::Vector4(0, 0.5f, 0.5f, 0);
                colors[249] = base::Vector4(0.5f, 0.5f, 0.5f, 0);

                Simple3DVertex vertices[256 * 6];

                for (uint32_t i = 0; i < 256; ++i)
                    AddQuad(vertices + 6 * i, -1, -1, 1, 1, base::Color::FromVectorLinear(colors[i]));

                m_fillBuffer = createVertexBuffer(sizeof(vertices), vertices);
            }

			{
				GraphicsRenderStatesSetup setup;
				setup.stencil(true);
				setup.stencilAll(CompareOp::Always, StencilOp::Replace, StencilOp::Replace, StencilOp::Replace);
				m_shaderBackground = loadGraphicsShader("GenericGeometry.csl", outputLayoutWithDepth(), &setup);
			}

			{
				GraphicsRenderStatesSetup setup;
				setup.stencil(true);
				setup.stencilAll(CompareOp::Equal, StencilOp::Keep, StencilOp::Keep, StencilOp::Keep);
				m_shaderPreview = loadGraphicsShader("GenericGeometry.csl", outputLayoutWithDepth(), &setup);
			}

			for (uint32_t x = 0; x < NUM_OPS; ++x)
			{
				for (uint32_t y = 0; y < NUM_OPS; ++y)
				{
					GraphicsRenderStatesSetup setup;
					setup.stencil(true);
					setup.stencilAll(StencilFuncNames[subTestIndex()], StencilOpNames[x], StencilOp::Keep, StencilOpNames[y]);
					m_shaderTest[x][y] = loadGraphicsShader("GenericGeometry.csl", outputLayoutWithDepth(), &setup);
				}
			}
        }

        void RenderingTest_StencilModes::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView )
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));
            fb.depth.view(backBufferDepthView).clearDepth(1.0f).clearStencil(0);

            cmd.opBeingPass(outputLayoutWithDepth(), fb);

            auto numOptions  = ARRAY_COUNT(StencilOpNames);

            auto viewWidth = backBufferView->width() / numOptions;
            auto viewHeight = backBufferView->height() / numOptions;

            for (uint32_t x = 0; x < numOptions; ++x)
            {
                for (uint32_t y = 0; y < numOptions; ++y)
                {
                    cmd.opSetViewportRect(0, x * viewWidth, y * viewHeight, viewWidth, viewHeight);

                    // draw the test background
					for (uint32_t i = 0; i < 4; ++i)
					{
						cmd.opBindVertexBuffer("Simple3DVertex"_id, m_testBuffer);
						cmd.opSetStencilReferenceValue(1 + i);
						cmd.opDraw(m_shaderBackground, 6 * (1 + i), 6);
					}

                    // use the value 2 for the test
                    cmd.opSetStencilReferenceValue(2);

                    // draw the test
                    cmd.opDraw(m_shaderTest[x][y], 0, 6); // draw the test piece

                    // draw the visualization
                    for (uint32_t i = 0; i < 256; ++i)
                    {
                        cmd.opBindVertexBuffer("Simple3DVertex"_id, m_fillBuffer);
                        cmd.opSetStencilReferenceValue(i);
                        cmd.opDraw(m_shaderPreview, 6 * i, 6);
                    }
                }
            }

            cmd.opEndPass();
        }

    } // test
} // rendering