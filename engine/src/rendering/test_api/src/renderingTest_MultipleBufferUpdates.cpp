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
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView) override final;

        private:
            static const uint32_t IMAGE_SIZE = 8;

            BufferView m_vertexBuffer;
            const ShaderLibrary* m_shaders;
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
            // create vertex buffer
            {
                rendering::BufferCreationInfo info;
                info.allowVertex = true;
                info.allowDynamicUpdate = true;
                info.size = sizeof(base::Vector2) * 6;

                m_vertexBuffer = createBuffer(info);
            }

            m_shaders = loadShader("GenericGeometryTwoStreams.csl");
        }

        void RenderingTest_MultipleBufferUpdates::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView)
        {
            auto GRID_SIZE = 16;

            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);

            // create temp buffer for color
            BufferViewFlags flags = BufferViewFlags() | BufferViewFlag::Vertex | BufferViewFlag::ShaderReadable | BufferViewFlag::Dynamic;
            TransientBufferView colorBuffer(flags, sizeof(base::Color) * 6);
            cmd.opAllocTransientBuffer(colorBuffer);

            // bind the same buffer for the whole draw
            cmd.opBindVertexBuffer("VertexStream0"_id, m_vertexBuffer);
            cmd.opBindVertexBuffer("VertexStream1"_id, colorBuffer);

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
                    auto a = base::RandOne(rand) * TWOPI + time;

                    // prepare positions
                    base::Vector2 updateVertices[6];
                    PrepareTestGeometry(cx, cy, r, a, updateVertices);

                    // prepare colors
                    base::Color updateColors[6];
                    base::Color randomColor = base::Color(base::Rand(rand), base::Rand(rand), base::Rand(rand));
                    for (uint32_t i=0; i<ARRAY_COUNT(updateColors); ++i)
                        updateColors[i] = randomColor;

                    // send update to buffers
                    cmd.opSetPrimitiveType(PrimitiveTopology::LineList);
                    cmd.opUpdateDynamicBuffer(m_vertexBuffer, 0, sizeof(updateVertices), updateVertices);
                    cmd.opUpdateDynamicBuffer(colorBuffer, 0, sizeof(updateColors), updateColors);

                    // draw the quad
                    cmd.opDraw(m_shaders, 0, 6); // triangle
                }
            }

            // exit pass
            cmd.opEndPass();
        }

    } // test
} // rendering
