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
        /// test of the discard functionality
        class RenderingTest_Discard : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_Discard, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView ) override final;

        private:
			BufferObjectPtr m_vertexBuffer;
			BufferObjectPtr m_extraBuffer;
            ShaderLibraryPtr m_shaders;

            uint32_t m_sideCount;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_Discard);
            RTTI_METADATA(RenderingTestOrderMetadata).order(150);
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
		
        void RenderingTest_Discard::initialize()
        {
            // generate test geometry
            {
                base::Array<Simple3DVertex> vertices;
                PrepareTestGeometry(-0.9f, -0.9f, 1.8f, 1.8f, vertices);
                m_vertexBuffer = createVertexBuffer(vertices);
            }

            m_shaders = loadShader("Discard.csl");
        }

        void RenderingTest_Discard::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView )
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);

			const auto ScreenResolution = base::Vector2((float)backBufferView->width(), (float)backBufferView->height());

			DescriptorEntry params[1];
			params[0].constants(ScreenResolution);
			cmd.opBindDescriptor("TestParams"_id, params);

            cmd.opSetPrimitiveType(PrimitiveTopology::TriangleList);
            cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBuffer);
            cmd.opDraw(m_shaders, 0, 6); // quad
            cmd.opEndPass();
        }

    } // test
} // rendering