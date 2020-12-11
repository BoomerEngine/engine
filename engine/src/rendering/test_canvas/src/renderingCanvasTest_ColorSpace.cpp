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

namespace rendering
{
    namespace test
    {
        /// test of color space stuff
        class SceneTest_CanvasColorSpace : public ICanvasTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_CanvasColorSpace, ICanvasTest);

        public:
            virtual void initialize() override
            {
            }

            virtual void render(base::canvas::Canvas& c) override
            {
				base::canvas::Geometry g;

				{
					base::canvas::GeometryBuilder b(m_storage, g);

					b.antialiasing(true);

					CanvasGridBuilder grid(1, 12, 20, c.width(), c.height());

					auto middleGray = base::FloatTo255(pow(0.5f, 1.0f / 2.2f));

					// WHITE

					// linear gradient
					{
						auto r = grid.cell();

						b.pushTransform();
						b.translatei(r.left(), r.top());

						b.beginPath();
						b.recti(0, 0, r.width(), r.height());
						b.fillPaint(base::canvas::LinearGradienti(0, r.centerY(), r.width(), r.centerY(), base::Color::BLACK, base::Color::WHITE));
						b.fill();
						b.popTransform();
					}

					// middle gray
					{
						auto r = grid.cell();

						b.pushTransform();
						b.translatei(r.left(), r.top());

						b.beginPath();
						b.recti(0, 0, r.width(), r.height());
						b.fillPaint(base::canvas::SolidColor(base::Color(middleGray, middleGray, middleGray, 1.0f)));
						b.fill();
						b.popTransform();
					}

					// SRGB test
					{
						auto r = grid.cell();

						b.pushTransform();
						b.translatei(r.left(), r.top());

						auto lx = r.width() / 4;
						auto rx = 3 * r.width() / 4;
						for (int x = lx; x < rx; ++x)
						{
							b.beginPath();
							b.recti(x, 0, 1, r.height());
							b.fillPaint(base::canvas::SolidColor((x & 1) ? base::Color::WHITE : base::Color::BLACK));
							b.fill();
						}

						b.popTransform();
					}

					// RED

					// linear gradient
					{
						auto r = grid.cell();

						b.pushTransform();
						b.translatei(r.left(), r.top());

						b.beginPath();
						b.recti(0, 0, r.width(), r.height());
						b.fillPaint(base::canvas::LinearGradienti(0, r.centerY(), r.width(), r.centerY(), base::Color::BLACK, base::Color::RED));
						b.fill();
						b.popTransform();
					}

					// middle gray
					{
						auto r = grid.cell();

						b.pushTransform();
						b.translatei(r.left(), r.top());

						b.beginPath();
						b.recti(0, 0, r.width(), r.height());
						b.fillPaint(base::canvas::SolidColor(base::Color(middleGray, 0, 0)));
						b.fill();
						b.popTransform();
					}

					// SRGB test
					{
						auto r = grid.cell();

						b.pushTransform();
						b.translatei(r.left(), r.top());

						auto lx = r.width() / 4;
						auto rx = 3 * r.width() / 4;
						for (int x = lx; x < rx; ++x)
						{
							b.beginPath();
							b.recti(x, 0, 1, r.height());
							b.fillPaint(base::canvas::SolidColor((x & 1) ? base::Color(255, 0, 0) : base::Color::BLACK));
							b.fill();
						}

						b.popTransform();
					}

					// GREEN

					// linear gradient
					{
						auto r = grid.cell();

						b.pushTransform();
						b.translatei(r.left(), r.top());

						b.beginPath();
						b.recti(0, 0, r.width(), r.height());
						b.fillPaint(base::canvas::LinearGradienti(0, r.centerY(), r.width(), r.centerY(), base::Color::BLACK, base::Color::GREEN));
						b.fill();
						b.popTransform();
					}

					// middle gray
					{
						auto r = grid.cell();

						b.pushTransform();
						b.translatei(r.left(), r.top());

						b.beginPath();
						b.recti(0, 0, r.width(), r.height());
						b.fillPaint(base::canvas::SolidColor(base::Color(0, middleGray, 0)));
						b.fill();
						b.popTransform();
					}

					// SRGB test
					{
						auto r = grid.cell();

						b.pushTransform();
						b.translatei(r.left(), r.top());

						auto lx = r.width() / 4;
						auto rx = 3 * r.width() / 4;
						for (int x = lx; x < rx; ++x)
						{
							b.beginPath();
							b.recti(x, 0, 1, r.height());
							b.fillPaint(base::canvas::SolidColor((x & 1) ? base::Color(0, 255, 0) : base::Color::BLACK));
							b.fill();
						}

						b.popTransform();
					}

					// BLUE

					// linear gradient
					{
						auto r = grid.cell();

						b.pushTransform();
						b.translatei(r.left(), r.top());

						b.beginPath();
						b.recti(0, 0, r.width(), r.height());
						b.fillPaint(base::canvas::LinearGradienti(0, r.centerY(), r.width(), r.centerY(), base::Color::BLACK, base::Color::BLUE));
						b.fill();
						b.popTransform();
					}

					// middle gray
					{
						auto r = grid.cell();

						b.pushTransform();
						b.translatei(r.left(), r.top());

						b.beginPath();
						b.recti(0, 0, r.width(), r.height());
						b.fillPaint(base::canvas::SolidColor(base::Color(0, 0, middleGray)));
						b.fill();
						b.popTransform();
					}

					// SRGB test
					{
						auto r = grid.cell();

						b.pushTransform();
						b.translatei(r.left(), r.top());

						auto lx = r.width() / 4;
						auto rx = 3 * r.width() / 4;
						for (int x = lx; x < rx; ++x)
						{
							b.beginPath();
							b.recti(x, 0, 1, r.height());
							b.fillPaint(base::canvas::SolidColor((x & 1) ? base::Color(0, 0, 255) : base::Color::BLACK));
							b.fill();
						}

						b.popTransform();
					}
				}

                c.place(base::Vector2(0,0), g);
            }
        };

        RTTI_BEGIN_TYPE_CLASS(SceneTest_CanvasColorSpace);
        RTTI_METADATA(CanvasTestOrderMetadata).order(50);
        RTTI_END_TYPE();

        //---

    } // test
} // rendering