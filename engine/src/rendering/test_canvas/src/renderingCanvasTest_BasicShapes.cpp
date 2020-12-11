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
        /// test of basic canvas shapes
        class SceneTest_CanvasBasicShapes : public ICanvasTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_CanvasBasicShapes, ICanvasTest);

        public:
            virtual void initialize() override
            {}

            virtual void render(base::canvas::Canvas& c) override
            {
                CanvasGridBuilder grid(4, 4, 20, 1024, 1024);

				base::canvas::Geometry g;
				{
					base::canvas::GeometryBuilder b(m_storage, g);

					// triangle
					{
						auto r = grid.cell();

						b.beginPath();
						b.moveToi(r.centerX(), r.top());
						b.lineToi(r.right(), r.bottom());
						b.lineToi(r.left(), r.bottom());
						b.fillColor(base::Color::RED);
						b.fill();
					}

					// square
					{
						auto r = grid.cell();

						b.beginPath();
						b.rect(r);
						b.fillColor(base::Color::GREEN);
						b.fill();
					}

					// rounded square
					{
						auto r = grid.cell();

						b.beginPath();
						b.roundedRect(r, 20);
						b.fillColor(base::Color::BLUE);
						b.fill();
					}

					// circle
					{
						auto r = grid.cell();

						b.beginPath();
						b.circlei(r.centerX(), r.centerY(), r.width() / 2);
						b.fillColor(base::Color::CYAN);
						b.fill();
					}

					// horizontal ellipse
					{
						auto r = grid.cell();

						b.beginPath();
						b.ellipsei(r.centerX(), r.centerY(), r.width() / 2, 10);
						b.fillColor(base::Color::YELLOW);
						b.fill();
					}

					// vertical ellipse
					{
						auto r = grid.cell();

						b.beginPath();
						b.ellipsei(r.centerX(), r.centerY(), 10, r.height() / 2);
						b.fillColor(base::Color(255, 127, 0));
						b.fill();
					}

					// arc
					{
						auto r = grid.cell();

						b.beginPath();
						b.arci(r.centerX(), r.centerY(), r.width() / 2, 0.0f, PI, base::canvas::Winding::CW);
						b.fillColor(base::Color::GRAY);
						b.fill();
					}
				}

                c.place(base::canvas::Placement(0,0), g);
            }
        };

        RTTI_BEGIN_TYPE_CLASS(SceneTest_CanvasBasicShapes);
            RTTI_METADATA(CanvasTestOrderMetadata).order(10);
        RTTI_END_TYPE();

        //---

    } // test
} // rendering