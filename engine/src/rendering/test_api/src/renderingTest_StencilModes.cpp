/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/

#include "build.h"
#include "renderingTest.h"
#include "renderingTestShared.h"

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
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView) override final;

        private:
            BufferView m_testBuffer;
            BufferView m_fillBuffer;
            ImageView m_depthBuffer;

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

            const ShaderLibrary* m_shader;
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

                rendering::BufferCreationInfo info;
                info.allowVertex = true;
                info.size = sizeof(vertices);

                auto sourceData  = CreateSourceDataRaw(vertices);
                m_testBuffer = createBuffer(info, &sourceData);
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
                {
                    AddQuad(vertices + 6 * i, -1, -1, 1, 1, base::Color::FromVectorLinear(colors[i]));
                }

                rendering::BufferCreationInfo info;
                info.allowVertex = true;;
                info.size = sizeof(vertices);

                auto sourceData  = CreateSourceDataRaw(vertices);
                m_fillBuffer = createBuffer(info, &sourceData);
            }

            m_shader = loadShader("GenericGeometry.csl");
        }

        void RenderingTest_StencilModes::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView )
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));
            fb.depth.view(backBufferDepthView).clearDepth(1.0f).clearStencil(0);

            cmd.opBeingPass(fb);

            auto numOptions  = ARRAY_COUNT(StencilOpNames);

            auto viewWidth = backBufferView.width() / numOptions;
            auto viewHeight = backBufferView.height() / numOptions;

            for (uint32_t x = 0; x < numOptions; ++x)
            {
                for (uint32_t y = 0; y < numOptions; ++y)
                {
                    cmd.opSetViewportRect(0, x * viewWidth, y * viewHeight, viewWidth, viewHeight);

                    // draw the test background
                    {
                        cmd.opSetStencilState(CompareOp::Always, StencilOp::Replace, StencilOp::Replace, StencilOp::Replace);
                        for (uint32_t i = 0; i < 4; ++i)
                        {
                            cmd.opBindVertexBuffer("Simple3DVertex"_id, m_testBuffer);
                            cmd.opSetStencilReferenceValue(1 + i);
                            cmd.opDraw(m_shader, 6 * (1 + i), 6);
                        }
                    }

                    // use the value 2 for the test
                    cmd.opSetStencilReferenceValue(2);

                    // draw the test
                    {
                        auto func = StencilFuncNames[subTestIndex()];
                        auto pass = StencilOpNames[x];
                        auto fail = StencilOpNames[y];
                        cmd.opSetStencilState(func, fail, StencilOp::Keep, pass);
                        cmd.opDraw(m_shader, 0, 6); // draw the test piece
                    }

                    // draw the visualization
                    {
                        cmd.opSetStencilState(CompareOp::Equal, StencilOp::Keep, StencilOp::Keep, StencilOp::Keep);
                        for (uint32_t i = 0; i < 256; ++i)
                        {
                            cmd.opBindVertexBuffer("Simple3DVertex"_id, m_fillBuffer);
                            cmd.opSetStencilReferenceValue(i);
                            cmd.opDraw(m_shader, 6 * i, 6);
                        }
                    }
                }
            }

            cmd.opEndPass();
        }

    } // test
} // rendering