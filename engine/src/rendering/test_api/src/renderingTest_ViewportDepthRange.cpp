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
        /// test of the viuewport depth-range setting 
        class RenderingTest_ViewportDepthRange : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_ViewportDepthRange, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth) override final;

        private:
            BufferView m_vertexBuffer;
            ImageView m_depthBuffer;
            const ShaderLibrary* m_shaders;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_ViewportDepthRange);
        RTTI_METADATA(RenderingTestOrderMetadata).order(130);
        RTTI_END_TYPE();

        //---

        namespace
        {
            struct ViewportDepthRangeConsts
            {
                base::Vector4 DrawColor;
                base::Vector4 Scale;
                base::Vector4 Offset;
            };

            struct TestParams
            {
                ConstantsView m_data;
            };
        }

        void RenderingTest_ViewportDepthRange::initialize()
        {
            // create vertex buffer with a test geometry
            {
                Simple3DVertex vertices[6];
                vertices[0].set(0.0f, 0.0f, 0.0f);
                vertices[1].set(1.0f, 0.0f, 0.0f);
                vertices[2].set(1.0f, 1.0f, 0.0f);
                vertices[3].set(0.0f, 0.0f, 0.0f);
                vertices[4].set(1.0f, 1.0f, 0.0f);
                vertices[5].set(0.0f, 1.0f, 0.0f);

                rendering::BufferCreationInfo info;
                info.allowVertex = true;
                info.size = sizeof(vertices);

                auto sourceData = CreateSourceDataRaw(vertices);
                m_vertexBuffer = createBuffer(info, &sourceData);
            }

            m_shaders = loadShader("ViewportDepthRange.csl");
        }

        static void DrawGroup(command::CommandWriter& cmd, const ShaderLibrary* func, float startX, float y, float startZ, float endZ, float size, uint32_t count, const base::Vector4& color)
        {
            for (uint32_t i = 0; i < count; ++i)
            {
                float frac = i / (float)(count-1);

                ViewportDepthRangeConsts params;
                params.DrawColor = color;
                params.Scale.x = size;
                params.Scale.y = size;
                params.Scale.z = 0.0f;
                params.Offset.x = startX + frac * (1.6f);
                params.Offset.y = y;
                params.Offset.z = startZ + frac * (endZ - startZ);

                TestParams desc;
                desc.m_data = cmd.opUploadConstants(params);
                cmd.opBindParametersInline("TestParams"_id, desc);

                cmd.opDraw(func, 0, 6);
            }
        }

        void RenderingTest_ViewportDepthRange::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView)
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));
            fb.depth.view(backBufferDepthView).clearDepth(1.0f).clearStencil(0);

            cmd.opBeingPass(fb);

            cmd.opSetDepthState(true, true, CompareOp::LessEqual);

            cmd.opBindVertexBuffer("Simple3DVertex"_id, m_vertexBuffer);

            // normal Z
            float y = -0.9f;
            float x = -1.0f;
            {
                cmd.opSetViewportDepthRange(0, 0.0f, 1.0f);
                DrawGroup(cmd, m_shaders, x + 0.0f, y + 0.00f, 0.03f, 0.93f, 0.4f, 4, base::Vector4::EX());
                DrawGroup(cmd, m_shaders, x + 0.2f, y + 0.05f, 0.06f, 0.96f, 0.3f, 4, base::Vector4::EY());
                DrawGroup(cmd, m_shaders, x + 0.4f, y + 0.10f, 0.09f, 0.99f, 0.2f, 4, base::Vector4::EZ());
                y += 0.5f;
            }

            // inverted Z
            {
                cmd.opSetViewportDepthRange(0, 1.0f, 0.0f);
                DrawGroup(cmd, m_shaders, x + 0.0f, y + 0.00f, 0.03f, 0.93f, 0.4f, 4, base::Vector4::EX());
                DrawGroup(cmd, m_shaders, x + 0.2f, y + 0.05f, 0.06f, 0.96f, 0.3f, 4, base::Vector4::EY());
                DrawGroup(cmd, m_shaders, x + 0.4f, y + 0.10f, 0.09f, 0.99f, 0.2f, 4, base::Vector4::EZ());
                y += 0.5f;
            }

            // sorted Z ranges
            {
                cmd.opSetViewportDepthRange(0, 0.0f, 0.3f);
                DrawGroup(cmd, m_shaders, x + 0.0f, y + 0.00f, 0.03f, 0.93f, 0.4f, 4, base::Vector4::EX());
                cmd.opSetViewportDepthRange(0, 0.3f, 0.6f);
                DrawGroup(cmd, m_shaders, x + 0.2f, y + 0.05f, 0.06f, 0.96f, 0.3f, 4, base::Vector4::EY());
                cmd.opSetViewportDepthRange(0, 0.6f, 1.0f);
                DrawGroup(cmd, m_shaders, x + 0.4f, y + 0.10f, 0.09f, 0.99f, 0.2f, 4, base::Vector4::EZ());
                y += 0.5f;
            }

            // inverted Z ranges
            {
                cmd.opSetViewportDepthRange(0, 0.6f, 1.0f);
                DrawGroup(cmd, m_shaders, x + 0.0f, y + 0.00f, 0.03f, 0.93f, 0.4f, 4, base::Vector4::EX());
                cmd.opSetViewportDepthRange(0, 0.3f, 0.6f);
                DrawGroup(cmd, m_shaders, x + 0.2f, y + 0.05f, 0.06f, 0.96f, 0.3f, 4, base::Vector4::EY());
                cmd.opSetViewportDepthRange(0, 0.0f, 0.3f);
                DrawGroup(cmd, m_shaders, x + 0.4f, y + 0.10f, 0.09f, 0.99f, 0.2f, 4, base::Vector4::EZ());
                y += 0.5f;
            }

            cmd.opEndPass();
        }

    } // test
} // rendering