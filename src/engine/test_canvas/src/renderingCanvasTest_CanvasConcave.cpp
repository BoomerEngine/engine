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

/// test of concave fill
class SceneTest_CanvasConcave : public ICanvasTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_CanvasConcave, ICanvasTest);

public:
    virtual void initialize() override
    {
    }

    virtual void render(canvas::Canvas& c) override
    {
		canvas::Geometry g;
		{
			CanvasGridBuilder grid(2, 2, 20, 1024, 1024);
			canvas::GeometryBuilder builder(g);

			{
				auto r = grid.cell();

				auto cx = r.centerX();
				auto cy = r.centerY();
				auto rad = r.width() / 2;

				builder.beginPath();

				for (uint32_t i = 0; i <= 36; ++i)
				{
					auto f = TWOPI * (i / 36.0f);
					auto pr = (i & 1) ? rad : rad / 4;
					auto px = cx + std::cos(f) * pr;
					auto py = cy + std::sin(f) * pr;

					if (i == 0)
						builder.moveTo(px, py);
					else
						builder.lineTo(px, py);
				}

				builder.fillColor(Color::WHITE);
				builder.fill();
			}
		}

		c.place(Vector2(0, 0), g);
    }
};

RTTI_BEGIN_TYPE_CLASS(SceneTest_CanvasConcave);
	RTTI_METADATA(CanvasTestOrderMetadata).order(60);
RTTI_END_TYPE();

//---

END_BOOMER_NAMESPACE_EX(test)
