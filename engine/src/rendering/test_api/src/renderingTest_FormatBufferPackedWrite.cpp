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
        /// test of the non-atomic writes to the texel buffer
        class RenderingTest_FormatBufferPackedWrite : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_FormatBufferPackedWrite, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferViewDepth) override final;

        private:
            BufferView m_vertexBuffer;
            uint32_t m_vertexCount;
            const ShaderLibrary* m_shaderGenerate;
            const ShaderLibrary* m_shaderDraw;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_FormatBufferPackedWrite);
        RTTI_METADATA(RenderingTestOrderMetadata).order(310);
        RTTI_END_TYPE();

        //---       

        namespace
        {
            struct TestConsts
            {
                float TimeOffset;
                uint32_t SideCount;
            };

            struct TestParamsWrite
            {
                ConstantsView Params;
                BufferView Colors;
            };

            struct TestParamsRead
            {
                ConstantsView Params;
                BufferView Colors;
            };
        }

        static void PrepareTestGeometry(float x, float y, float w, float h, base::Array<Simple3DVertex>& outVertices)
        {
            outVertices.pushBack(Simple3DVertex(x, y, 0.5f, 0.0f, 0.0f));
            outVertices.pushBack(Simple3DVertex(x + w, y, 0.5f, 1.0f, 0.0f));
            outVertices.pushBack(Simple3DVertex(x + w, y + h, 0.5f, 1.0f, 1.0f));

            outVertices.pushBack(Simple3DVertex(x, y, 0.5f, 0.0f, 0.0f));
            outVertices.pushBack(Simple3DVertex(x + w, y + h, 0.5f, 1.0f, 1.0f));
            outVertices.pushBack(Simple3DVertex(x, y + h, 0.5f, 0.0f, 1.0f));
        }

        void RenderingTest_FormatBufferPackedWrite::initialize()
        {
            // generate test geometry
            base::Array<Simple3DVertex> vertices;
            PrepareTestGeometry(-0.9f, -0.9f, 1.8f, 1.8f, vertices);

            // create vertex buffer
            {
                rendering::BufferCreationInfo info;
                info.allowVertex = true;
                info.size = vertices.dataSize();

                auto sourceData  = CreateSourceData(vertices);
                m_vertexBuffer = createBuffer(info, &sourceData);
            }

            m_shaderGenerate = loadShader("FormatBufferPackedWriteGenerate.csl");
            m_shaderDraw = loadShader("FormatBufferPackedWriteTest.csl");
        }

        void RenderingTest_FormatBufferPackedWrite::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferViewDepth)
        {
            uint32_t SIDE_RESOLUTION = 128;

            //--

            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);

            TransientBufferView storageBuffer(BufferViewFlag::ShaderReadable, TransientBufferAccess::ShaderReadWrite, 4 * SIDE_RESOLUTION * SIDE_RESOLUTION);
            cmd.opAllocTransientBuffer(storageBuffer);

            TestConsts tempConsts;
            tempConsts.TimeOffset = time;
            tempConsts.SideCount = SIDE_RESOLUTION;
            auto consts  = cmd.opUploadConstants(tempConsts);

            {
                TestParamsWrite params;
                params.Params = consts;
                params.Colors = storageBuffer;
                cmd.opSetPrimitiveType(PrimitiveTopology::PointList);
                cmd.opBindParametersInline("TestParamsWrite"_id, params);
                cmd.opDraw(m_shaderGenerate, 0, SIDE_RESOLUTION * SIDE_RESOLUTION);
            }

            cmd.opGraphicsBarrier();

            {
                TestParamsRead params;
                params.Params = consts;
                params.Colors = storageBuffer;
                cmd.opBindParametersInline("TestParamsRead"_id, params);

                cmd.opSetPrimitiveType(PrimitiveTopology::TriangleList);
                cmd.opBindVertexBuffer("Simple3DVertex"_id, m_vertexBuffer);
                cmd.opDraw(m_shaderDraw, 0, 6);
            }

            cmd.opGraphicsBarrier();
            cmd.opEndPass();
        }

    } // test
} // rendering;