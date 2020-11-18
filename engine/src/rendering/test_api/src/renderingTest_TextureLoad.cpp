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

#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingCommandWriter.h"

namespace rendering
{
    namespace test
    {
        /// test of a static image
        class RenderingTest_TextureLoad : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_TextureLoad, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth) override final;

        private:
            BufferView m_vertexBuffer;
            ImageView m_staticImage;
            const ShaderLibrary* m_shaders;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_TextureLoad);
            RTTI_METADATA(RenderingTestOrderMetadata).order(700);
            RTTI_METADATA(RenderingTestSubtestCountMetadata).count(2);
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

        namespace
        {
            struct TestParams
            {
                ImageView TestImage;
            };
        }

        void RenderingTest_TextureLoad::initialize()
        {
            // generate test geometry
            base::Array<Simple3DVertex> vertices;
            PrepareTestGeometry(-0.9f, -0.9f, 1.8f, 1.8f, vertices);

            // create vertex buffer
            {
                rendering::BufferCreationInfo info;
                info.allowVertex = true;
                info.size = vertices.dataSize();

                auto sourceData = CreateSourceData(vertices);
                m_vertexBuffer = createBuffer(info, &sourceData);
            }

            m_staticImage = loadImage2D("lena.png");

            if (subTestIndex() == 0)
                m_shaders = loadShader("TextureLoad.csl");
            else
                m_shaders = loadShader("TextureLoadOffset.csl");
        }

        void RenderingTest_TextureLoad::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth)
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);
            cmd.opSetPrimitiveType(PrimitiveTopology::TriangleList);

            TestParams tempParams;
            tempParams.TestImage = m_staticImage;
            cmd.opBindParametersInline("TestParams"_id, tempParams);

            cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBuffer);
            cmd.opDraw(m_shaders, 0, 6); // quad

            cmd.opEndPass();            
        }

    } // test
} // rendering