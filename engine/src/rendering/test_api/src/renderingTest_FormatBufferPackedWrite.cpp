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
        /// test of the non-atomic writes to the texel buffer
        class RenderingTest_FormatBufferPackedWrite : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_FormatBufferPackedWrite, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferViewDepth) override final;

        private:
            static const auto SIDE_RESOLUTION = 128;

            BufferObjectPtr m_tempBuffer;
			BufferViewPtr m_tempBufferSRV;
			BufferWritableViewPtr m_tempBufferUAV;

            BufferObjectPtr m_vertexBuffer;
            uint32_t m_vertexCount;

            GraphicsPipelineObjectPtr m_shaderGenerate;
            GraphicsPipelineObjectPtr m_shaderDraw;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_FormatBufferPackedWrite);
			RTTI_METADATA(RenderingTestOrderMetadata).order(310);
        RTTI_END_TYPE();

        //---       

        static void PrepareTestGeometry(float x, float y, float w, float h, base::Array<Simple3DVertex>& outVertices)
        {
            outVertices.pushBack(Simple3DVertex(x, y, 0.5f, 0.0f, 0.0f));
            outVertices.pushBack(Simple3DVertex(x + w, y, 0.5f, 1.0f, 0.0f));
            outVertices.pushBack(Simple3DVertex(x + w, y + h, 0.5f, 1.0f, 1.0f));

            outVertices.pushBack(Simple3DVertex(x, y, 0.5f, 0.0f, 0.0f));
            outVertices.pushBack(Simple3DVertex(x + w, y + h, 0.5f, 1.0f, 1.0f));
            outVertices.pushBack(Simple3DVertex(x, y + h, 0.5f, 0.0f, 1.0f));
        }

        void RenderingTest_FormatBufferPackedWrite::initialize()
        {
            // generate test geometry
            base::Array<Simple3DVertex> vertices;
            PrepareTestGeometry(-0.9f, -0.9f, 1.8f, 1.8f, vertices);

            m_vertexBuffer = createVertexBuffer(vertices.dataSize(), vertices.data());

            m_shaderGenerate = loadGraphicsShader("FormatBufferPackedWriteGenerate.csl", outputLayoutNoDepth());
            m_shaderDraw = loadGraphicsShader("FormatBufferPackedWriteTest.csl", outputLayoutNoDepth());

            m_tempBuffer = createStorageBuffer(4 * SIDE_RESOLUTION * SIDE_RESOLUTION);
			m_tempBufferSRV = m_tempBuffer->createView(ImageFormat::RG16F);
			m_tempBufferUAV = m_tempBuffer->createWritableView(ImageFormat::RG16F);
        }

        void RenderingTest_FormatBufferPackedWrite::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferViewDepth)
        {
			struct
			{
				float TimeOffset;
				uint32_t SideCount;
			} consts;

			consts.TimeOffset = time;
			consts.SideCount = SIDE_RESOLUTION;

            //--

            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(outputLayoutNoDepth(), fb);

            {
				DescriptorEntry desc[2];
				desc[0].constants(consts);
				desc[1] = m_tempBufferUAV;
				cmd.opBindDescriptor("TestParams"_id, desc);
                cmd.opDraw(m_shaderGenerate, 0, SIDE_RESOLUTION * SIDE_RESOLUTION);
            }

			cmd.opTransitionLayout(m_tempBuffer, ResourceLayout::UAV, ResourceLayout::ShaderResource);

            {
				DescriptorEntry desc[2];
				desc[0].constants(consts);
				desc[1] = m_tempBufferSRV;
				cmd.opBindDescriptor("TestParams"_id, desc);

                cmd.opBindVertexBuffer("Simple3DVertex"_id, m_vertexBuffer);
                cmd.opDraw(m_shaderDraw, 0, 6);
            }

            cmd.opEndPass();
        }

    } // test
} // rendering;