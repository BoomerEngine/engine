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
#include "rendering/device/include/public.h"

namespace rendering
{
    namespace test
    {
        /// inline buffer test
        class RenderingTest_InlineBuffers : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_InlineBuffers, IRenderingTest);

        public:
            virtual void initialize() override final;            
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView ) override final;

            const ShaderLibrary* m_shaders;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_InlineBuffers);
        RTTI_METADATA(RenderingTestOrderMetadata).order(60);
        RTTI_END_TYPE();

        //---       

        void RenderingTest_InlineBuffers::initialize()
        {
            m_shaders = loadShader("GenericGeometry.csl");
        }

        static void GenerateTempGeometry(float x, float y, float w, float h, float time, base::Array<Simple3DVertex>& outVertices, base::Array<uint16_t>& outIndices)
        {
            static auto GRID_SIZE = 64;

            outVertices.reserve((1 + GRID_SIZE)*(1 + GRID_SIZE));
            outIndices.reserve(GRID_SIZE*GRID_SIZE*6);

            // generate the vertices for the magical bullshit
            float t = time;
            for (uint32_t py = 0; py <= GRID_SIZE; ++py)
            {
                float fy = py / (float)GRID_SIZE;
                for (uint32_t px = 0; px <= GRID_SIZE; ++px)
                {
                    float fx = px / (float)GRID_SIZE;

                    float cx = x + fx * w;
                    float cy = y + fy * h;

                    float d = base::Vector2(fx - 0.5f, fy - 0.5f).length();
                    float ro = d * 2.0f;
                    float rs = d * 10.0f;
                    float dx = std::cos(ro + rs*t) * (0.5f / GRID_SIZE);
                    float dy = std::sin(ro + rs*t) * (0.5f / GRID_SIZE);

                    Simple3DVertex v;
                    v.VertexPosition.x = cx + dx;
                    v.VertexPosition.y = cy + dy;
                    v.VertexPosition.z = 0.5f;

                    float co = d * 5.0f + t * 0.2f;
                    int cv = std::clamp<int>((int)(127.0f + 127.0f * cos(co)), 0, 255);
                    v.VertexColor = base::Color(cv, 255-cv, 0, 255);
                    outVertices.pushBack(v);
                }
            }


            // generate the indices for a rect grid
            for (uint16_t py = 0; py < GRID_SIZE; ++py)
            {
                uint16_t thisRow = (py) * (1 + GRID_SIZE);
                uint16_t nextRow = (py + 1) * (1 + GRID_SIZE);
                for (uint16_t px = 0; px < GRID_SIZE; ++px, thisRow += 1, nextRow += 1)
                {
                    outIndices.pushBack(thisRow);
                    outIndices.pushBack(thisRow+1);
                    outIndices.pushBack(nextRow+1);
                    outIndices.pushBack(thisRow);
                    outIndices.pushBack(nextRow+1);
                    outIndices.pushBack(nextRow);
                }
            }
        }

        void RenderingTest_InlineBuffers::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView )
        {
            base::Array<Simple3DVertex> tempVertices;
            base::Array<uint16_t> tempIndices;
            GenerateTempGeometry(-0.9f, -0.9f, 1.8f, 1.8f, time, tempVertices, tempIndices);

            BufferObjectPtr vertexBuffer;
            {
                BufferCreationInfo data;
                data.size = tempVertices.dataSize();
                data.allowVertex = true;

                SourceData source;
                source.data = tempVertices.createBuffer();
                source.offset = 0;
                source.size = tempVertices.dataSize();

                vertexBuffer = device()->createBuffer(data, &source);
            }

            BufferObjectPtr indexBuffer;
            {
                BufferCreationInfo data;
                data.size = tempIndices.dataSize();
                data.allowIndex = true;

                SourceData source;
                source.data = tempIndices.createBuffer();
                source.offset = 0;
                source.size = tempIndices.dataSize();

                indexBuffer = device()->createBuffer(data, &source);
            }

            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);
            cmd.opSetPrimitiveType(PrimitiveTopology::TriangleList);
            cmd.opSetFillState(PolygonMode::Line);

            cmd.opBindVertexBuffer("Simple3DVertex"_id,  vertexBuffer->view());
            cmd.opBindIndexBuffer(indexBuffer->view(), ImageFormat::R16_UINT);
            cmd.opDrawIndexed(m_shaders, 0, 0, tempIndices.size());

            cmd.opEndPass();
        }

    } // test
} // rendering