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
#include "rendering/device/include/renderingBuffer.h"

namespace rendering
{
    namespace test
    {
        /// basic blending mode test
        class RenderingTest_BasicBlendingModes : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_BasicBlendingModes, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView ) override final;

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

            base::StringBuf m_blendMode;
            ShaderLibraryPtr m_shader;
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

                vertices[0].set(-0.9f, -0.9f, 0.5f, 0.0f, 0.0f, base::Color(128, 128, 128, 255));
                vertices[1].set(+0.9f, -0.9f, 0.5f, 0.0f, 0.0f, base::Color(128, 128, 128, 0));
                vertices[2].set(+0.9f, +0.9f, 0.5f, 0.0f, 0.0f, base::Color(255, 255, 255, 0));
                vertices[3].set(-0.9f, -0.9f, 0.5f, 0.0f, 0.0f, base::Color(128, 128, 128, 255));
                vertices[4].set(+0.9f, +0.9f, 0.5f, 0.0f, 0.0f, base::Color(255, 255, 255, 0));
                vertices[5].set(-0.9f, +0.9f, 0.5f, 0.0f, 0.0f, base::Color(255, 255, 255, 255));

                m_backgroundBuffer = createVertexBuffer(sizeof(vertices), vertices);
            }

            // allocate test
            {
                Simple3DVertex vertices[12];

                // quad with dest alpha
                vertices[0].set(-0.9f, -0.7f, 0.5f, 0.0f, 0.0f, base::Color(255, 255, 255, 255));
                vertices[1].set(+0.9f, -0.7f, 0.5f, 0.0f, 0.0f, base::Color(0, 255, 0, 255));
                vertices[2].set(+0.9f, -0.1f, 0.5f, 0.0f, 0.0f, base::Color(255, 0, 0, 0));
                vertices[3].set(-0.9f, -0.7f, 0.5f, 0.0f, 0.0f, base::Color(255, 255, 255, 255));
                vertices[4].set(+0.9f, -0.1f, 0.5f, 0.0f, 0.0f, base::Color(255, 0, 0, 0));
                vertices[5].set(-0.9f, -0.1f, 0.5f, 0.0f, 0.0f, base::Color(0, 0, 255, 0));

                vertices[6].set(-0.9f, +0.1f, 0.5f, 0.0f, 0.0f, base::Color(0, 0, 0, 0));
                vertices[7].set(+0.9f, +0.1f, 0.5f, 0.0f, 0.0f, base::Color(255, 0, 255, 0));
                vertices[8].set(+0.9f, +0.7f, 0.5f, 0.0f, 0.0f, base::Color(0, 255, 255, 255));
                vertices[9].set(-0.9f, +0.1f, 0.5f, 0.0f, 0.0f, base::Color(0, 0, 0, 0));
                vertices[10].set(+0.9f, +0.7f, 0.5f, 0.0f, 0.0f, base::Color(0, 255, 255, 255));
                vertices[11].set(-0.9f, +0.7f, 0.5f, 0.0f, 0.0f, base::Color(255, 255, 0, 255));

                m_testBuffer = createVertexBuffer(sizeof(vertices), vertices);
            }

            // load shaders
            m_shader = loadShader("GenericGeometry.csl");
        }

        void RenderingTest_BasicBlendingModes::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView )
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);

            auto numBlendModes  = ARRAY_COUNT(BlendModesNames);
            auto viewWidth = backBufferView->width() / numBlendModes;
            auto viewHeight = backBufferView->height() / numBlendModes;

            auto blendOp = BlendFuncNames[subTestIndex()];

            for (uint32_t y = 0; y < numBlendModes; ++y)
            {
                for (uint32_t x = 0; x < numBlendModes; ++x)
                {
                    cmd.opSetViewportRect(0, x * viewWidth, y * viewHeight, viewWidth, viewHeight);

                    // background
                    {
                        cmd.opSetBlendState(0);
                        cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_backgroundBuffer);
                        cmd.opDraw(m_shader, 0, m_backgroundBuffer->size() / sizeof(Simple3DVertex));
                    }

                    // test
                    {
                        auto blendSrc = BlendModesNames[x];
                        auto blendDest = BlendModesNames[y];
                        cmd.opSetBlendState(0, blendOp, blendSrc, blendDest);
                        cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_testBuffer);
                        cmd.opDraw(m_shader, 0, m_testBuffer->size() / sizeof(Simple3DVertex));
                    }
                }
            }

            cmd.opEndPass();
        }

        //--

    } // test
} // rendering