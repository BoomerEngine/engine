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

namespace rendering
{
    namespace test
    {
        /// test of a multiple buffer updates of the same buffer within one frame
        class RenderingTest_MultipleBufferUpdates : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_MultipleBufferUpdates, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView) override final;

        private:
            static const uint32_t IMAGE_SIZE = 8;

            BufferObjectPtr m_vertexBuffer;
			BufferObjectPtr m_vertexColorBuffer;

            GraphicsPipelineObjectPtr m_shaders;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_MultipleBufferUpdates);
        RTTI_METADATA(RenderingTestOrderMetadata).order(2010);
        RTTI_END_TYPE();

        //---       

        static void PrepareTestGeometry(float x, float y, float r, float phase, base::Vector2* outVertices)
        {
            auto NUM_SEGS = 3;

            auto delta = TWOPI / NUM_SEGS;
            for (uint32_t i=0; i<NUM_SEGS; i++)
            {
                float a = i * delta + phase;
                float b = (i+1) * delta + phase;

                auto ax = x + std::cos(a) * r;
                auto ay = y + std::sin(a) * r;
                auto bx = x + std::cos(b) * r;
                auto by = y + std::sin(b) * r;

                outVertices[i*2+0] = base::Vector2(ax,ay);
                outVertices[i*2+1] = base::Vector2(bx,by);
            }
        }

        void RenderingTest_MultipleBufferUpdates::initialize()
        {
            m_vertexBuffer = createVertexBuffer(sizeof(base::Vector2) * 6, nullptr);
            m_vertexColorBuffer = createVertexBuffer(sizeof(base::Color) * 6, nullptr);                

			GraphicsRenderStatesSetup setup;
			setup.primitiveTopology(PrimitiveTopology::LineList);

            m_shaders = loadGraphicsShader("GenericGeometryTwoStreams.csl", &setup);
        }

        void RenderingTest_MultipleBufferUpdates::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView)
        {
            auto GRID_SIZE = 16;

            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);

            cmd.opBindVertexBuffer("VertexStream0"_id, m_vertexBuffer);
            cmd.opBindVertexBuffer("VertexStream1"_id, m_vertexColorBuffer);

            // draw the grid
            base::FastRandState rand;
            for (uint32_t y = 0; y < GRID_SIZE; ++y)
            {
                for (uint32_t x = 0; x < GRID_SIZE; ++x)
                {
                    // get position in grid
                    auto cx = -0.9f + 1.8f * ((x + 0.5f) / (float)GRID_SIZE);
                    auto cy = -0.9f + 1.8f * ((y + 0.5f) / (float)GRID_SIZE);
                    auto r = 0.5f / (float)GRID_SIZE;
                    auto a = rand.unit() * TWOPI + time;

                    // prepare positions
					{
						cmd.opTransitionLayout(m_vertexBuffer, ResourceLayout::VertexBuffer, ResourceLayout::CopyDest);
                        auto* updateVertices = cmd.opUpdateDynamicBufferPtrN<base::Vector2>(m_vertexBuffer, 0, 6);
                        PrepareTestGeometry(cx, cy, r, a, updateVertices);
						cmd.opTransitionLayout(m_vertexBuffer, ResourceLayout::CopyDest, ResourceLayout::VertexBuffer);
                    }

                    // prepare colors
                    {
						cmd.opTransitionLayout(m_vertexColorBuffer, ResourceLayout::VertexBuffer, ResourceLayout::CopyDest);
                        const auto randomColor = base::Color(rand.next(), rand.next(), rand.next());
                        auto* updateColors = cmd.opUpdateDynamicBufferPtrN<base::Color>(m_vertexColorBuffer, 0, 6);
                        for (uint32_t i = 0; i < 6; ++i)
                            updateColors[i] = randomColor;
						cmd.opTransitionLayout(m_vertexColorBuffer, ResourceLayout::CopyDest, ResourceLayout::VertexBuffer);
                    }

                    // draw the quad
                    cmd.opDraw(m_shaders, 0, 6); // triangle
                }
            }

            // exit pass
            cmd.opEndPass();
        }

    } // test
} // rendering
