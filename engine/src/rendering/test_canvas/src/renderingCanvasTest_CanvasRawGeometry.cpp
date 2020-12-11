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
#include "base/canvas/include/canvasStorage.h"

namespace rendering
{
    namespace test
    {
        /// test of lines of different width
        class SceneTest_CanvasRawGeometry : public ICanvasTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_CanvasRawGeometry, ICanvasTest);

        public:
            base::NativeTimePoint m_lastUpdateTime;

			base::canvas::ImageAtlasIndex m_atlas;
			base::canvas::ImageEntry m_atlasEntry; 

            virtual void initialize() override
            {
                m_lastUpdateTime.resetToNow();

                auto lena = loadImage("lena.png");

				m_atlas = m_storage->createAtlas(1024, 1, "TestAtlas");
				m_atlasEntry = m_storage->registerImage(m_atlas, lena, true);
            }

			virtual void shutdown() override
			{
				m_storage->destroyAtlas(m_atlas);
			}

            static float CalcPlasma(float x, float y, float t)
            {
                float h = 20;
                float fx = x * h - h / 2;
                float fy = y * h - h / 2;

                float v = 0.0;
                v = v + sin((fx + t));
                v = v + sin((fy + t) / 2.0);
                v = v + sin((fx + fy + t) / 2.0);

                fx = fx + sin(t / 3.0) + h / 2;
                fy = fy + cos(t / 2.0) + h / 2;

                v = v + sin(sqrt(fx * fx + fy * fy + 1.0) + t);
                v = v / 2.0;

                return (1.0f + sin(PI * v)) * 0.5f;
            }

            virtual void render(base::canvas::Canvas& c) override
            {
                const auto GRID_SIZE = 32;

                const auto t = m_lastUpdateTime.timeTillNow().toSeconds();

                base::Array<base::canvas::Vertex> vertices;
                base::Array<uint16_t> indices;

                vertices.resize((GRID_SIZE + 1) * (GRID_SIZE + 1));
                indices.resize((GRID_SIZE) * (GRID_SIZE) * 6);

                const float fracStep = 1.0f / GRID_SIZE;

                float fracY = 0.0f;
                float imageX = 20.0f;
                float imageY = 20.0f;
                float imageW = 960.0f;
                float imageH = 960.0f;

                float displacementSizeX = (imageW / GRID_SIZE) * 0.5f;
                float displacementSizeY = (imageH / GRID_SIZE) * 0.5f;

                auto* writeVertex = vertices.typedData();
                for (uint32_t y = 0; y <= GRID_SIZE; ++y, fracY += fracStep)
                {
                    float fracX = 0.0f;
                    for (uint32_t x = 0; x <= GRID_SIZE; ++x, fracX += fracStep, ++writeVertex)
                    {
                        float dx = cosf(fracX*4.44 + 0.123 + t * 1.663) * cosf(fracY * 5.37 + 0.523 + t * 1.266);
                        float dy = cosf(fracX*4.48 + 0.321 + t * 1.345) * cosf(fracY * 5.37 + 0.523 + t * 1.688);

                        writeVertex->pos.x = imageX + fracX * imageW + (dx * displacementSizeX);
                        writeVertex->pos.y = imageY + fracY * imageH + (dy * displacementSizeY);

                        writeVertex->uv.x = fracX;
                        writeVertex->uv.y = fracY;

                        const auto color = base::FloatTo255(CalcPlasma(fracX, fracY, t * 5.0f));
                        writeVertex->color.r = color;
                        writeVertex->color.g = color;
                        writeVertex->color.b = color;
                        writeVertex->color.a = 255;
                    }
                }

                auto* writeIndex = indices.typedData();
                for (uint32_t y = 0; y < GRID_SIZE; ++y)
                {
                    uint16_t prev = y * (GRID_SIZE + 1);
                    uint16_t cur = prev + (GRID_SIZE + 1);
                    for (uint32_t x = 0; x < GRID_SIZE; ++x, ++cur, ++prev)
                    {
                        writeIndex[0] = prev + 0;
                        writeIndex[1] = prev + 1;
                        writeIndex[2] = cur + 1;
                        writeIndex[3] = prev + 0;
                        writeIndex[4] = cur + 1;
                        writeIndex[5] = cur + 0;
                        writeIndex += 6;
                    }
                }

				{
					auto style = base::canvas::ImagePattern(base::canvas::ImagePatternSettings(m_atlasEntry).customUV());

					base::canvas::Geometry geometry;
					geometry.appendIndexedBatch(vertices.typedData(), indices.typedData(), indices.size(), base::canvas::Batch(), &style, m_storage);

					c.place(base::Vector2(0, 0), geometry);
				}
            }
        };

        RTTI_BEGIN_TYPE_CLASS(SceneTest_CanvasRawGeometry);
            RTTI_METADATA(CanvasTestOrderMetadata).order(105);
        RTTI_END_TYPE();

        //---      

    } // test
} // rendering