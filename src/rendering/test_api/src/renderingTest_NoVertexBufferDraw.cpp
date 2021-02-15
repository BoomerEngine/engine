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
#include "../../device/include/renderingBuffer.h"

namespace rendering
{
    namespace test
    {
        /// test of the rendering without vertex buffer
        class RenderingTest_NoVertexBufferDraw : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_NoVertexBufferDraw, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView ) override final;

        private:
			BufferObjectPtr m_storageVertices;
            BufferObjectPtr m_storageColors;

			DeviceObjectViewPtr m_storageVerticesView;
			DeviceObjectViewPtr m_storageColorsView;

            GraphicsPipelineObjectPtr m_shaders;
            uint32_t m_vertexCount = 0;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_NoVertexBufferDraw);
            RTTI_METADATA(RenderingTestOrderMetadata).order(60);
			RTTI_METADATA(RenderingTestSubtestCountMetadata).count(4);
        RTTI_END_TYPE();

        //---       

        static const auto TRIANGLES_PER_SIDE = 64;
        static const auto MAX_TRIANGLES = TRIANGLES_PER_SIDE * TRIANGLES_PER_SIDE * 2;
        static const auto MAX_VERTICES = MAX_TRIANGLES * 3;

        static void PrepareTestGeometry(float x, float y, float w, float h, int test, base::Vector4* outVertices, base::Vector4* outColors)
        {
            uint32_t vertexWriteIndex = 0;
            uint32_t colorWriteIndex = 0;
            for (uint32_t py = 0; py < TRIANGLES_PER_SIDE; ++py)
            {
                auto fx = py / (float)TRIANGLES_PER_SIDE;
                for (uint32_t px = 0; px < TRIANGLES_PER_SIDE; ++px, vertexWriteIndex += 6, colorWriteIndex += 2)
                {
                    auto fy = px / (float)TRIANGLES_PER_SIDE;

                    auto d1 = base::Vector2(fx + 0.5f, fy + 0.5f).length();
                    auto s1 = 0.5f + 0.5f * cos(d1 * TWOPI * 2.0f);
                    auto d2 = base::Vector2(fx + 3.5f, fy - 3.5f).length();
                    auto s2 = 0.5f + 0.5f * cos(d2 * TWOPI * 3.0f);

                    auto x0 = x + w * fx;
                    auto y0 = y + h * fy;
                    auto dx = w * (1.0f / (float)TRIANGLES_PER_SIDE);
                    auto dy = h * (1.0f / (float)TRIANGLES_PER_SIDE);

                    outVertices[vertexWriteIndex + 0] = base::Vector4(x0, y0, 0.5f, 1.0f);
                    outVertices[vertexWriteIndex + 1] = base::Vector4(x0 + dx, y0, 0.5f, 1.0f);
                    outVertices[vertexWriteIndex + 2] = base::Vector4(x0 + dx, y0 + dy, 0.5f, 1.0f);

                    outVertices[vertexWriteIndex + 3] = base::Vector4(x0, y0, 0.5f, 1.0f);
                    outVertices[vertexWriteIndex + 4] = base::Vector4(x0 + dx, y0 + dy, 0.5f, 1.0f);
                    outVertices[vertexWriteIndex + 5] = base::Vector4(x0, y0 + dy, 0.5f, 1.0f);

					switch (test)
					{
					case 0:
						outColors[colorWriteIndex + 0].x = s1;
						outColors[colorWriteIndex + 0].y = 0.0f;
						outColors[colorWriteIndex + 0].z = 0.0f;
						outColors[colorWriteIndex + 1].x = 0.0f;
						outColors[colorWriteIndex + 1].y = s2;
						outColors[colorWriteIndex + 1].z = 0.0f;
						break;

					case 1:
						outColors[colorWriteIndex + 0].x = s1;
						outColors[colorWriteIndex + 0].y = 0.0f;
						outColors[colorWriteIndex + 0].z = 0.0f;
						outColors[colorWriteIndex + 1].x = s1;
						outColors[colorWriteIndex + 1].y = 0.0f;
						outColors[colorWriteIndex + 1].z = 0.0f;
						break;

					case 2:
						outColors[colorWriteIndex + 0].x = 0.0f;
						outColors[colorWriteIndex + 0].y = s1;
						outColors[colorWriteIndex + 0].z = 0.0f;
						outColors[colorWriteIndex + 1].x = 0.0f;
						outColors[colorWriteIndex + 1].y = 0.0f;
						outColors[colorWriteIndex + 1].z = s2;
						break;

					case 3:
						outColors[colorWriteIndex + 0].x = 0.0f;
						outColors[colorWriteIndex + 0].y = s1;
						outColors[colorWriteIndex + 0].z = 0.0f;
						outColors[colorWriteIndex + 1].x = 0.0f;
						outColors[colorWriteIndex + 1].y = s1;
						outColors[colorWriteIndex + 1].z = 0.0f;
						break;
					}
                }
            }

            ASSERT(vertexWriteIndex == MAX_VERTICES);
            ASSERT(colorWriteIndex == MAX_TRIANGLES);
        }

        void RenderingTest_NoVertexBufferDraw::initialize()
        {
            base::Vector4 positionBuffer[MAX_VERTICES];
            base::Vector4 colorBuffer[MAX_TRIANGLES];

            // generate test geometry
            PrepareTestGeometry(-0.9f, -0.9f, 1.8f, 1.8f, subTestIndex(), positionBuffer, colorBuffer);
            m_vertexCount = MAX_VERTICES;

			// common setup
			rendering::BufferCreationInfo info;

			if (subTestIndex() & 1) // 1 3
				info.allowUAV = true;
			else
				info.allowShaderReads = true;

			if (subTestIndex() & 2) // 2 3
				info.stride = sizeof(positionBuffer[0]);
			else
				info.stride = 0;

            // create vertex buffer
			{
				info.size = sizeof(positionBuffer);
				auto sourceData = CreateSourceDataRaw(positionBuffer);
                m_storageVertices = createBuffer(info, sourceData);
            }

            // create extra buffer
            {
                info.size = sizeof(colorBuffer);
                auto sourceData = CreateSourceDataRaw(colorBuffer);
                m_storageColors = createBuffer(info, sourceData);
            }

			// load shader and prepare view
			switch (subTestIndex())
			{
				case 0:
				{
					m_shaders = loadGraphicsShader("NoVertexBufferDraw.csl");
					m_storageVerticesView = m_storageVertices->createView(ImageFormat::RGBA32F);
					m_storageColorsView = m_storageColors->createView(ImageFormat::RGBA32F);
					break;
				}

				case 1:
				{
					m_shaders = loadGraphicsShader("NoVertexBufferDrawUAV.csl");
					m_storageVerticesView = m_storageVertices->createWritableView(ImageFormat::RGBA32F);
					m_storageColorsView = m_storageColors->createWritableView(ImageFormat::RGBA32F);
					break;
				}

				case 2:
				{
					m_shaders = loadGraphicsShader("NoVertexBufferDrawStructured.csl");
					m_storageVerticesView = m_storageVertices->createStructuredView();
					m_storageColorsView = m_storageColors->createStructuredView();
					break;
				}

				case 3:
				{
					m_shaders = loadGraphicsShader("NoVertexBufferDrawStructuredUAV.csl");
					m_storageVerticesView = m_storageVertices->createWritableStructuredView();
					m_storageColorsView = m_storageColors->createWritableStructuredView();
					break;
				}
			}
        }

        void RenderingTest_NoVertexBufferDraw::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView )
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);

			DescriptorEntry params[2];
			params[0] = m_storageVerticesView;
			params[1] = m_storageColorsView;
			cmd.opBindDescriptor("TestParams"_id, params);

            cmd.opDraw(m_shaders, 0, m_vertexCount);
            cmd.opEndPass();
        }

    } // test
} // rendering