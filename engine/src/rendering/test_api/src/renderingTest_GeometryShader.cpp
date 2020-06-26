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

#include "rendering/driver/include/renderingDriver.h"
#include "rendering/driver/include/renderingCommandWriter.h"

namespace rendering
{
    namespace test
    {
        /// test of the geometry shader functionality
        class RenderingTest_GeometryShader : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_GeometryShader, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView ) override final;

        private:
            BufferView m_vertexBuffer;
            uint32_t m_vertexCount;
            const ShaderLibrary* m_shaders;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_GeometryShader);
            RTTI_METADATA(RenderingTestOrderMetadata).order(111);
        RTTI_END_TYPE();

        //---       

        static float RandOne()
        {
            return (float)rand() / (float)RAND_MAX;
        }

        static int RandRange(int range)
        {
            return (int)std::floor(range * RandOne());
        }

        static void PrepareTestGeometry(float x, float y, float w, float h, uint32_t count, base::Array<Simple3DVertex>& outVertices)
        {
            srand(0);

            base::Array<base::Color> colors;
            for (uint32_t i=0; i<10; ++i)
                colors.pushBack(base::Color(RandRange(127)+128, RandRange(127)+128, RandRange(127)+128, 255));

            for (uint32_t py = 0; py <= count; ++py)
            {
                auto fx = py / (float)count;
                for (uint32_t px = 0; px <= count; ++px)
                {
                    auto fy = px / (float)count;

                    auto d = base::Vector2(fx, fy).length();
                    auto s = 1.0f + 7.0f + 7.0f * cos(d * TWOPI * 2.0f);

                    Simple3DVertex t;
                    t.VertexPosition.x = x + w * fx;
                    t.VertexPosition.y = y + h * fy;
                    t.VertexPosition.z = 0.5f;
                    t.VertexUV.x = 3 + RandRange(4); // 3,4,5,6
                    t.VertexColor = colors[RandRange(colors.size())];
                    outVertices.pushBack(t);
                }
            }
        }

        void RenderingTest_GeometryShader::initialize()
        {
            m_shaders = loadShader("GeometryShader.csl");

            // generate test geometry
            base::Array<Simple3DVertex> vertices;
            PrepareTestGeometry(-0.9f, -0.9f, 1.8f, 1.8f, 32, vertices);
            m_vertexCount = vertices.size();

            // create vertex buffer
            { 
                rendering::BufferCreationInfo info;
                info.allowVertex = true;
                info.size = vertices.dataSize();

                auto sourceData = CreateSourceData(vertices);
                m_vertexBuffer = createBuffer(info, &sourceData);
            }
        }

        void RenderingTest_GeometryShader::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView )
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);

            cmd.opSetPrimitiveType(PrimitiveTopology::PointList);
            cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBuffer);
            cmd.opDraw(m_shaders, 0, m_vertexCount);
            cmd.opEndPass();
        }

    } // test
} // rendering