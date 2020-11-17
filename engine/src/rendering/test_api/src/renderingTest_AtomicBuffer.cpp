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
        /// test of the atomic writes to the storage buffer
        class RenderingTest_AtomicBasicOps : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_AtomicBasicOps, IRenderingTest);

        public:
            RenderingTest_AtomicBasicOps();
            virtual void initialize() override final;
            
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth) override final;

        private:
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
            };
        }

        void RenderingTest_AtomicBasicOps::initialize()
        {
            base::StringView name = "AtomicBufferIncrement.csl";
                        
            switch (subTestIndex())
            {
            case 1: name = "AtomicBufferDecrement.csl"; break;
            case 2: name = "AtomicBufferAdd.csl"; break;
            case 3: name = "AtomicBufferSubtract.csl"; break;
            case 4: name = "AtomicBufferMin.csl"; break;
            case 5: name = "AtomicBufferMax.csl"; break;
            case 6: name = "AtomicBufferOr.csl"; break;
            case 7: name = "AtomicBufferAnd.csl"; break;
            case 8: name = "AtomicBufferXor.csl"; break;
            case 9: name = "AtomicBufferExchange.csl"; break;
            case 10: name = "AtomicBufferCompSwap.csl"; break;
            }

            m_shaderClear = loadShader("AtomicBufferClear.csl");
            m_shaderDraw = loadShader("AtomicBufferDraw.csl");
            m_shaderGenerate = loadShader(name);

            spawnTris();
        }

        static float RandOne()
        {
            return (float)rand() / (float)RAND_MAX;
        }

        static float RandRange(float min, float max)
        {
            return min + (max-min) * RandOne();
        }

        void RenderingTest_AtomicBasicOps::spawnTris()
        {
            static auto NUM_TRIS = 256;

            srand(0);
            for (uint32_t i=0; i<NUM_TRIS; ++i)
            {
                auto& tri = m_tris.emplaceBack();
                tri.radius = 0.02f + 0.2f * RandOne();
                tri.color = base::Color::FromVectorLinear(base::Vector4(RandRange(0.2f, 1.0f), RandRange(0.2f, 1.0f), RandRange(0.2f, 1.0f), 1.0f));
                tri.center.x = RandRange(-0.95f + tri.radius, 0.95f - tri.radius);
                tri.center.y = RandRange(-0.95f + tri.radius, 0.65f - tri.radius);
                tri.speed = RandRange(-0.1f, 0.1f);
                tri.value = i;
            }
        }

        void RenderingTest_AtomicBasicOps::renderLine(command::CommandWriter& cmd, const ShaderLibrary* func) const
        {
            base::Array<Simple3DVertex> verts;
            verts.resize(1024);

            auto writeVertex  = verts.typedData();
            for (uint32_t i=0; i<verts.size(); ++i, ++writeVertex)
            {
                float x = i / (float)verts.size();
                writeVertex->set(-1.0f + 2.0f*x, 0.95f, 0.5f, x, 0.2f, base::Color::WHITE);
            }

            TransientBufferView tempVerticesBuffer(BufferViewFlag::Vertex, TransientBufferAccess::NoShaders, verts.dataSize());
            cmd.opSetPrimitiveType(PrimitiveTopology::LineStrip);
            cmd.opAllocTransientBufferWithData(tempVerticesBuffer, verts.data(), verts.dataSize());
            cmd.opBindVertexBuffer("Simple3DVertex"_id,  tempVerticesBuffer);
            cmd.opDraw(func, 0, verts.size());
        }

        void RenderingTest_AtomicBasicOps::renderTris(command::CommandWriter& cmd, float timeOffset, const ShaderLibrary* func) const
        {
            if (m_tris.empty())
                return;

            static const float PHASE_A = 0.0f;
            static const float PHASE_B = DEG2RAD * 120.0f;
            static const float PHASE_C = DEG2RAD * 240.0f;

            // allocate buffer fo rendering vertices
            base::Array<Simple3DVertex> verts;
            verts.resize(m_tris.size() * 3);

            // generate renderable geometry
            auto writeVertex  = verts.typedData();
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

            // upload and render
            TransientBufferView tempVerticesBuffer(BufferViewFlag::Vertex, TransientBufferAccess::NoShaders, verts.dataSize());
            cmd.opAllocTransientBufferWithData(tempVerticesBuffer, verts.data(), verts.dataSize());
            cmd.opBindVertexBuffer("Simple3DVertex"_id,  tempVerticesBuffer);
            cmd.opDraw(func, 0, verts.size());
        }

        void RenderingTest_AtomicBasicOps::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth)
        {
            static const uint32_t MAX_ELEMENTS = 1024;

            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);

            TransientBufferView storageBuffer(BufferViewFlag::ShaderReadable, TransientBufferAccess::ShaderReadWrite, MAX_ELEMENTS * 4);
            cmd.opAllocTransientBuffer(storageBuffer);

            // fence
            cmd.opGraphicsBarrier();

            // clear
            {
                TestParams tempParams;
                tempParams.BufferData = storageBuffer;
                cmd.opBindParametersInline("TestParams"_id, tempParams);
                cmd.opDispatch(m_shaderClear, 1024/8,1,1);
            }

            // draw
            {
                TestParams tempParams;
                tempParams.BufferData = storageBuffer;
                cmd.opBindParametersInline("TestParams"_id, tempParams);

                renderTris(cmd, 10.0f + time, m_shaderGenerate);

                cmd.opGraphicsBarrier();

                renderLine(cmd, m_shaderDraw);

                cmd.opEndPass();
            }
        }

    } // test
} // rendering