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
        /// cull mode test
        class RenderingTest_CullMode : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_CullMode, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView ) override final;

        private:
            BufferObjectPtr m_vertexBuffer;
            ShaderLibraryPtr m_shader;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_CullMode);
            RTTI_METADATA(RenderingTestOrderMetadata).order(30);
        RTTI_END_TYPE();

        //---       

        static void AddQuad(float x, float y, float w, float h, bool cwOrder, base::Color color, Simple3DVertex*& writePtr)
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

        void RenderingTest_CullMode::initialize()
        {
            // create vertex buffer with a single triangle
            {
                Simple3DVertex vertices[6*2*4*2];

                Simple3DVertex* writePtr = vertices;
                float x = -0.85f;
                float y = -0.5f;
                AddQuad(x, y, 0.2f, 0.8f, true, base::Color::RED, writePtr); x += 0.2f;
                AddQuad(x, y, 0.2f, 0.8f, false, base::Color::GREEN, writePtr); x += 0.3f;
                AddQuad(x, y, 0.2f, 0.8f, true, base::Color::RED, writePtr); x += 0.2f;
                AddQuad(x, y, 0.2f, 0.8f, false, base::Color::GREEN, writePtr); x += 0.3f;
                AddQuad(x, y, 0.2f, 0.8f, true, base::Color::RED, writePtr); x += 0.2f;
                AddQuad(x, y, 0.2f, 0.8f, false, base::Color::GREEN, writePtr); x += 0.3f;
                AddQuad(x, y, 0.2f, 0.8f, true, base::Color::RED, writePtr); x += 0.2f;
                AddQuad(x, y, 0.2f, 0.8f, false, base::Color::GREEN, writePtr); x += 0.3f;
                x = -0.85f;
                y = 0.5f;
                AddQuad(x, y, 0.2f, 0.8f, true, base::Color::RED, writePtr); x += 0.2f;
                AddQuad(x, y, 0.2f, 0.8f, false, base::Color::GREEN, writePtr); x += 0.3f;
                AddQuad(x, y, 0.2f, 0.8f, true, base::Color::RED, writePtr); x += 0.2f;
                AddQuad(x, y, 0.2f, 0.8f, false, base::Color::GREEN, writePtr); x += 0.3f;
                AddQuad(x, y, 0.2f, 0.8f, true, base::Color::RED, writePtr); x += 0.2f;
                AddQuad(x, y, 0.2f, 0.8f, false, base::Color::GREEN, writePtr); x += 0.3f;
                AddQuad(x, y, 0.2f, 0.8f, true, base::Color::RED, writePtr); x += 0.2f;
                AddQuad(x, y, 0.2f, 0.8f, false, base::Color::GREEN, writePtr); x += 0.3f;

                m_vertexBuffer = createVertexBuffer(sizeof(vertices), vertices);
            }

            m_shader = loadShader("SimpleVertexColor.csl");
        }

        void RenderingTest_CullMode::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView )
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);
            cmd.opSetPrimitiveType(PrimitiveTopology::TriangleList);
            cmd.opBindVertexBuffer("Simple3DVertex"_id, m_vertexBuffer);

            int pos = 0;
            rendering::FrontFace FACE_MODES[2] = { rendering::FrontFace::CW, rendering::FrontFace::CCW };
            for (uint32_t i = 0; i < ARRAY_COUNT(FACE_MODES); ++i)
            {
                rendering::CullMode CULL_MODES[4] = { rendering::CullMode::Disabled, rendering::CullMode::Front, rendering::CullMode::Back, rendering::CullMode::Both };
                for (uint32_t j = 0; j < ARRAY_COUNT(CULL_MODES); ++j)
                {
                    CullState state;
                    state.face = FACE_MODES[i];
                    state.mode = CULL_MODES[j];
                    cmd.opSetCullState(state);

                    cmd.opDraw(m_shader, 12 * pos, 12);
                    pos += 1;
                }
            }

            cmd.opEndPass();
        }

    } // test
} // rendering