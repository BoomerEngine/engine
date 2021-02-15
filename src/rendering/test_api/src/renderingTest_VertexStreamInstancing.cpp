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
        /// test of vertex stream based instancing
        class RenderingTest_VertexStreamInstancing : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_VertexStreamInstancing, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth) override final;

        private:
            GraphicsPipelineObjectPtr m_shaders;
            BufferObjectPtr m_vertexBuffer;
			BufferObjectPtr m_instanceBuffer;

            uint32_t m_vertexCount;
            uint16_t m_instanceCount;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_VertexStreamInstancing);
        RTTI_METADATA(RenderingTestOrderMetadata).order(180);
        RTTI_END_TYPE();

        //---       

        struct InstanceDataTest
        {
            base::Vector2 InstanceOffset;
            base::Color InstanceColor;
            float InstanceScale;
        };

        static void PrepareInstanceBuffer(float x, float y, float w, float h, uint32_t count, base::Array<InstanceDataTest>& outInstances)
        {
            for (uint32_t py = 0; py < count; ++py)
            {
                for (uint32_t px = 0; px < count; ++px)
                {
                    InstanceDataTest t;
                    t.InstanceOffset.x = x + w * (px / (float)(count - 1));
                    t.InstanceOffset.y = y + h * (py / (float)(count - 1));
                    t.InstanceScale = 1.0f / (float)count;
                    t.InstanceColor = base::Color::FromVectorLinear(base::Vector4(px / (float)(count - 1), py / (float)(count - 1), 0, 1));
                    outInstances.pushBack(t);                   
                }
            }
        }

        static void PrepareTestGeometry(base::Array<Simple3DVertex>& outVertices)
        {
            static const uint32_t NUM_SEGMENTS = 64;

            float s = 1.0f / (float)NUM_SEGMENTS;
            for (uint32_t i = 0; i < NUM_SEGMENTS; ++i)
            {
                float a = i * s;
                float b = (i+1) * s;

                float ax = cos(a * TWOPI);
                float ay = sin(a * TWOPI);
                float bx = cos(b * TWOPI);
                float by = sin(b * TWOPI);

                outVertices.pushBack(Simple3DVertex(ax, ay, 0.5f));
                outVertices.pushBack(Simple3DVertex(bx, by, 0.5f));
                outVertices.pushBack(Simple3DVertex(0, 0, 0.5f));
            }
        }

        void RenderingTest_VertexStreamInstancing::initialize()
        {
            static auto NUM_INSTANCES_PER_SIDE = 32;

            // generate test geometry
            {
                base::Array<Simple3DVertex> vertices;
                PrepareTestGeometry(vertices);
                m_vertexCount = vertices.size();
                m_vertexBuffer = createVertexBuffer(vertices);
            }

            // generate test instances
            {
                base::Array<InstanceDataTest> instances;
                PrepareInstanceBuffer(-0.9f, -0.9f, 1.8f, 1.8f, NUM_INSTANCES_PER_SIDE, instances);
                m_instanceCount = range_cast<uint16_t>(instances.size());
                m_instanceBuffer = createVertexBuffer(instances);
            }

            m_shaders = loadGraphicsShader("VertexStreamInstancing.csl");
        }

        void RenderingTest_VertexStreamInstancing::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth)
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);

            cmd.opBindVertexBuffer("Simple3DVertex"_id, m_vertexBuffer);
            cmd.opBindVertexBuffer("InstanceDataTest"_id, m_instanceBuffer);
            cmd.opDrawInstanced(m_shaders, 0, m_vertexCount, 0, m_instanceCount);
            cmd.opEndPass();
        }

    } // test
} // rendering