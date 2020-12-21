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
#include "rendering/device/include/renderingDeviceGlobalObjects.h"
#include "rendering/device/include/renderingPipeline.h"
#include "rendering/device/include/renderingShader.h"
#include "rendering/device/include/renderingShaderMetadata.h"

namespace rendering
{
    namespace test
    {
        /// test of the compute shader access to storage image
        class RenderingTest_ComputeFillTexture2D : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_ComputeFillTexture2D, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth) override final;

        private:
            static const auto SIDE_RESOLUTION = 512;

            ComputePipelineObjectPtr m_shaderGenerate;
            GraphicsPipelineObjectPtr m_shaderTest;

            BufferObjectPtr m_vertexBuffer;

            ImageObjectPtr m_image;
			ImageReadOnlyViewPtr m_imageSRV;
			ImageWritableViewPtr m_imageUAV;

            uint32_t m_vertexCount;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_ComputeFillTexture2D);
            RTTI_METADATA(RenderingTestOrderMetadata).order(2210);
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

        void RenderingTest_ComputeFillTexture2D::initialize()
        {
            // generate test geometry
            {
                base::Array<Simple3DVertex> vertices;
                PrepareTestGeometry(-0.9f, -0.9f, 1.8f, 1.8f, vertices);
                m_vertexBuffer = createVertexBuffer(vertices);
            }

            // create image
            ImageCreationInfo storageImageSetup;
            storageImageSetup.format = ImageFormat::RGBA8_UNORM;
            storageImageSetup.width = SIDE_RESOLUTION;
            storageImageSetup.height = SIDE_RESOLUTION;
            storageImageSetup.view = ImageViewType::View2D;
            storageImageSetup.allowShaderReads = true;
            storageImageSetup.allowUAV = true;
            m_image = createImage(storageImageSetup);

			auto sampler = Globals().SamplerClampPoint;
			m_imageSRV = m_image->createReadOnlyView();
			m_imageUAV = m_image->createWritableView();

            m_shaderGenerate = loadComputeShader("ComputeFillTexture2DGenerate.csl");
            m_shaderTest = loadGraphicsShader("ComputeFillTexture2DText.csl");
        }

        void RenderingTest_ComputeFillTexture2D::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth)
        {
			struct
			{
				base::Vector2 ZoomOffset;
				base::Vector2 ZoomScale;
				uint32_t SideCount;
				uint32_t MaxIterations;
			} params;

			// upload consts
            float linear = std::fmod(time * 0.2f, 1.0f); // 10s
            float logZoom = base::Lerp(std::log(2.0f), std::log(0.000014628f), linear);
            float zoom = exp(logZoom);
			params.ZoomOffset = base::Vector2(-0.743643135f, -0.131825963f);
			params.ZoomScale = base::Vector2(zoom, zoom);
			params.SideCount = SIDE_RESOLUTION;
			params.MaxIterations = (uint32_t)(256 + 768 * linear);

            {
				DescriptorEntry desc[2];
				desc[0].constants(params);
				desc[1] = m_imageUAV;
                cmd.opBindDescriptor("TestParamsWrite"_id, desc);
				cmd.opDispatchThreads(m_shaderGenerate, SIDE_RESOLUTION, SIDE_RESOLUTION);
            }

			cmd.opTransitionLayout(m_image, ResourceLayout::UAV, ResourceLayout::ShaderResource);

            {
                FrameBuffer fb;
                fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

                cmd.opBeingPass(fb);

				{
					DescriptorEntry desc[1];
					desc[0] = m_imageSRV;
					cmd.opBindDescriptor("TestParamsRead"_id, desc);
				}

                cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBuffer);
                cmd.opDraw(m_shaderTest, 0, 6);
                cmd.opEndPass();
            }
        }

    } // test
} // rendering