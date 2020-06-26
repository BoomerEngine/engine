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

#include "rendering/driver/include/renderingDriver.h"
#include "rendering/driver/include/renderingCommandWriter.h"

namespace rendering
{
    namespace test
    {
        /// test of buffer offsetting
        class RenderingTest_BufferOffsets : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_BufferOffsets, IRenderingTest);

        public:
            virtual void initialize() override final;            
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView ) override final;

        private:
            VertexIndexBunch<> m_indexedTriList;
            const ShaderLibrary* m_shaders;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_BufferOffsets);
            RTTI_METADATA(RenderingTestOrderMetadata).order(50);
        RTTI_END_TYPE();

        //---       

        static const uint32_t NUM_PARTS = 256;


        static base::Color Rainbow(float x)
        {
            static const base::Color colors[] = {
                base::Color(255, 0, 0),
                base::Color(0, 255, 0),
                base::Color(0, 0, 255) };

            auto numSegs  = ARRAY_COUNT(colors) + 1;
            auto i  = (int)floor(fabs(x));
            auto f  = fabs(x) - (float)i;

            return base::Blend(colors[i % numSegs], colors[(i + 1) % numSegs], f * 256.0f);
        }

        static void PrepareIndexedTriangleList(float x, float y, float w, float h, VertexIndexBunch<>& outGeometry)
        {
            for (uint32_t px = 0; px < NUM_PARTS; ++px)
            {
                auto dx  = px / (float)NUM_PARTS;

                Simple3DVertex v;
                v.VertexPosition.x = x + w * dx;
                v.VertexPosition.y = y;
                v.VertexPosition.z = 0.5f;
                v.VertexColor = Rainbow(dx * 10);
                outGeometry.m_vertices.pushBack(v);

                v.VertexPosition.y = y + h;
                outGeometry.m_vertices.pushBack(v);
            }

            uint16_t top = 0;
            uint16_t bottom = 1;
            uint16_t prevTop = 0, prevBottom = 0;
            for (uint32_t px = 0; px < NUM_PARTS; ++px, top += 2, bottom += 2)
            {
                if (px > 0)
                {
                    outGeometry.m_indices.pushBack(prevTop);
                    outGeometry.m_indices.pushBack(top);
                    outGeometry.m_indices.pushBack(bottom);
                    outGeometry.m_indices.pushBack(prevTop);
                    outGeometry.m_indices.pushBack(bottom);
                    outGeometry.m_indices.pushBack(prevBottom);
                }

                prevTop = top;
                prevBottom = bottom;
            }
        }

        void RenderingTest_BufferOffsets::initialize()
        {
            PrepareIndexedTriangleList(-0.9f, 0.f, 1.8f, 0.09f, m_indexedTriList);
            m_indexedTriList.createBuffers(*this);

            m_shaders = loadShader("BufferOffsets.csl");
        }

        void RenderingTest_BufferOffsets::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView )
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);

            cmd.opSetPrimitiveType(PrimitiveTopology::TriangleList);
            cmd.opBindIndexBuffer(m_indexedTriList.m_indexBuffer, rendering::ImageFormat::R16_UINT);
            cmd.opBindVertexBuffer("Simple3DVertex"_id, m_indexedTriList.m_vertexBuffer);

            auto numIndices = m_indexedTriList.m_indices.size();
            auto numVertices = m_indexedTriList.m_vertices.size();

            {
                float y = -0.9f;
                float ystep = 0.1f;

                cmd.opBindParametersInline("TestParams"_id, cmd.opUploadConstants(base::Vector2(0.0f, y)));
                y += ystep;
                cmd.opDrawIndexed(m_shaders, 0, 0, numIndices);

                y += 0.05f;

                // sub-parts by index offset
                for (uint32_t subOffset = 0; subOffset < 3; subOffset++)
                {
                    cmd.opBindParametersInline("TestParams"_id, cmd.opUploadConstants(base::Vector2(0.0f, y)));

                    y += ystep;
                    uint32_t firstIndex = 0;
                    uint32_t indexBlock = 6 * 10;
                    while (firstIndex + indexBlock < numIndices)
                    {
                        cmd.opDrawIndexed(m_shaders, 0, firstIndex, indexBlock);
                        firstIndex += indexBlock + 12 + subOffset;
                    }
                }

                y += 0.05f;

                // sub-parts by index buffer view
                for (uint32_t subOffset = 0; subOffset < 3; subOffset++)
                {
                    cmd.opBindParametersInline("TestParams"_id, cmd.opUploadConstants(base::Vector2(0.0f, y)));
                
                    y += ystep;
                    uint32_t firstIndex = 0;
                    uint32_t indexBlock = 6 * 10;
                    while (firstIndex + indexBlock < numIndices)
                    {
                        auto offsetedIndexBufer  = m_indexedTriList.m_indexBuffer.createSubViewAtOffset(firstIndex * sizeof(uint16_t));
                        cmd.opBindIndexBuffer(offsetedIndexBufer, ImageFormat::R16_UINT);

                        cmd.opDrawIndexed(m_shaders, 0, 0, indexBlock);
                        firstIndex += indexBlock + 12 + subOffset;
                    }
                }

                y += 0.05f;

                // sub-parts by vertex offset
                for (uint32_t subOffset = 0; subOffset < 3; subOffset++)
                {
                    cmd.opBindIndexBuffer(m_indexedTriList.m_indexBuffer, ImageFormat::R16_UINT);

                    cmd.opBindParametersInline("TestParams"_id, cmd.opUploadConstants(base::Vector2(0.0f, y)));

                    y += ystep;
                    uint32_t firstVertex = 0;
                    uint32_t indexBlock = 6 * 10;
                    uint32_t vertexBlock = 2 * 10;
                    while (firstVertex + vertexBlock < numVertices)
                    {
                        cmd.opDrawIndexed(m_shaders, firstVertex, 0, indexBlock);
                        firstVertex += vertexBlock + 4 + subOffset;
                    }
                }

                y += 0.05f;

                // sub-parts by vertex buffer offset
                for (uint32_t subOffset = 0; subOffset < 3; subOffset++)
                {
                    cmd.opBindIndexBuffer(m_indexedTriList.m_indexBuffer, rendering::ImageFormat::R16_UINT);

                    cmd.opBindParametersInline("TestParams"_id, cmd.opUploadConstants(base::Vector2(0.0f, y)));

                    y += ystep;

                    uint32_t firstVertex = 0;
                    uint32_t indexBlock = 6 * 10;
                    uint32_t vertexBlock = 2 * 10;
                    while (firstVertex + vertexBlock < numVertices)
                    {
                        auto offsetedVertexBuffer  = m_indexedTriList.m_vertexBuffer.createSubViewAtOffset(firstVertex * sizeof(Simple3DVertex));
                        cmd.opBindVertexBuffer("Simple3DVertex"_id,  offsetedVertexBuffer);

                        cmd.opDrawIndexed(m_shaders, 0, 0, indexBlock);
                        firstVertex += vertexBlock + 4 + subOffset;
                    }
                }
            }

            cmd.opEndPass();
        }

    } // test
} // rendering