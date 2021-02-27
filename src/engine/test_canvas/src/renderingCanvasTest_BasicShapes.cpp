/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/


#include "build.h"
#include "renderingCanvasTest.h"

#include "engine/canvas/include/geometryBuilder.h"
#include "engine/canvas/include/geometry.h"
#include "engine/canvas/include/canvas.h"
#include "engine/canvas/include/style.h"

BEGIN_BOOMER_NAMESPACE_EX(test)

/// test of basic canvas shapes
class SceneTest_CanvasBasicShapes : public ICanvasTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_CanvasBasicShapes, ICanvasTest);

public:
    virtual void initialize() override
    {}

    virtual void render(canvas::Canvas& c) override
    {
        CanvasGridBuilder grid(4, 4, 20, 1024, 1024);

		canvas::Geometry g;
		{
			canvas::GeometryBuilder b(g);

			// triangle
			{
				auto r = grid.cell();

				b.beginPath();
				b.moveToi(r.centerX(), r.top());
				b.lineToi(r.right(), r.bottom());
				b.lineToi(r.left(), r.bottom());
				b.fillColor(Color::RED);
				b.fill();
			}

			// square
			{
				auto r = grid.cell();

				b.beginPath();
				b.rect(r);
				b.fillColor(Color::GREEN);
				b.fill();
			}

			// rounded square
			{
				auto r = grid.cell();

				b.beginPath();
				b.roundedRect(r, 20);
				b.fillColor(Color::BLUE);
				b.fill();
			}

			// circle
			{
				auto r = grid.cell();

				b.beginPath();
				b.circlei(r.centerX(), r.centerY(), r.width() / 2);
				b.fillColor(Color::CYAN);
				b.fill();
			}

			// horizontal ellipse
			{
				auto r = grid.cell();

				b.beginPath();
				b.ellipsei(r.centerX(), r.centerY(), r.width() / 2, 10);
				b.fillColor(Color::YELLOW);
				b.fill();
			}

			// vertical ellipse
			{
				auto r = grid.cell();

				b.beginPath();
				b.ellipsei(r.centerX(), r.centerY(), 10, r.height() / 2);
				b.fillColor(Color(255, 127, 0));
				b.fill();
			}

			// arc
			{
				auto r = grid.cell();

				b.beginPath();
				b.arci(r.centerX(), r.centerY(), r.width() / 2, 0.0f, PI, canvas::Winding::CW);
				b.fillColor(Color::GRAY);
				b.fill();
			}
		}

        c.place(canvas::Placement(0,0), g);
    }
};

RTTI_BEGIN_TYPE_CLASS(SceneTest_CanvasBasicShapes);
    RTTI_METADATA(CanvasTestOrderMetadata).order(10);
RTTI_END_TYPE();

//---

END_BOOMER_NAMESPACE_EX(test)
