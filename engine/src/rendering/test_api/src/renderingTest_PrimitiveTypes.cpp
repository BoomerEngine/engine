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
        /// primitive types test
        class RenderingTest_PrimitiveTypes : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_PrimitiveTypes, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView ) override final;

        private:
            BufferObjectPtr m_vertexBufferPointList;
			BufferObjectPtr m_vertexBufferLineList;
			BufferObjectPtr m_vertexBufferLineStrip;
			BufferObjectPtr m_vertexBufferTriangleList;
			BufferObjectPtr m_vertexBufferTriangleStrip;

            uint32_t m_numPointListVertices = 0;
            uint32_t m_numLineListVertices = 0;
            uint32_t m_numLineStripVertices = 0;
            uint32_t m_numTriangleListVertices = 0;
            uint32_t m_numTriangleStripVertices = 0;

            ShaderLibraryPtr m_shaders;

            static const auto NUM_LINE_SEGMENTS = 512U;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_PrimitiveTypes);
            RTTI_METADATA(RenderingTestOrderMetadata).order(40);
        RTTI_END_TYPE();

        //---       

        static float Sinc(float x)
        {
            auto f = -40.0f + 80.0f*x;
            if (f == 0)
                return 1.0f;

            return sin(f) / f;
        }

        static void GeneratePointListTest(float x, float y, float w, float h, base::Color color, uint32_t count, base::Array<Simple3DVertex>& outVertices)
        {
            for (uint32_t i = 0; i < count; ++i)
            {
                auto alpha = (float)i / (float)(count - 1);
                auto beta = Sinc(alpha);

                Simple3DVertex vertex;
                vertex.VertexPosition.x = x + alpha * w;
                vertex.VertexPosition.y = y + h * (1 - beta);
                vertex.VertexPosition.z = 0.5f;
                vertex.VertexColor = color;
                outVertices.pushBack(vertex);
            }
        }

        static void GenerateLineListTest(float x, float y, float w, float h, base::Color color, uint32_t count, base::Array<Simple3DVertex>& outVertices)
        {       
            Simple3DVertex prev;

            for (uint32_t i = 0; i < count; ++i)
            {
                auto alpha = (float)i / (float)(count - 1);
                auto beta = Sinc(alpha);

                Simple3DVertex vertex;
                vertex.VertexPosition.x = x + alpha * w;
                vertex.VertexPosition.y = y + h * (1-beta);
                vertex.VertexPosition.z = 0.5f;
                vertex.VertexColor = base::Color::FromVectorLinear(base::Vector4(beta, 1.0f - beta, 0.0f, 1.0f));

                if (i > 1)
                {
                    prev.VertexColor = vertex.VertexColor;
                    outVertices.pushBack(prev);
                    outVertices.pushBack(vertex);
                }

                prev = vertex;
            }
        }

        static void GenerateLineStripTest(float x, float y, float w, float h, base::Color color, uint32_t count, base::Array<Simple3DVertex>& outVertices)
        {
            for (uint32_t i = 0; i < count; ++i)
            {
                auto alpha = (float)i / (float)(count - 1);
                auto beta = Sinc(alpha);

                Simple3DVertex vertex;
                vertex.VertexPosition.x = x + alpha * w;
                vertex.VertexPosition.y = y + h * (1 - beta);
                vertex.VertexPosition.z = 0.5f;
                vertex.VertexColor = base::Color::FromVectorLinear(base::Vector4(beta, 1.0f - beta, 0.0f, 1.0f));
                outVertices.pushBack(vertex);
            }
        }

        static void GenerateTriangleListTest(float x, float y, float w, float h, base::Color color, uint32_t count, base::Array<Simple3DVertex>& outVertices)
        {

            Simple3DVertex prevTop;
            Simple3DVertex prevBottom;

            for (uint32_t i = 0; i < count; ++i)
            {
                auto alpha = (float)i / (float)(count - 1);
                auto beta = Sinc(alpha);

                Simple3DVertex topVertex;
                topVertex.VertexPosition.x = x + alpha * w;
                topVertex.VertexPosition.y = y + h * (1 - beta);
                topVertex.VertexPosition.z = 0.5f;
                topVertex.VertexColor = base::Color::FromVectorLinear(base::Vector4(beta, 1.0f - beta, 0.0f, 1.0f));

                Simple3DVertex bottomVertex;
                bottomVertex.VertexPosition.x = x + alpha * w;
                bottomVertex.VertexPosition.y = y + h;
                bottomVertex.VertexPosition.z = 0.5f;
                bottomVertex.VertexColor = base::Color::BLACK;

                if (i > 0)
                {
                    outVertices.pushBack(prevTop);
                    outVertices.pushBack(topVertex);
                    outVertices.pushBack(bottomVertex);
                    outVertices.pushBack(prevTop);
                    outVertices.pushBack(bottomVertex);
                    outVertices.pushBack(prevBottom);
                }

                prevTop = topVertex;
                prevBottom = bottomVertex;
            }
        }

        static void GenerateTriangleStripTest(float x, float y, float w, float h, base::Color color, uint32_t count, base::Array<Simple3DVertex>& outVertices)
        {
            for (uint32_t i = 0; i < count; ++i)
            {
                auto alpha = (float)i / (float)(count - 1);
                auto beta = Sinc(alpha);

                Simple3DVertex topVertex;
                topVertex.VertexPosition.x = x + alpha * w;
                topVertex.VertexPosition.y = y + h * (1 - beta);
                topVertex.VertexPosition.z = 0.5f;
                topVertex.VertexColor = base::Color::FromVectorLinear(base::Vector4(beta, 1.0f - beta, 0.0f, 1.0f));

                Simple3DVertex bottomVertex;
                bottomVertex.VertexPosition.x = x + alpha * w;
                bottomVertex.VertexPosition.y = y + h;
                bottomVertex.VertexPosition.z = 0.5f;
                bottomVertex.VertexColor = topVertex.VertexColor;

                outVertices.pushBack(topVertex);
                outVertices.pushBack(bottomVertex);
            }
        }


        void RenderingTest_PrimitiveTypes::initialize()
        {
            float y = -0.98f;
            float ystep = 0.37f;
            float yscale = 0.9f;

            // create vertex buffer for point test
            {
                base::Array<Simple3DVertex> verties;
                GeneratePointListTest(-0.9f, y, 1.8f, yscale * ystep, base::Color::CYAN, NUM_LINE_SEGMENTS, verties);
                y += ystep;

                m_numPointListVertices = verties.size();
                m_vertexBufferPointList = createVertexBuffer(verties);
            }

            // create vertex buffer for line test
            {
                base::Array<Simple3DVertex> verties;
                GenerateLineListTest(-0.9f, y, 1.8f, yscale * ystep, base::Color::CYAN, NUM_LINE_SEGMENTS, verties);
                y += ystep;

                m_numLineListVertices = verties.size();
                m_vertexBufferLineList = createVertexBuffer(verties);
            }

            // create vertex buffer for line strip test
            {
                base::Array<Simple3DVertex> verties;
                GenerateLineStripTest(-0.9f, y, 1.8f, yscale * ystep, base::Color::CYAN, NUM_LINE_SEGMENTS, verties);
                y += ystep;

                m_numLineStripVertices = verties.size();
                m_vertexBufferLineStrip = createVertexBuffer(verties);
            }

            // create vertex buffer for triangle list test
            {
                base::Array<Simple3DVertex> verties;
                GenerateTriangleListTest(-0.9f, y, 1.8f, yscale * ystep, base::Color::CYAN, NUM_LINE_SEGMENTS, verties);
                y += ystep;

                m_numTriangleListVertices = verties.size();
                m_vertexBufferTriangleList = createVertexBuffer(verties);
            }

            // create vertex buffer for triangle trip test
            {
                base::Array<Simple3DVertex> verties;
                GenerateTriangleStripTest(-0.9f, y, 1.8f, yscale * ystep, base::Color::CYAN, NUM_LINE_SEGMENTS, verties);
                y += ystep;

                m_numTriangleStripVertices = verties.size();
                m_vertexBufferTriangleStrip = createVertexBuffer(verties);
            }

            m_shaders = loadShader("GenericGeometry.csl");
        }

        void RenderingTest_PrimitiveTypes::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView )
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);

            {
                cmd.opSetPrimitiveType(PrimitiveTopology::PointList);
                cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBufferPointList);
                cmd.opDraw(m_shaders, 0, m_numPointListVertices);
            }

            {
                cmd.opSetPrimitiveType(PrimitiveTopology::LineList);
                cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBufferLineList);
                cmd.opDraw(m_shaders, 0, m_numLineListVertices);
            }

            {
                cmd.opSetPrimitiveType(PrimitiveTopology::LineStrip);
                cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBufferLineStrip);
                cmd.opDraw(m_shaders, 0, m_numLineStripVertices);
            }

            {
                cmd.opSetPrimitiveType(PrimitiveTopology::TriangleList);
                cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBufferTriangleList);
                cmd.opDraw(m_shaders, 0, m_numTriangleListVertices);
            }

            {
                cmd.opSetPrimitiveType(PrimitiveTopology::TriangleStrip);
                cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBufferTriangleStrip);
                cmd.opDraw(m_shaders, 0, m_numTriangleStripVertices);
            }

            cmd.opEndPass();
        }

    } // test
} // rendering