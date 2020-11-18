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
        /// test of the atomic writes to the storage buffer
        class RenderingTest_AtomicTexture : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_AtomicTexture, IRenderingTest);

        public:
            RenderingTest_AtomicTexture();
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

            ImageView m_atomicTexture;
            const ShaderLibrary* m_shaderClear;
            const ShaderLibrary* m_shaderGenerate;
            const ShaderLibrary* m_shaderDraw;

            void spawnTris();
            void renderTris(command::CommandWriter& cmd, float timeOffset, const ShaderLibrary* func) const;
            void renderLine(command::CommandWriter& cmd, const ShaderLibrary* func) const;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_AtomicTexture);
            RTTI_METADATA(RenderingTestOrderMetadata).order(2151);
            RTTI_METADATA(RenderingTestSubtestCountMetadata).count(11);
        RTTI_END_TYPE();

        //---       

        RenderingTest_AtomicTexture::RenderingTest_AtomicTexture()
        {
        }

        namespace
        {

            struct TestParams
            {
                ImageView TextureData;
            };
        }

        void RenderingTest_AtomicTexture::initialize()
        {
            base::StringView name = "AtomicTextureIncrement.csl";

            switch (subTestIndex())
            {
            case 1: name = "AtomicTextureDecrement.csl"; break;
            case 2: name = "AtomicTextureAdd.csl"; break;
            case 3: name = "AtomicTextureSubtract.csl"; break;
            case 4: name = "AtomicTextureMin.csl"; break;
            case 5: name = "AtomicTextureMax.csl"; break;
            case 6: name = "AtomicTextureOr.csl"; break;
            case 7: name = "AtomicTextureAnd.csl"; break;
            case 8: name = "AtomicTextureXor.csl"; break;
            case 9: name = "AtomicTextureExchange.csl"; break;
            case 10: name = "AtomicTextureCompSwap.csl"; break;
            }

            m_shaderClear = loadShader("AtomicTextureClear.csl");
            m_shaderGenerate = loadShader(name);
            m_shaderDraw = loadShader("AtomicTextureDraw.csl");

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

        void RenderingTest_AtomicTexture::spawnTris()
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

        void RenderingTest_AtomicTexture::renderLine(command::CommandWriter& cmd, const ShaderLibrary* func) const
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
            cmd.opAllocTransientBufferWithData(tempVerticesBuffer, verts.data(), verts.dataSize());
            cmd.opBindVertexBuffer("Simple3DVertex"_id,  tempVerticesBuffer);
            cmd.opDraw(func, 0, verts.size());
        }

        void RenderingTest_AtomicTexture::renderTris(command::CommandWriter& cmd, float timeOffset, const ShaderLibrary* func) const
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

        void RenderingTest_AtomicTexture::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth)
        {
            static const uint32_t MAX_ELEMENTS = 1024;

            auto halfWidth = base::Align( (1+backBufferView.width()) / 2, 8);
            auto halfHeight = base::Align((1 + backBufferView.height()) / 2, 8);
            if (m_atomicTexture.width() != halfWidth || m_atomicTexture.height() != halfHeight)
            {
                rendering::ImageCreationInfo colorBuffer;
                colorBuffer.allowUAV = true;
                colorBuffer.allowShaderReads = true;
                colorBuffer.format = rendering::ImageFormat::R32_INT;
                colorBuffer.width = halfWidth;
                colorBuffer.height = halfHeight;
                m_atomicTexture = createImage(colorBuffer);
            }

            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);

            cmd.opGraphicsBarrier();

            // clear
            {
                TestParams tempParams;
                tempParams.TextureData = m_atomicTexture;
                cmd.opBindParametersInline("TestParams"_id, tempParams);
                cmd.opDispatch(m_shaderClear, halfWidth/8, halfHeight/8, 1);
            }

            // generate
            {
                TestParams tempParams;
                tempParams.TextureData = m_atomicTexture;
                cmd.opBindParametersInline("TestParams"_id, tempParams);

                renderTris(cmd, 10.0f + time, m_shaderGenerate);

                cmd.opGraphicsBarrier();

                renderLine(cmd, m_shaderDraw);
            }

            cmd.opEndPass();
        }

    } // test
} // rendering