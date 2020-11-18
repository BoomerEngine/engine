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
        /// test of a dynamic buffers
        class RenderingTest_DynamicBufferUpdate : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_DynamicBufferUpdate, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView) override final;

        private:
            static const uint32_t MAX_VERTICES = 1024;

            BufferView m_vertexBuffer;
            const ShaderLibrary* m_shaders;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_DynamicBufferUpdate);
            RTTI_METADATA(RenderingTestOrderMetadata).order(2020);
        RTTI_END_TYPE();

        //---       

        static void PrepareTestGeometry(uint32_t count, float x, float y, float r, float t, base::Vector2* outVertices, base::Color* outColors)
        {
            for (uint32_t i=0; i<count; ++i)
            {
                auto frac = (float)i / (float)(count-1);

                // delta
                auto dx = cos(TWOPI * frac);
                auto dy = sin(TWOPI * frac);
                auto displ = r;

                // displacement
                auto speed = 20.0f + 19.0f * sin(t * 0.1f);
                auto displPhase = (cos(frac * speed) + 1.0f);
                displ += displPhase * r;

                // noise
                float noise = rand() / (float)RAND_MAX;
                displ += noise * 0.1f;

                // create vertex
                outVertices[i].x = x + dx * displ;
                outVertices[i].y = y + dy * displ;

                // calc color
                outColors[i] = base::Lerp(base::Color(base::Color::RED), base::Color(base::Color::GREEN), (0.5f + 0.5f*displPhase));
            }
        }

        void RenderingTest_DynamicBufferUpdate::initialize()
        {
            // create vertex buffer
            {
                rendering::BufferCreationInfo info;
                info.allowVertex = true;
                info.allowDynamicUpdate = true;
                info.size = sizeof(base::Vector2) * MAX_VERTICES;

                m_vertexBuffer = createBuffer(info);
            }

            m_shaders = loadShader("GenericGeometryTwoStreams.csl");
        }

        void RenderingTest_DynamicBufferUpdate::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView)
        {
            // create temp buffer for color
            BufferViewFlags flags = BufferViewFlags() | BufferViewFlag::Vertex | BufferViewFlag::ShaderReadable | BufferViewFlag::Dynamic;
            TransientBufferView colorBuffer(flags, sizeof(base::Color) * MAX_VERTICES);
            cmd.opAllocTransientBuffer(colorBuffer);

            // prepare some dynamic geometry
            base::Vector2 dynamicVerticesData[MAX_VERTICES];
            base::Color dynamicColorsData[MAX_VERTICES];
            PrepareTestGeometry(MAX_VERTICES, 0.0f, 0.0f, 0.33f, time, dynamicVerticesData, dynamicColorsData);

            // update vertex data
            cmd.opUpdateDynamicBuffer(m_vertexBuffer, 0, sizeof(dynamicVerticesData), dynamicVerticesData);
            cmd.opUpdateDynamicBuffer(colorBuffer, 0, sizeof(dynamicColorsData), dynamicColorsData);

            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);
            cmd.opSetPrimitiveType(PrimitiveTopology::LineStrip);
            cmd.opBindVertexBuffer("VertexStream0"_id,  m_vertexBuffer);
            cmd.opBindVertexBuffer("VertexStream1"_id, colorBuffer);
            cmd.opDraw(m_shaders, 0, MAX_VERTICES); // loop
            cmd.opEndPass();
        }

    } // test
} // rendering
