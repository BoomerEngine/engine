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

/// test of basic gradients
class SceneTest_CanvasGradients : public ICanvasTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_CanvasGradients, ICanvasTest);

public:
    virtual void initialize() override
    {}

    virtual void render(canvas::Canvas& c) override
    {
		canvas::Geometry g;

		{
			canvas::GeometryBuilder b(g);

			CanvasGridBuilder grid(4, 4, 20, c.width(), c.height());

			// linear gradient
			{
				auto r = grid.cell();

				b.pushTransform();
				b.translatei(r.left(), r.top());

				b.beginPath();
				b.roundedRecti(0, 0, r.width(), r.height(), 20);
				b.fillPaint(canvas::LinearGradienti(0, 0, r.width(), r.height(), Color::RED, Color::WHITE));
				b.fill();
				b.popTransform();
			}

			// linear gradient
			{
				auto r = grid.cell();

				b.pushTransform();
				b.translatei(r.left(), r.top());

				b.beginPath();
				b.roundedRecti(0, 0, r.width(), r.height(), 20);
				b.fillPaint(canvas::LinearGradienti(r.width(), 0, 0, r.height(), Color::RED, Color::WHITE));
				b.fill();
				b.popTransform();
			}

			// linear gradient
			{
				auto r = grid.cell();

				b.pushTransform();
				b.translatei(r.left(), r.top());

				b.beginPath();
				b.roundedRecti(0, 0, r.width(), r.height(), 20);
				b.fillPaint(canvas::LinearGradienti(r.width(), r.height() / 2, 0, r.height() / 2, Color::RED, Color::WHITE));
				b.fill();
				b.popTransform();
			}

			// linear gradient
			{
				auto r = grid.cell();

				b.pushTransform();
				b.translatei(r.left(), r.top());

				b.beginPath();
				b.roundedRecti(0, 0, r.width(), r.height(), 20);
				b.fillPaint(canvas::LinearGradienti(r.width() / 2, 0, r.width() / 2, r.height(), Color::RED, Color::WHITE));
				b.fill();
				b.popTransform();
			}

			// box gradient
			{
				auto r = grid.cell();

				b.pushTransform();
				b.translatei(r.left(), r.top());

				b.beginPath();
				b.roundedRecti(0, 0, r.width(), r.height(), 20);
				b.fillPaint(canvas::BoxGradienti(0, 0, r.width(), r.height(), 80, 50, Color::GREEN, Color::WHITE));
				b.fill();
				b.popTransform();
			}

			// box gradient
			{
				auto r = grid.cell();

				b.pushTransform();
				b.translatei(r.left(), r.top());

				b.beginPath();
				b.roundedRecti(0, 0, r.width(), r.height(), 20);
				b.fillPaint(canvas::BoxGradienti(0, 0, r.width(), r.height(), 80, 0, Color::GREEN, Color::WHITE));
				b.fill();
				b.popTransform();
			}

			// box gradient
			{
				auto r = grid.cell();

				b.pushTransform();
				b.translatei(r.left(), r.top());

				b.beginPath();
				b.roundedRecti(0, 0, r.width(), r.height(), 20);
				b.fillPaint(canvas::BoxGradienti(0, 0, r.width(), r.height(), 120, 50, Color::GREEN, Color::WHITE));
				b.fill();
				b.popTransform();
			}

			// box gradient
			{
				auto r = grid.cell();

				b.pushTransform();
				b.translatei(r.left(), r.top());

				b.beginPath();
				b.roundedRecti(0, 0, r.width(), r.height(), 20);
				b.fillPaint(canvas::BoxGradienti(0, 0, r.width(), r.height(), 120, 0, Color::GREEN, Color::WHITE));
				b.fill();
				b.popTransform();
			}

			// circle gradient
			{
				auto r = grid.cell();

				b.pushTransform();
				b.translatei(r.left(), r.top());

				b.beginPath();
				b.roundedRecti(0, 0, r.width(), r.height(), 20);
				b.fillPaint(canvas::RadialGradienti(r.width() / 2, r.height() / 2, 20, 80, Color::BLUE, Color::WHITE));
				b.fill();
				b.popTransform();
			}

			// circle gradient
			{
				auto r = grid.cell();

				b.pushTransform();
				b.translatei(r.left(), r.top());

				b.beginPath();
				b.roundedRecti(0, 0, r.width(), r.height(), 20);
				b.fillPaint(canvas::RadialGradienti(r.width() / 2, r.height() / 2, 0, 80, Color::BLUE, Color::WHITE));
				b.fill();
				b.popTransform();
			}

			// circle gradient
			{
				auto r = grid.cell();

				b.pushTransform();
				b.translatei(r.left(), r.top());

				b.beginPath();
				b.roundedRecti(0, 0, r.width(), r.height(), 20);
				b.fillPaint(canvas::RadialGradienti(r.width() / 2, r.height() / 2, 80, 20, Color::BLUE, Color::WHITE));
				b.fill();
				b.popTransform();
			}

			// circle gradient
			{
				auto r = grid.cell();

				b.pushTransform();
				b.translatei(r.left(), r.top());

				static float rot = 0.0f;
				rot += 0.001f;
				b.translate(r.width() / 2, r.height() / 2);
				b.rotate(rot);
				b.translate(-r.width() / 2, -r.height() / 2);

				b.beginPath();
				b.roundedRecti(0, 0, r.width(), r.height(), 20);
				b.fillPaint(canvas::RadialGradienti(r.width() / 2, r.height() / 2, 80, 0, Color::BLUE, Color::WHITE));
				b.fill();
				b.popTransform();
			}
		}

        c.place(Vector2(0,0), g);
    }
};

RTTI_BEGIN_TYPE_CLASS(SceneTest_CanvasGradients);
RTTI_METADATA(CanvasTestOrderMetadata).order(30);
RTTI_END_TYPE();

//---

END_BOOMER_NAMESPACE_EX(test)
