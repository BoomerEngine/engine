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
        /// test of the texture buffer read in the shader
        class RenderingTest_BufferCopyRect : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_BufferCopyRect, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferViewDepth) override final;

        private:
            GraphicsPipelineObjectPtr m_shaders;

			BufferObjectPtr m_sourceBuffer;
			BufferObjectPtr m_sourceBuffer2;

			BufferObjectPtr m_extraBuffer;
			BufferViewPtr m_extraBufferSRV;

            uint32_t m_sideCount;

			base::FastRandState m_rand;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_BufferCopyRect);
        RTTI_METADATA(RenderingTestOrderMetadata).order(336);
        RTTI_END_TYPE();

        //---

		static void PrepareTestTexels(uint32_t sideCount, base::Array<base::Color>& outColors)
		{
			outColors.reserve(sideCount * sideCount);
			for (uint32_t y = 0; y < sideCount; y++)
			{
				float fy = y / (float)(sideCount - 1);
				for (uint32_t x = 0; x < sideCount; x++)
				{
					float fx = x / (float)(sideCount - 1);

					float f = 0.5f + cos(fy * 4) * sin(fx * 4);
					outColors.pushBack(base::Color::FromVectorLinear(base::Vector4(0.0f, f, 1.0f - f, 1.0f)));
				}
			}
		}

		static void PrepareTestTexels2(uint32_t sideCount, base::Array<base::Color>& outColors)
		{
			outColors.reserve(sideCount * sideCount);
			for (uint32_t y = 0; y < sideCount; y++)
			{
				float fy = y / (float)(sideCount - 1);
				for (uint32_t x = 0; x < sideCount; x++)
				{
					float fx = x / (float)(sideCount - 1);

					float f = 0.5f + cos(fy * 8) * sin(fx * 10);
					outColors.pushBack(base::Color::FromVectorLinear(base::Vector4(1.0f - f, 0, f, 1.0f)));
				}
			}
		}

		void RenderingTest_BufferCopyRect::initialize()
        {
            m_sideCount = 512;

			// generate source buffer
			{
				base::Array<base::Color> colors;
				PrepareTestTexels(m_sideCount, colors);

				rendering::BufferCreationInfo info;
				info.allowShaderReads = true;
				info.allowCopies = true;
				info.size = colors.dataSize();

				auto sourceData = CreateSourceData(colors);
				m_sourceBuffer = createBuffer(info, sourceData);
			}

			// generate second source buffer
			{
				base::Array<base::Color> colors;
				PrepareTestTexels2(m_sideCount, colors);

				rendering::BufferCreationInfo info;
				info.allowShaderReads = true;
				info.allowCopies = true;
				info.size = colors.dataSize();

				auto sourceData = CreateSourceData(colors);
				m_sourceBuffer2 = createBuffer(info, sourceData);
			}

            // generate colors
            {
                rendering::BufferCreationInfo info;
                info.allowShaderReads = true;
				info.allowCopies = true;
				info.initialLayout = ResourceLayout::ShaderResource;
                info.size = m_sideCount * m_sideCount * 4;

                m_extraBuffer = createBuffer(info);
				m_extraBufferSRV = m_extraBuffer->createView(ImageFormat::RGBA8_UNORM);
            }

            m_shaders = loadGraphicsShader("FormatBufferPackedRead.csl");
        }

        void RenderingTest_BufferCopyRect::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferViewDepth)
        {
			// copy part
			{
				const auto length = (uint32_t)m_sideCount * m_sideCount;
				auto w = (uint32_t)m_rand.range(1, 600);
				auto sx = m_rand.range(length - w);
				auto tx = sx;

				ResourceCopyRange srcRange, destRange;
				srcRange.buffer.offset = sx * 4;
				srcRange.buffer.size = w * 4;
				destRange.buffer.offset = tx * 4;
				destRange.buffer.size = w * 4;

				bool flip = (int)(time / 8) & 1;
				auto source = flip ? m_sourceBuffer2 : m_sourceBuffer;

				cmd.opTransitionLayout(source, ResourceLayout::ShaderResource, ResourceLayout::CopySource);
				cmd.opTransitionLayout(m_extraBuffer, ResourceLayout::ShaderResource, ResourceLayout::CopyDest);

				cmd.opCopy(source, srcRange, m_extraBuffer, destRange);

				cmd.opTransitionLayout(source, ResourceLayout::CopySource, ResourceLayout::ShaderResource);
				cmd.opTransitionLayout(m_extraBuffer, ResourceLayout::CopyDest, ResourceLayout::ShaderResource);
			}

            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);		

			struct
			{
				uint32_t sideCount;
			} consts;

			consts.sideCount = m_sideCount;

			DescriptorEntry desc[2];
			desc[0].constants(consts);
			desc[1] = m_extraBufferSRV;
            cmd.opBindDescriptor("TestParams"_id, desc);

			drawQuad(cmd, m_shaders);

            cmd.opEndPass();
        }

    } // test
} // rendering