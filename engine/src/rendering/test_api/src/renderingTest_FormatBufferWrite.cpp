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
        /// test of the non-atomic writes to the storage buffer
        class RenderingTest_FormatBufferWrite : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_FormatBufferWrite, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth) override final;

        private:
            static const uint32_t MAX_ELEMENTS = 1024;

            uint32_t m_vertexCount;
            const ShaderLibrary* m_shaderGenerate;
            const ShaderLibrary* m_shaderTest;

            BufferView m_tempBuffer;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_FormatBufferWrite);
        RTTI_METADATA(RenderingTestOrderMetadata).order(330);
        RTTI_END_TYPE();

        //---       

        namespace
        {

            struct TestConsts
            {
                float TimeOffset;
                float DrawOffset;
                float DrawScale;
            };

            struct TestParams
            {
                ConstantsView Params;
                BufferView VertexData;
            };
        }

        void RenderingTest_FormatBufferWrite::initialize()
        {
            m_shaderGenerate = loadShader("FormatBufferWriteGenerate.csl");
            m_shaderTest = loadShader("FormatBufferWriteTest.csl");

            m_tempBuffer = createStorageBuffer(2 * sizeof(base::Vector4) * MAX_ELEMENTS);
        }

        void RenderingTest_FormatBufferWrite::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth)
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);

            float yScale = 0.05f;
            for (float y = -1.0f; y < 1.0f; y += yScale)
            {
                TestConsts tempConsts;
                tempConsts.TimeOffset = time + y * TWOPI;
                tempConsts.DrawOffset = y;
                tempConsts.DrawScale = yScale;

                TestParams params;
                params.Params = cmd.opUploadConstants(tempConsts);
                params.VertexData = m_tempBuffer;
                cmd.opBindParametersInline("TestParams"_id, params);

                {
                    cmd.opSetPrimitiveType(PrimitiveTopology::PointList);
                    cmd.opDraw(m_shaderGenerate, 0, MAX_ELEMENTS);
                }

                cmd.opGraphicsBarrier();

                {
                    cmd.opSetPrimitiveType(PrimitiveTopology::LineStrip);
                    cmd.opDraw(m_shaderTest, 0, MAX_ELEMENTS);
                }

                cmd.opGraphicsBarrier();
            }

            cmd.opEndPass();
        }

    } // test
} // rendering;