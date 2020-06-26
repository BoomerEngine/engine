/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/

#include "build.h"
#include "renderingCanvasTest.h"

#include "base/canvas/include/canvasGeometryBuilder.h"
#include "base/canvas/include/canvasGeometry.h"
#include "base/canvas/include/canvas.h"
#include "base/canvas/include/canvasStyle.h"

#include "rendering/driver/include/renderingShaderLibrary.h"
#include "rendering/driver/include/renderingCommandWriter.h"
#include "rendering/canvas/include/renderingCanvasRendererCustomBatchHandler.h"

namespace rendering
{
    namespace test
    {

        //--

        base::res::StaticResource<ShaderLibrary> resCanvasCustomHandlerTestShaders("engine/shaders/canvas/canvas_mandelbrot.fx");

        /// custom rendering handler
        class SceneTest_MandelbrotCustomRenderer : public rendering::canvas::ICanvasRendererCustomBatchHandler
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_MandelbrotCustomRenderer, rendering::canvas::ICanvasRendererCustomBatchHandler);

        public:
            virtual void initialize(IDriver* drv) override final
            {
            }

            virtual void render(command::CommandWriter& cmd, const base::canvas::Canvas& canvas, const rendering::canvas::CanvasRenderingParams& params, uint32_t firstIndex, uint32_t numIndices, uint32_t numPayloads, const rendering::canvas::CanvasCustomBatchPayload* payloads) override
            {
                if (auto shader = resCanvasCustomHandlerTestShaders.loadAndGet())
                {
                    cmd.opDrawIndexed(shader, 0, firstIndex, numIndices);
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(SceneTest_MandelbrotCustomRenderer);
        RTTI_END_TYPE();

        //--

        /// test of custom drawer functionality
        class SceneTest_CanvasCustomDrawer : public ICanvasTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_CanvasCustomDrawer, ICanvasTest);

        public:
            virtual void initialize() override
            {
                m_time.resetToNow();
            }

            static void SetupQuad(base::canvas::Canvas::RawVertex* vertices, uint16_t* indices, float x, float y, float w, float h, float u0, float v0, float u1, float v1)
            {
                vertices[0].pos.x = x;
                vertices[0].pos.y = y;
                vertices[1].pos.x = x + w;
                vertices[1].pos.y = y;
                vertices[2].pos.x = x + w;
                vertices[2].pos.y = y + h;
                vertices[3].pos.x = x;
                vertices[3].pos.y = y + h;

                vertices[0].color = base::Color::WHITE;
                vertices[1].color = base::Color::WHITE;
                vertices[2].color = base::Color::WHITE;
                vertices[3].color = base::Color::WHITE;

                vertices[0].uv.x = u0;
                vertices[0].uv.y = v0;
                vertices[1].uv.x = u1;
                vertices[1].uv.y = v0;
                vertices[2].uv.x = u1;
                vertices[2].uv.y = v1;
                vertices[3].uv.x = u0;
                vertices[3].uv.y = v1;

                indices[0] = 0;
                indices[1] = 1;
                indices[2] = 2;
                indices[3] = 0;
                indices[4] = 2;
                indices[5] = 3;
            }

            virtual void render(base::canvas::Canvas& c) override
            {
                float time = m_time.timeTillNow().toSeconds();

                const float u0 = -2.0f;
                const float v0 = -2.0f;
                const float u1 = 2.0f;
                const float v1 = 2.0f;

                base::canvas::Canvas::RawVertex vertices[4];
                uint16_t indices[6];
                SetupQuad(vertices, indices, 100.0f, 100.0f, 512.0f, 512.0f, u0, v0, u1, v1);

                auto style = base::canvas::SolidColor(base::Color::WHITE);
                style.customUV = true;

                {
                    const auto customDrawerId = rendering::canvas::GetHandlerIndex<SceneTest_MandelbrotCustomRenderer>();

                    base::canvas::Canvas::RawGeometry geom;
                    geom.vertices = vertices;
                    geom.indices = indices;
                    geom.numIndices = 6;
                    c.place(style, geom, customDrawerId);
                }
            }

        private:
            base::NativeTimePoint m_time;
        };

        RTTI_BEGIN_TYPE_CLASS(SceneTest_CanvasCustomDrawer);
            RTTI_METADATA(CanvasTestOrderMetadata).order(107);
        RTTI_END_TYPE();

        //--

    } // test
} // rendering