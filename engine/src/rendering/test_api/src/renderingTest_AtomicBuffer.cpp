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
        /// test of the atomic writes to the storage buffer
        class RenderingTest_AtomicBasicOps : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_AtomicBasicOps, IRenderingTest);

        public:
            RenderingTest_AtomicBasicOps();
            virtual void initialize() override final;
            
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth) override final;

        private:
            static const uint32_t NUM_LINES = 1024;
            static const uint32_t NUM_TRIS = 1024;

            float m_drawScale = 30000.0;
            float m_clearValue = 0.0f;

            struct Tri
            {
                base::Vector3 center;
                base::Color color;
                float phase;
                float radius;
                float speed;
                int value;
            };

            base::Array<Tri> m_tris;

            const ShaderLibrary* m_shaderClear;
            const ShaderLibrary* m_shaderGenerate;
            const ShaderLibrary* m_shaderDraw;

            BufferView m_tempLineBuffer;
            BufferView m_tempTrisBuffer;
            BufferView m_storageBuffer;

            void spawnTris();
            void renderTris(command::CommandWriter& cmd, float timeOffset, const ShaderLibrary* func) const;
            void renderLine(command::CommandWriter& cmd, const ShaderLibrary* func) const;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_AtomicBasicOps);
            RTTI_METADATA(RenderingTestOrderMetadata).order(2150);
            RTTI_METADATA(RenderingTestSubtestCountMetadata).count(11);
        RTTI_END_TYPE();

        //---       

        RenderingTest_AtomicBasicOps::RenderingTest_AtomicBasicOps()
        {
        }

        namespace
        {

            struct TestParams
            {
                BufferView BufferData;
                ConstantsView Costs;
            };
        }

        void RenderingTest_AtomicBasicOps::initialize()
        {
            base::StringView name = "AtomicBufferIncrement.csl";
                        
            switch (subTestIndex())
            {
            case 1: name = "AtomicBufferDecrement.csl"; m_clearValue = 30000.0f; break;
            case 2: name = "AtomicBufferAdd.csl"; m_drawScale = 5000000.0f; break;
            case 3: name = "AtomicBufferSubtract.csl"; m_clearValue = 5000000.0f; m_drawScale = 5000000.0f; break;
            case 4: name = "AtomicBufferMin.csl"; m_clearValue = 1024; m_drawScale = 1024; break;
            case 5: name = "AtomicBufferMax.csl"; m_drawScale = 1024; break;
            case 6: name = "AtomicBufferOr.csl"; m_drawScale = 1024; break;
            case 7: name = "AtomicBufferAnd.csl"; m_clearValue = 1023; m_drawScale = 1024; break;
            case 8: name = "AtomicBufferXor.csl"; m_clearValue = 1023; m_drawScale = 1024; break;
            case 9: name = "AtomicBufferExchange.csl"; m_drawScale = 1024; break;
            case 10: name = "AtomicBufferCompSwap.csl"; m_drawScale = 1024; break;
            }

            m_shaderClear = loadShader("AtomicBufferClear.csl");
            m_shaderDraw = loadShader("AtomicBufferDraw.csl");
            m_shaderGenerate = loadShader(name);

            m_tempLineBuffer = createVertexBuffer(NUM_LINES * sizeof(Simple3DVertex), nullptr);
            m_tempTrisBuffer = createVertexBuffer(NUM_TRIS * sizeof(Simple3DVertex) * 3, nullptr);

            m_storageBuffer = createStorageBuffer(NUM_LINES * 4);

            spawnTris();
        }

        void RenderingTest_AtomicBasicOps::spawnTris()
        {
            base::MTRandState rnd;

            for (uint32_t i=0; i<NUM_TRIS; ++i)
            {
                auto& tri = m_tris.emplaceBack();
                tri.radius = 0.02f + 0.2f * rnd.unit();
                tri.color = base::Color::FromVectorLinear(base::Vector4(rnd.range(0.2f, 1.0f), rnd.range(0.2f, 1.0f), rnd.range(0.2f, 1.0f), 1.0f));
                tri.center.x = rnd.range(-0.95f + tri.radius, 0.95f - tri.radius);
                tri.center.y = rnd.range(-0.95f + tri.radius, 0.65f - tri.radius);
                tri.speed = rnd.range(-0.1f, 0.1f);
                tri.value = i;
            }
        }

        void RenderingTest_AtomicBasicOps::renderLine(command::CommandWriter& cmd, const ShaderLibrary* func) const
        {
            auto* writeVertex = cmd.opUpdateDynamicBufferPtrN<Simple3DVertex>(m_tempLineBuffer, 0, NUM_LINES);
            for (uint32_t i=0; i< NUM_LINES; ++i, ++writeVertex)
            {
                float x = i / (float)NUM_LINES;
                writeVertex->set(-1.0f + 2.0f*x, 0.95f, 0.5f, x, 0.2f, base::Color::WHITE);
            }

            cmd.opSetPrimitiveType(PrimitiveTopology::LineStrip);
            cmd.opBindVertexBuffer("Simple3DVertex"_id, m_tempLineBuffer);
            cmd.opDraw(func, 0, NUM_LINES);
        }

        void RenderingTest_AtomicBasicOps::renderTris(command::CommandWriter& cmd, float timeOffset, const ShaderLibrary* func) const
        {
            static const float PHASE_A = 0.0f;
            static const float PHASE_B = DEG2RAD * 120.0f;
            static const float PHASE_C = DEG2RAD * 240.0f;

            auto* writeVertex = cmd.opUpdateDynamicBufferPtrN<Simple3DVertex>(m_tempTrisBuffer, 0, NUM_TRIS * 3);
            for (auto& tri : m_tris)
            {
                auto phase = timeOffset * tri.speed;

                auto ax = tri.center.x + tri.radius * cos(phase + PHASE_A);
                auto ay = tri.center.y + tri.radius * sin(phase + PHASE_A);
                auto bx = tri.center.x + tri.radius * cos(phase + PHASE_B);
                auto by = tri.center.y + tri.radius * sin(phase + PHASE_B);
                auto cx = tri.center.x + tri.radius * cos(phase + PHASE_C);
                auto cy = tri.center.y + tri.radius * sin(phase + PHASE_C);

                writeVertex[0].set(ax, ay, tri.center.z, 0.0f, 0.0f, tri.color);
                writeVertex[1].set(bx, by, tri.center.z, 0.0f, 0.0f, tri.color);
                writeVertex[2].set(cx, cy, tri.center.z, 0.0f, 0.0f, tri.color);
                writeVertex += 3;
            }

            cmd.opBindVertexBuffer("Simple3DVertex"_id, m_tempTrisBuffer);
            cmd.opDraw(func, 0, NUM_TRIS * 3);
        }

        void RenderingTest_AtomicBasicOps::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth)
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);

            // fence
            cmd.opGraphicsBarrier();

            // clear
            {
                TestParams tempParams;
                tempParams.BufferData = m_storageBuffer;
                tempParams.Costs = cmd.opUploadConstants(m_clearValue);
                cmd.opBindParametersInline("TestParams"_id, tempParams);
                cmd.opDispatch(m_shaderClear, 1024/8,1,1);
            }

            // draw
            {
                TestParams tempParams;
                tempParams.BufferData = m_storageBuffer;
                tempParams.Costs = cmd.opUploadConstants(m_drawScale);
                cmd.opBindParametersInline("TestParams"_id, tempParams);

                renderTris(cmd, 10.0f + time, m_shaderGenerate);

                cmd.opGraphicsBarrier();

                renderLine(cmd, m_shaderDraw);

                cmd.opEndPass();
            }
        }

    } // test
} // rendering