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
        /// test of the compute shader access to storage image
        class RenderingTest_ComputeFillFormatBuffer : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_ComputeFillFormatBuffer, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth) override final;

        private:
            static const auto SIDE_RESOLUTION = 512;

            ShaderLibraryPtr m_shaderGenerate;
            ShaderLibraryPtr m_shaderDraw;

            BufferObjectPtr m_vertexBuffer;

			BufferObjectPtr m_texelBuffer;
			BufferWritableViewPtr m_texelBufferUAV;
			BufferViewPtr m_texelBufferSRV;

            uint32_t m_vertexCount;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_ComputeFillFormatBuffer);
            RTTI_METADATA(RenderingTestOrderMetadata).order(2200);
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

        void RenderingTest_ComputeFillFormatBuffer::initialize()
        {
            {
                base::Array<Simple3DVertex> vertices;
                PrepareTestGeometry(-0.9f, -0.9f, 1.8f, 1.8f, vertices);
                m_vertexBuffer = createVertexBuffer(vertices);
            }

            m_texelBuffer = createStorageBuffer(SIDE_RESOLUTION * SIDE_RESOLUTION * 4);
			m_texelBufferSRV = m_texelBuffer->createView(ImageFormat::RGBA8_UNORM);
			m_texelBufferUAV = m_texelBuffer->createWritableView(ImageFormat::RGBA8_UNORM);

            m_shaderGenerate = loadShader("ComputeFillFormatBufferGenerate.csl");
            m_shaderDraw = loadShader("ComputeFillFormatBufferTest.csl");
        }

        void RenderingTest_ComputeFillFormatBuffer::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth)
        {
			struct {
				uint32_t SideCount;
				uint32_t FrameIndex;
			} params;

			params.SideCount = SIDE_RESOLUTION;
			params.FrameIndex = (int)(time * 60.0f);

            {
				DescriptorEntry desc[2];
				desc[0].constants(params);
				desc[1] = m_texelBufferUAV;
                cmd.opBindDescriptor("TestParamsWrite"_id, desc);
                cmd.opDispatch(m_shaderGenerate, SIDE_RESOLUTION / 8, SIDE_RESOLUTION / 8);
            }

			cmd.opTransitionLayout(m_texelBuffer, ResourceLayout::UAV, ResourceLayout::ShaderResource);   

            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);            

            {
				DescriptorEntry desc[2];
				desc[0].constants(params);
				desc[1] = m_texelBufferSRV;
				cmd.opBindDescriptor("TestParamsRead"_id, desc);

                cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBuffer);
                cmd.opDraw(m_shaderDraw, 0, 6);

                cmd.opEndPass();
            }
        }

    } // test
} // rendering;