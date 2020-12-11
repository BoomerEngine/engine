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

		//--

        /// test of lines of different width
        class SceneTest_CanvasLines: public ICanvasTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_CanvasLines, ICanvasTest);

        public:
            virtual void initialize() override
            {}

            virtual void render(base::canvas::Canvas& c) override
            {
				base::canvas::Geometry g;
				{
					base::canvas::GeometryBuilder builder(m_storage, g);
					builder.antialiasing(true);

					const float strokeSizes[] = { 1.0f, 0.1f, 0.25f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 15.0f, 20.0f };

					CanvasGridBuilder grid(1, ARRAY_COUNT(strokeSizes) + 1, 20, 1024, 1024);
					for (uint32_t i = 0; i < ARRAY_COUNT(strokeSizes); ++i)
					//for (uint32_t i = 0; i < 1; ++i)
					{
						auto r = grid.cell();

						auto b = r.centerX();
						auto a = (b + r.left()) / 2;
						auto c = (b + r.right()) / 2;

						if (i == 0)
						{
							builder.beginPath();
							//builder.moveToi(r.left(), r.bottom());
							//builder.lineToi(r.right(), r.bottom());
							builder.moveToi(5, 5);
							builder.lineToi(20, 5);
							builder.strokeColor(base::Color::WHITE, strokeSizes[i]);
							builder.stroke();

							builder.beginPath();
							//builder.moveToi(r.left(), r.bottom());
							//builder.lineToi(r.right(), r.bottom());
							builder.moveToi(20, 5);
							builder.lineToi(25, 5);
							builder.strokeColor(base::Color::YELLOW, strokeSizes[i]);
							builder.stroke();

							builder.beginPath();
							//builder.moveToi(r.left(), r.bottom());
							//builder.lineToi(r.right(), r.bottom());
							builder.moveToi(20, 7);
							builder.lineToi(5, 7);
							builder.strokeColor(base::Color::CYAN, strokeSizes[i]);
							builder.stroke();



							builder.beginPath();
							//builder.moveToi(r.left(), r.bottom());
							//builder.lineToi(r.right(), r.bottom());
							builder.lineJoin(base::canvas::LineJoin::Bevel);
							builder.recti(5, 10, 20, 20);
							builder.strokeColor(base::Color::WHITE, strokeSizes[i]);
							builder.stroke();

							builder.lineJoin(base::canvas::LineJoin::Miter);
						}
						else
						{
							builder.beginPath();
							builder.moveToi(r.left(), r.bottom());

							// triangle
							builder.lineToi(a - 40, r.bottom());
							builder.lineToi(a, r.top());
							builder.lineToi(a + 40, r.bottom());

							// curve
							builder.lineToi(b - 40, r.bottom());
							builder.bezierToi(b - 40, r.top(), b + 40, r.top(), b + 40, r.bottom());

							// square
							builder.lineToi(c - 40, r.bottom());
							builder.lineToi(c - 40, r.top());
							builder.lineToi(c + 40, r.top());
							builder.lineToi(c + 40, r.bottom());

							builder.lineToi(r.right(), r.bottom());
							builder.strokeColor(base::Color::WHITE, strokeSizes[i]);
							builder.stroke();
						}
					}
				}

                c.place(base::Vector2(0,0), g);
            }
        };

        RTTI_BEGIN_TYPE_CLASS(SceneTest_CanvasLines);
            RTTI_METADATA(CanvasTestOrderMetadata).order(20);
        RTTI_END_TYPE();

        //---      

    } // test
} // rendering