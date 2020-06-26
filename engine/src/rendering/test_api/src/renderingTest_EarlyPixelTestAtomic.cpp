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
        /// test of the early pixel rejection
        class RenderingTest_EarlyPixelTestAtomic : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_EarlyPixelTestAtomic, IRenderingTest);

        public:
            RenderingTest_EarlyPixelTestAtomic();
            virtual void initialize() override final;
            
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth) override final;

        private:
            const ShaderLibrary* m_shaderClear;
            const ShaderLibrary* m_shaderOccluder;
            const ShaderLibrary* m_shaderGenerateA;
            const ShaderLibrary* m_shaderGenerateB;
            const ShaderLibrary* m_shaderDraw;

            void renderLine(command::CommandWriter& cmd, const ShaderLibrary* func, base::Color color) const;
            void renderQuad(command::CommandWriter& cmd, const ShaderLibrary* func, float x, float y, float z, float w, float h, base::Color color) const;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_EarlyPixelTestAtomic);
            RTTI_METADATA(RenderingTestOrderMetadata).order(2155);
            RTTI_METADATA(RenderingTestSubtestCountMetadata).count(2);
        RTTI_END_TYPE();

        //---       

        RenderingTest_EarlyPixelTestAtomic::RenderingTest_EarlyPixelTestAtomic()
        {
        }

        namespace
        {

            struct TestParams
            {
                BufferView BufferData;
            };
        }

        void RenderingTest_EarlyPixelTestAtomic::initialize()
        {
            m_shaderClear = loadShader("EarlyFragmentTestsClear.csl");
            m_shaderOccluder = loadShader("EarlyFragmentTestsOccluder.csl");
            m_shaderGenerateA = loadShader("EarlyFragmentTestsGenerateNormal.csl");
            m_shaderGenerateB = loadShader("EarlyFragmentTestsGenerateEarlyTests.csl");
            m_shaderDraw = loadShader("EarlyFragmentTestsDraw.csl");
        }

        void RenderingTest_EarlyPixelTestAtomic::renderLine(command::CommandWriter& cmd, const ShaderLibrary* func, base::Color color) const
        {
            base::Array<Simple3DVertex> verts;
            verts.resize(1024);

            auto writeVertex  = verts.typedData();
            for (uint32_t i=0; i<verts.size(); ++i, ++writeVertex)
            {
                float x = i / (float)verts.size();
                writeVertex->set(-1.0f + 2.0f*x, 0.95f, 0.5f, x, 0.2f, color);
            }

            TransientBufferView tempVerticesBuffer(BufferViewFlag::Vertex, TransientBufferAccess::NoShaders, verts.dataSize());
            cmd.opSetPrimitiveType(PrimitiveTopology::LineStrip);
            cmd.opAllocTransientBufferWithData(tempVerticesBuffer, verts.data(), verts.dataSize());
            cmd.opBindVertexBuffer("Simple3DVertex"_id,  tempVerticesBuffer);
            cmd.opDraw(func, 0, verts.size());
        }

        void RenderingTest_EarlyPixelTestAtomic::renderQuad(command::CommandWriter& cmd, const ShaderLibrary* func, float x, float y, float z, float w, float h, base::Color color) const
        {
            Simple3DVertex verts[6];
            verts[0] = Simple3DVertex(x, y, z, 0.0f, 0.0f, color);
            verts[1] = Simple3DVertex(x + w, y, z, 1.0f, 0.0f, color);
            verts[2] = Simple3DVertex(x + w, y + h, z, 1.0f, 1.0f, color);
            verts[3] = Simple3DVertex(x, y, z, 0.0f, 0.0f, color);
            verts[4] = Simple3DVertex(x + w, y + h, z, 1.0f, 1.0f, color);
            verts[5] = Simple3DVertex(x, y + h, z, 0.0f, 1.0f, color);

            TransientBufferView tempVerticesBuffer(BufferViewFlag::Vertex, TransientBufferAccess::NoShaders, sizeof(verts));
            cmd.opAllocTransientBufferWithData(tempVerticesBuffer, verts, sizeof(verts));
            cmd.opBindVertexBuffer("Simple3DVertex"_id,  tempVerticesBuffer);
            cmd.opDraw(func, 0, ARRAY_COUNT(verts));
        }

        void RenderingTest_EarlyPixelTestAtomic::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth)
        {
            static const uint32_t MAX_ELEMENTS = 1024;

            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));
            fb.depth.view(depth).clearDepth(1.0f).clearStencil(0);

            cmd.opBeingPass(fb);

            cmd.opSetDepthState(true, true, CompareOp::Less);

            TransientBufferView storageBufferA(BufferViewFlag::ShaderReadable, TransientBufferAccess::ShaderReadWrite, MAX_ELEMENTS * 4);
            TransientBufferView storageBufferB(BufferViewFlag::ShaderReadable, TransientBufferAccess::ShaderReadWrite, MAX_ELEMENTS * 4);
            cmd.opAllocTransientBuffer(storageBufferA);
            cmd.opAllocTransientBuffer(storageBufferB);

            // fence
            cmd.opGraphicsBarrier();

            // clear A
            {
                TestParams tempParams;
                tempParams.BufferData = storageBufferA;
                cmd.opBindParametersInline("TestParams"_id, tempParams);
                cmd.opDispatch(m_shaderClear, 1024/8,1,1);
            }

            // clear B
            {
                TestParams tempParams;
                tempParams.BufferData = storageBufferB;
                cmd.opBindParametersInline("TestParams"_id, tempParams);
                cmd.opDispatch(m_shaderClear, 1024 / 8, 1, 1);
            }

            // draw test rect A - no early tests
            const float h = 0.1f;
            const float y1 = -0.15f;
            const float y2 = 0.05f;

            const float oy = -0.2f;
            const float oh = 0.4f;
            const float x1 = 0.7f * sin(time * 1.0f);
            const float x2 = 0.8f * sin(time * 0.4f);
            const float x3 = 0.9f * sin(time * 0.1f);
            const float w = 0.3f;

            // draw occludes
            {
                renderQuad(cmd, m_shaderOccluder, -0.2f, oy, 0.1f, 0.4f, oh, base::Color(70, 70, 70, 255));
                renderQuad(cmd, m_shaderOccluder, -0.6f, oy, 0.1f, 0.05f, oh, base::Color(70, 70, 70, 255));
                renderQuad(cmd, m_shaderOccluder, -0.7f, oy, 0.1f, 0.01f, oh, base::Color(70, 70, 70, 255));
                renderQuad(cmd, m_shaderOccluder, 0.3f, oy, 0.1f, 0.1f, oh, base::Color(70, 70, 70, 255));
                renderQuad(cmd, m_shaderOccluder, 0.5f, oy, 0.1f, 0.01f, oh, base::Color(70, 70, 70, 255));
                renderQuad(cmd, m_shaderOccluder, 0.7f, oy, 0.1f, 0.001f, oh, base::Color(70, 70, 70, 255));
            }

            {
                TestParams tempParams;
                tempParams.BufferData = storageBufferA;
                cmd.opBindParametersInline("TestParams"_id, tempParams);
                renderQuad(cmd, m_shaderGenerateA, x1, y1, 0.5f, w, h, base::Color(255, 0, 0, 255));
                if (subTestIndex() >= 1)
                    renderQuad(cmd, m_shaderGenerateA, x2, y1, 0.5f, w, h, base::Color(255, 0, 0, 255));
                if (subTestIndex() >= 2)
                    renderQuad(cmd, m_shaderGenerateA, x3, y1, 0.5f, w, h, base::Color(255, 0, 0, 255));
            }

            {
                TestParams tempParams;
                tempParams.BufferData = storageBufferB;
                cmd.opBindParametersInline("TestParams"_id, tempParams);
                renderQuad(cmd, m_shaderGenerateB, x1, y2, 0.5f, w, h*1.01f, base::Color(0, 255, 0, 255));
                if (subTestIndex() >= 1)
                    renderQuad(cmd, m_shaderGenerateB, x2, y2, 0.5f, w, h * 1.01f, base::Color(0, 255, 0, 255));
                if (subTestIndex() >= 2)
                    renderQuad(cmd, m_shaderGenerateB, x3, y2, 0.5f, w, h * 1.01f, base::Color(0, 255, 0, 255));
            }

            cmd.opGraphicsBarrier();

            // draw
            {
                TestParams tempParams;
                tempParams.BufferData = storageBufferA;
                cmd.opBindParametersInline("TestParams"_id, tempParams);
                renderLine(cmd, m_shaderDraw, base::Color::RED);

                tempParams.BufferData = storageBufferB;
                cmd.opBindParametersInline("TestParams"_id, tempParams);
                renderLine(cmd, m_shaderDraw, base::Color::GREEN);
            }

            cmd.opEndPass();
        }

    } // test
} // rendering