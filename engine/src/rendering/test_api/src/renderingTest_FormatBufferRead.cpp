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
        /// test of the storage buffer read in the shader
        class RenderingTest_FormatBufferRead : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_FormatBufferRead, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth) override final;

        private:
            BufferObjectPtr m_vertexBuffer;

			BufferObjectPtr m_extraBuffer;
			BufferViewPtr m_extraBufferSRV;

            GraphicsPipelineObjectPtr m_shaders;
            uint32_t m_vertexCount;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_FormatBufferRead);
        RTTI_METADATA(RenderingTestOrderMetadata).order(320);
        RTTI_END_TYPE();

        //---       

		void PrepareTestGeometry(float x, float y, float w, float h, uint32_t count, base::Array<Simple3DVertex>& outVertices, base::Array<base::Vector4>& outBufferData)
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
            m_shaders = loadGraphicsShader("FormatBufferRead.csl", outputLayoutNoDepth());

            base::Array<Simple3DVertex> vertices;
            base::Array<base::Vector4> bufferData;
            PrepareTestGeometry(-0.9f, -0.9f, 1.8f, 1.8f, 48, vertices, bufferData);
            m_vertexCount = vertices.size();
            m_vertexBuffer = createVertexBuffer(vertices);

            // create extra buffer
            {
                rendering::BufferCreationInfo info;
                info.allowShaderReads = true;
                info.size = bufferData.dataSize();

                auto sourceData = CreateSourceData(bufferData);
                m_extraBuffer = createBuffer(info, sourceData);
				m_extraBufferSRV = m_extraBuffer->createView(ImageFormat::RGBA32F);
            }
        }

        void RenderingTest_FormatBufferRead::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth)
        {
			FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(outputLayoutNoDepth(), fb);

			DescriptorEntry tempParams[2];
			tempParams[0].constants<float>(5.0f + 3.0f * cos(time));
			tempParams[1] = m_extraBufferSRV;
            cmd.opBindDescriptor("TestParams"_id, tempParams);

            cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBuffer);
            cmd.opDraw(m_shaders, 0, m_vertexCount);

            cmd.opEndPass();
        }

    } // test
} // rendering