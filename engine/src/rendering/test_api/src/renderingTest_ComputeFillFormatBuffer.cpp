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
        /// test of the compute shader access to storage image
        class RenderingTest_ComputeFillFormatBuffer : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_ComputeFillFormatBuffer, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth) override final;

        private:
            const ShaderLibrary* m_shaderGenerate;
            const ShaderLibrary* m_shaderDraw;

            BufferView m_vertexBuffer;
            BufferView m_tempBuffer;
            uint32_t m_vertexCount;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_ComputeFillFormatBuffer);
            RTTI_METADATA(RenderingTestOrderMetadata).order(2200);
        RTTI_END_TYPE();

        //---       

        namespace
        {
            struct TestConsts
            {
                uint32_t SideCount;
                uint32_t FrameIndex;
            };

            struct TestParamsWrite
            {
                ConstantsView Consts;
                BufferView ColorsWrite;
            };

            struct TestParamsRead
            {
                ConstantsView Consts;
                BufferView ColorsRead;
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

        void RenderingTest_ComputeFillFormatBuffer::initialize()
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

            // create temp buffer
            {

                base::Array<base::Color> initialData;
                initialData.resize(512*512);

                for (uint32_t y=0; y<512; ++y)
                    for (uint32_t x=0; x<512; ++x)
                        initialData[x+y*512] = base::Color(x/2, y/2, 0);

                rendering::BufferCreationInfo info;
                info.allowShaderReads = true;
                info.allowUAV = true;
                info.size = 512 * 512 * 4;

                auto sourceData = CreateSourceData(initialData);
                m_tempBuffer = createBuffer(info, &sourceData);
            }

            m_shaderGenerate = loadShader("ComputeFillFormatBufferGenerate.csl");
            m_shaderDraw = loadShader("ComputeFillFormatBufferTest.csl");
        }

        void RenderingTest_ComputeFillFormatBuffer::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth)
        {
            uint32_t SIDE_RESOLUTION = 512;

            TransientBufferView storageBuffer(BufferViewFlag::ShaderReadable, TransientBufferAccess::ShaderReadWrite,  SIDE_RESOLUTION * SIDE_RESOLUTION * 4);
            cmd.opAllocTransientBuffer(storageBuffer);

            TestConsts tempConsts;
            tempConsts.SideCount = SIDE_RESOLUTION;
            tempConsts.FrameIndex = (int)(time * 60.0f);
            auto uploadedConsts = cmd.opUploadConstants(tempConsts);

            {
                TestParamsWrite params;
                params.Consts = uploadedConsts;
                params.ColorsWrite = storageBuffer;
                cmd.opBindParametersInline("TestParamsWrite"_id, params);
                cmd.opDispatch(m_shaderGenerate, SIDE_RESOLUTION / 8, SIDE_RESOLUTION / 8);
            }

            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);

            cmd.opGraphicsBarrier();

            {
                TestParamsRead tempParams;
                tempParams.Consts = uploadedConsts;
                tempParams.ColorsRead = storageBuffer;
                cmd.opBindParametersInline("TestParamsRead"_id, tempParams);

                cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBuffer);
                cmd.opDraw(m_shaderDraw, 0, 6);

                cmd.opEndPass();
            }

            cmd.opGraphicsBarrier();
        }

    } // test
} // rendering;