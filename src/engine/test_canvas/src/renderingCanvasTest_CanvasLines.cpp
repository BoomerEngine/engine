/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/

#include "build.h"
#include "renderingCanvasTest.h"

#include "engine/canvas/include/canvasGeometryBuilder.h"
#include "engine/canvas/include/canvasGeometry.h"
#include "engine/canvas/include/canvas.h"
#include "engine/canvas/include/canvasStyle.h"

BEGIN_BOOMER_NAMESPACE_EX(test)

//--

/// test of lines of different width
class SceneTest_CanvasLines: public ICanvasTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_CanvasLines, ICanvasTest);

public:
    virtual void initialize() override
    {}

    virtual void render(canvas::Canvas& c) override
    {
		canvas::Geometry g;
		{
			canvas::GeometryBuilder builder(g);
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
					builder.strokeColor(Color::WHITE, strokeSizes[i]);
					builder.stroke();

					builder.beginPath();
					//builder.moveToi(r.left(), r.bottom());
					//builder.lineToi(r.right(), r.bottom());
					builder.moveToi(20, 5);
					builder.lineToi(25, 5);
					builder.strokeColor(Color::YELLOW, strokeSizes[i]);
					builder.stroke();

					builder.beginPath();
					//builder.moveToi(r.left(), r.bottom());
					//builder.lineToi(r.right(), r.bottom());
					builder.moveToi(20, 7);
					builder.lineToi(5, 7);
					builder.strokeColor(Color::CYAN, strokeSizes[i]);
					builder.stroke();



					builder.beginPath();
					//builder.moveToi(r.left(), r.bottom());
					//builder.lineToi(r.right(), r.bottom());
					builder.lineJoin(canvas::LineJoin::Bevel);
					builder.recti(5, 10, 20, 20);
					builder.strokeColor(Color::WHITE, strokeSizes[i]);
					builder.stroke();

					builder.lineJoin(canvas::LineJoin::Miter);
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
					builder.strokeColor(Color::WHITE, strokeSizes[i]);
					builder.stroke();
				}
			}
		}

        c.place(Vector2(0,0), g);
    }
};

RTTI_BEGIN_TYPE_CLASS(SceneTest_CanvasLines);
    RTTI_METADATA(CanvasTestOrderMetadata).order(20);
RTTI_END_TYPE();

//---

END_BOOMER_NAMESPACE_EX(test)
