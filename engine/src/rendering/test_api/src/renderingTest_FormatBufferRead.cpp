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
        /// test of the storage buffer read in the shader
        class RenderingTest_FormatBufferRead : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_FormatBufferRead, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth) override final;

        private:
            BufferView m_vertexBuffer;
            BufferView m_extraBuffer;
            const ShaderLibrary* m_shaders;
            uint32_t m_vertexCount;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_FormatBufferRead);
        RTTI_METADATA(RenderingTestOrderMetadata).order(320);
        RTTI_END_TYPE();

        //---       

        namespace
        {
            struct TestConsts
            {
                float ElementSizeScale;
            };

            struct TestParams
            {
                ConstantsView Consts;
                BufferView ElementData;
            };
        }

        static void PrepareTestGeometry(float x, float y, float w, float h, uint32_t count, base::Array<Simple3DVertex>& outVertices, base::Array<base::Vector4>& outBufferData)
        {
            for (uint32_t py = 0; py <= count; ++py)
            {
                auto fx = py / (float)count;
                for (uint32_t px = 0; px <= count; ++px)
                {
                    auto fy = px / (float)count;

                    auto d = base::Vector2(fx - 0.5f, fy - 0.5f).length();
                    auto s = 0.2f + 0.8f + 0.8f * cos(d * TWOPI * 2.0f);

                    Simple3DVertex t;
                    t.VertexPosition.x = x + w * fx;
                    t.VertexPosition.y = y + h * fy;
                    t.VertexPosition.z = 0.5f;
                    outVertices.pushBack(t);

                    outBufferData.pushBack(base::Vector4(0, fy, fx, s));
                }
            }
        }

        void RenderingTest_FormatBufferRead::initialize()
        {
            m_shaders = loadShader("FormatBufferRead.csl");

            // generate test geometry
            base::Array<Simple3DVertex> vertices;
            base::Array<base::Vector4> bufferData;
            PrepareTestGeometry(-0.9f, -0.9f, 1.8f, 1.8f, 48, vertices, bufferData);
            m_vertexCount = vertices.size();

            // create vertex buffer
            {
                rendering::BufferCreationInfo info;
                info.allowVertex = true;
                info.size = vertices.dataSize();

                auto sourceData = CreateSourceData(vertices);
                m_vertexBuffer = createBuffer(info, &sourceData);
            }

            // create extra buffer
            {
                rendering::BufferCreationInfo info;
                info.allowShaderReads = true;
                info.allowUAV = true;
                info.size = bufferData.dataSize();

                auto sourceData = CreateSourceData(bufferData);
                m_extraBuffer = createBuffer(info, &sourceData);
            }
        }

        void RenderingTest_FormatBufferRead::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth)
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);

            TestConsts tempConsts;
            tempConsts.ElementSizeScale = 5.0f + 3.0f * cos(time);

            TestParams tempParams;
            tempParams.ElementData = m_extraBuffer;
            tempParams.Consts = cmd.opUploadConstants(tempConsts);
            cmd.opBindParametersInline("TestParams"_id, tempParams);

            cmd.opSetPrimitiveType(PrimitiveTopology::PointList);
            cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBuffer);
            cmd.opDraw(m_shaders, 0, m_vertexCount);

            cmd.opEndPass();
        }

    } // test
} // rendering