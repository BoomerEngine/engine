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

        namespace
        {
            struct TestConsts
            {
                static const uint32_t ELEMENTS_PER_SIDE = 32;
                static const uint32_t MAX_BUFFER_ELEMENTS = (ELEMENTS_PER_SIDE + 1) * (ELEMENTS_PER_SIDE + 1);

                struct TestElement
                {
                    float m_size;
                    base::Vector4 m_color;
                };

                float TestSizeScale;
                TestElement TestElements[MAX_BUFFER_ELEMENTS];
            };

            struct TestParams
            {
                ConstantsView Consts;
            };
        }

        /// test of the custom uniform buffer read in the shader
        class RenderingTest_UniformBufferRead : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_UniformBufferRead, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth) override final;

        private:
            BufferView m_vertexBuffer;
            const ShaderLibrary* m_shaders;
            TestConsts m_data;
            uint32_t m_vertexCount;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_UniformBufferRead);
            RTTI_METADATA(RenderingTestOrderMetadata).order(190);
        RTTI_END_TYPE();

        //---       

        static void PrepareTestGeometry(float x, float y, float w, float h, uint32_t count, base::Array<Simple3DVertex>& outVertices, TestConsts& outBufferData)
        {
            uint32_t index = 0;
            for (uint32_t py = 0; py <= count; ++py)
            {
                auto fy = py / (float)count;
                for (uint32_t px = 0; px <= count; ++px)
                {
                    auto fx = px / (float)count;

                    auto d = base::Vector2(fx - 1.5f, fy - 1.5f).length();
                    auto s = 1.0f + 7.0f + 7.0f * sin(d * TWOPI * 3.0f);

                    Simple3DVertex t;
                    t.VertexPosition.x = x + w * fx;
                    t.VertexPosition.y = y + h * fy;
                    t.VertexPosition.z = 0.5f;
                    outVertices.pushBack(t);

                    if (index < ARRAY_COUNT(outBufferData.TestElements))
                    {
                        auto& outElem = outBufferData.TestElements[index++];
                        outElem.m_size = s;
                        outElem.m_color = base::Vector4(fy, 0, fx, 1);
                    }
                }
            }
        }

        void RenderingTest_UniformBufferRead::initialize()
        {
            m_data.TestSizeScale = 1.0f;

            {
                base::Array<Simple3DVertex> vertices;
                PrepareTestGeometry(-0.9f, -0.9f, 1.8f, 1.8f, TestConsts::ELEMENTS_PER_SIDE, vertices, m_data);
                m_vertexCount = vertices.size();
                m_vertexBuffer = createVertexBuffer(vertices);
            }

            m_shaders = loadShader("UniformBufferRead.csl");
        }

        void RenderingTest_UniformBufferRead::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth)
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);
            
            TestParams params;
            params.Consts = cmd.opUploadConstants(m_data);
            cmd.opBindParametersInline("TestParams"_id, params);

            cmd.opSetPrimitiveType(PrimitiveTopology::PointList);
            cmd.opBindVertexBuffer("Simple3DVertex"_id, m_vertexBuffer);
            cmd.opDraw(m_shaders, 0, m_vertexCount);

            cmd.opEndPass();
        }

    } // test
} // rendering