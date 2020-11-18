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
        /// test of the rendering without vertex buffer
        class RenderingTest_NoVertexBufferDraw : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_NoVertexBufferDraw, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView ) override final;

        private:
            BufferView m_storageVertices;
            BufferView m_storageColors;
            const ShaderLibrary* m_shaders;
            uint32_t m_vertexCount;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_NoVertexBufferDraw);
            RTTI_METADATA(RenderingTestOrderMetadata).order(60);
        RTTI_END_TYPE();

        //---       

        static const auto TRIANGLES_PER_SIDE = 64;
        static const auto MAX_TRIANGLES = TRIANGLES_PER_SIDE * TRIANGLES_PER_SIDE * 2;
        static const auto MAX_VERTICES = MAX_TRIANGLES * 3;

        static void PrepareTestGeometry(float x, float y, float w, float h, base::Vector4* outVertices, base::Vector4* outColors)
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

                    outColors[colorWriteIndex + 0].x = s1;
                    outColors[colorWriteIndex + 0].y = 0.0f;
                    outColors[colorWriteIndex + 0].z = 0.0f;

                    outColors[colorWriteIndex + 1].x = 0.0f;
                    outColors[colorWriteIndex + 1].y = s2;
                    outColors[colorWriteIndex + 1].z = 0.0f;
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
            PrepareTestGeometry(-0.9f, -0.9f, 1.8f, 1.8f, positionBuffer, colorBuffer);
            m_vertexCount = MAX_VERTICES;

            // create vertex buffer
            {
                rendering::BufferCreationInfo info;
                info.allowShaderReads = true;
                info.allowUAV = true;
                info.size = sizeof(positionBuffer);
                info.stride = sizeof(positionBuffer[0]);

                auto sourceData = CreateSourceDataRaw(positionBuffer);
                m_storageVertices = createBuffer(info, &sourceData);
            }

            // create extra buffer
            {
                rendering::BufferCreationInfo info;
                info.allowShaderReads = true;
                info.allowUAV = true;
                info.size = sizeof(colorBuffer);
                info.stride = sizeof(colorBuffer[0]);

                auto sourceData = CreateSourceDataRaw(colorBuffer);
                m_storageColors = createBuffer(info, &sourceData);
            }

            m_shaders = loadShader("NoVertexBufferDraw.csl");
        }

        namespace
        {
            struct TestParams
            {
                BufferView VertexPositions;
                BufferView VertexColors;
            };
        }

        void RenderingTest_NoVertexBufferDraw::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView )
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);

            TestParams tempParams;
            tempParams.VertexPositions = m_storageVertices;
            tempParams.VertexColors = m_storageColors;
            cmd.opSetPrimitiveType(PrimitiveTopology::TriangleList);
            cmd.opBindParametersInline("TestParams"_id, tempParams);

            cmd.opDraw(m_shaders, 0, m_vertexCount);
            cmd.opEndPass();
        }

    } // test
} // rendering