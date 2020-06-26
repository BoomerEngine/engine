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
        /// test of a implicit lod calculation + mipmaps
        class RenderingTest_Mipmaps : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_Mipmaps, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth) override final;

        private:
            BufferView m_vertexBuffer;
            ImageView m_sampledImage;
            const ShaderLibrary* m_shaders;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_Mipmaps);
            RTTI_METADATA(RenderingTestOrderMetadata).order(800);
        RTTI_END_TYPE();

        //---       

        static void AddQuad(float x, float y, float size, base::Array<Simple3DVertex>& outVertices)
        {
            outVertices.pushBack(Simple3DVertex(x, y, 0.5f, 0.0f, 0.0f));
            outVertices.pushBack(Simple3DVertex(x + size, y, 0.5f, 1.0f, 0.0f));
            outVertices.pushBack(Simple3DVertex(x + size, y + size, 0.5f, 1.0f, 1.0f));
            outVertices.pushBack(Simple3DVertex(x, y, 0.5f, 0.0f, 0.0f));
            outVertices.pushBack(Simple3DVertex(x + size, y + size, 0.5f, 1.0f, 1.0f));
            outVertices.pushBack(Simple3DVertex(x, y + size, 0.5f, 0.0f, 1.0f));
        }

        static void PrepareTestGeometry(float x, float y, float w, float h, base::Array<Simple3DVertex>& outVertices)
        {
            auto px = x;
            auto py = y;
            auto m = 0.05f;

            AddQuad(px, py, 2.0f, outVertices);
            px += 0.2f;
            py += 0.2f;

            AddQuad(px, py, 1.0f, outVertices); 
            px += 1.0f + m;

            float size = 0.5f;
            for (uint32_t i = 0; i < 12; ++i)
            {
                AddQuad(px, py, size, outVertices);
                py += size + m;
                size /= 2.0f;
            }
        }

        namespace
        {
            struct TestParams
            {
                ImageView TestImage;
            };
        }

        void RenderingTest_Mipmaps::initialize()
        {
            // generate test geometry
            base::Array<Simple3DVertex> vertices;
            PrepareTestGeometry(-1.0f, -1.0f, 2.0f, 2.0f, vertices);

            // create vertex buffer
            {
                rendering::BufferCreationInfo info;
                info.allowVertex = true;
                info.size = vertices.dataSize();

                auto sourceData = CreateSourceData(vertices);
                m_vertexBuffer = createBuffer(info, &sourceData);
            }

            //m_sampledImage = CreateChecker2D(1024, rs, 16);
            m_sampledImage = createMipmapTest2D(1024, true);
            
            m_shaders = loadShader("GenericTexture.csl");
        }

        void RenderingTest_Mipmaps::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth)
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);

            TestParams tempParams;
            tempParams.TestImage = m_sampledImage;
            cmd.opBindParametersInline("TestParams"_id, tempParams);

            cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBuffer);
            cmd.opDraw(m_shaders, 0, m_vertexBuffer.size() / sizeof(Simple3DVertex));
            cmd.opEndPass();
        }

    } // test
} // rendering