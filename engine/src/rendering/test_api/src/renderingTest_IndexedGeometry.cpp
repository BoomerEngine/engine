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
        /// indexed geometry test
        class RenderingTest_IndexedGeometry : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_IndexedGeometry, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView ) override final;

        private:
            VertexIndexBunch<> m_indexedLineList;
            VertexIndexBunch<> m_indexedTriList;
            VertexIndexBunch<> m_indexedTriStrip;
            VertexIndexBunch<> m_indexedTriStripVertexRestart;
            ShaderLibraryPtr m_shaders;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_IndexedGeometry);
        RTTI_METADATA(RenderingTestOrderMetadata).order(80);
        RTTI_END_TYPE();

        //---       

        static const uint32_t GRID_SIZE_X = 64;
        static const uint32_t GRID_SIZE_Y = 16;

        static base::Color Rainbow(float x)
        {
            static const base::Color colors[] = {
                base::Color(148, 0, 211),
                 base::Color(75, 0, 130),
                 base::Color(0, 0, 255),
                 base::Color(0, 255, 0),
                 base::Color(255, 255, 0),
                 base::Color(255, 127, 0),
                 base::Color(255, 0, 0) };

            auto numSegs = ARRAY_COUNT(colors) + 1;
            auto i = (int)floor(fabs(x));
            auto f = fabs(x) - (float)i;

            return base::Lerp(colors[i % numSegs], colors[(i + 1) % numSegs], f);
        }

        static void PrepareRainbowVertices(float x, float y, float w, float h, VertexIndexBunch<>& outGeometry)
        {
            for (uint32_t py = 0; py < GRID_SIZE_Y; ++py)
            {
                for (uint32_t px = 0; px < GRID_SIZE_X; ++px)
                {
                    auto dv = base::Vector2(px / (float)GRID_SIZE_X, py / (float)GRID_SIZE_Y);

                    Simple3DVertex v;
                    v.VertexPosition.x = x + w * dv.x;
                    v.VertexPosition.y = y + h * dv.y;
                    v.VertexPosition.z = 0.5f;
                    v.VertexColor = Rainbow(dv.length() * 17.0f);
                    outGeometry.m_vertices.pushBack(v);
                }
            }
        }

        static void PrepareIndexedLineList(float x, float y, float w, float h, VertexIndexBunch<>& outGeometry)
        {
            PrepareRainbowVertices(x, y, w, h, outGeometry);

            for (uint16_t py = 0; py < GRID_SIZE_Y; ++py)
            {
                uint16_t prevRow = range_cast<uint16_t>((py - 1) * GRID_SIZE_X);
                uint16_t thisRow = range_cast<uint16_t>(py * GRID_SIZE_X);

                for (uint16_t px = 0; px < GRID_SIZE_X; ++px)
                {
                    if (px > 0)
                    {
                        outGeometry.m_indices.pushBack(thisRow + px - 1);
                        outGeometry.m_indices.pushBack(thisRow + px);
                    }

                    if (py > 0)
                    {
                        outGeometry.m_indices.pushBack(prevRow + px);
                        outGeometry.m_indices.pushBack(thisRow + px);
                    }
                }
            }
        }

        static void PrepareIndexedTriangleList(float x, float y, float w, float h, VertexIndexBunch<>& outGeometry)
        {
            PrepareRainbowVertices(x, y, w, h, outGeometry);

            for (uint16_t py = 0; py < GRID_SIZE_Y; ++py)
            {
                uint16_t prevRow = range_cast<uint16_t>((py - 1) * GRID_SIZE_X);
                uint16_t thisRow = range_cast<uint16_t>(py * GRID_SIZE_X);

                for (uint16_t px = 0; px < GRID_SIZE_X; ++px)
                {
                    if (px > 0 && py > 0)
                    {
                        outGeometry.m_indices.pushBack(prevRow + px - 1);
                        outGeometry.m_indices.pushBack(prevRow + px);
                        outGeometry.m_indices.pushBack(thisRow + px);

                        outGeometry.m_indices.pushBack(prevRow + px - 1);
                        outGeometry.m_indices.pushBack(thisRow + px);
                        outGeometry.m_indices.pushBack(thisRow + px - 1);
                    }
                }
            }
        }

        static void PrepareIndexedTriangleStrip(float x, float y, float w, float h, VertexIndexBunch<>& outGeometry)
        {
            PrepareRainbowVertices(x, y, w, h, outGeometry);

            for (uint16_t py = 0; py < GRID_SIZE_Y; ++py)
            {
                uint16_t prevRow = range_cast<uint16_t>((py - 1) * GRID_SIZE_X);
                uint16_t thisRow = range_cast<uint16_t>(py * GRID_SIZE_X);

                if (py > 1)
                    outGeometry.m_indices.pushBack(prevRow);

                for (uint16_t px = 0; px < GRID_SIZE_X; ++px)
                {
                    if (py > 0)
                    {
                        outGeometry.m_indices.pushBack(prevRow + px);
                        outGeometry.m_indices.pushBack(thisRow + px);
                    }
                }

                if (py > 0)
                    outGeometry.m_indices.pushBack(thisRow + GRID_SIZE_X - 1);
            }
        }

        static void PrepareIndexedTriangleStripWithVertexRestart(float x, float y, float w, float h, VertexIndexBunch<>& outGeometry)
        {
            PrepareRainbowVertices(x, y, w, h, outGeometry);

            for (uint16_t py = 0; py < GRID_SIZE_Y; ++py)
            {
                uint16_t prevRow = range_cast<uint16_t>((py - 1) * GRID_SIZE_X);
                uint16_t thisRow = range_cast<uint16_t>(py * GRID_SIZE_X);

                if (py > 1)
                    outGeometry.m_indices.pushBack(0xFFFF);

                for (uint16_t px = 0; px < GRID_SIZE_X; ++px)
                {
                    if (py > 0)
                    {
                        outGeometry.m_indices.pushBack(prevRow + px);
                        outGeometry.m_indices.pushBack(thisRow + px);
                    }
                }
            }
        }


        void RenderingTest_IndexedGeometry::initialize()
        {
            m_shaders = loadShader("GenericGeometry.csl");

            float y = -0.9f;
            float ystep = 0.48f;
            float ysize = ystep * 0.9f;

            // create test for indexed lists 
            {
                PrepareIndexedLineList(-0.9f, y, 1.8f, ysize, m_indexedLineList);
                m_indexedLineList.createBuffers(*this);
                y += ystep;
            }

            // create test for triangle list
            {
                PrepareIndexedTriangleList(-0.9f, y, 1.8f, ysize, m_indexedTriList);
                m_indexedTriList.createBuffers(*this);
                y += ystep;
            }

            // create test for triangle strip
            {
                PrepareIndexedTriangleStrip(-0.9f, y, 1.8f, ysize, m_indexedTriStrip);
                m_indexedTriStrip.createBuffers(*this);
                y += ystep;
            }

            // create test for triangle strip with restart vertex
            {
                PrepareIndexedTriangleStripWithVertexRestart(-0.9f, y, 1.8f, ysize, m_indexedTriStripVertexRestart);
                m_indexedTriStripVertexRestart.createBuffers(*this);
                y += ystep;
            }
        }

        void RenderingTest_IndexedGeometry::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView )
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);

            cmd.opSetPrimitiveType(PrimitiveTopology::LineList);
            m_indexedLineList.draw(cmd, m_shaders);

            cmd.opSetPrimitiveType(PrimitiveTopology::TriangleList);
            m_indexedTriList.draw(cmd, m_shaders);

            cmd.opSetPrimitiveType(PrimitiveTopology::TriangleStrip);
            m_indexedTriStrip.draw(cmd, m_shaders);

            cmd.opSetPrimitiveType(PrimitiveTopology::TriangleStrip, true);
            m_indexedTriStripVertexRestart.draw(cmd, m_shaders);

            cmd.opEndPass();
        }

    } // test
} // rendering