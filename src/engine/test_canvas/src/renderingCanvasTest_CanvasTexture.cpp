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
#include "engine/canvas/include/atlas.h"

BEGIN_BOOMER_NAMESPACE_EX(test)

/// test of basic texture rendering
class SceneTest_CanvasTexture : public ICanvasTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_CanvasTexture, ICanvasTest);

public:
	RefPtr<canvas::DynamicAtlas> m_atlas;

	canvas::ImageEntry m_atlasEntry;

	NativeTimePoint m_time;

    virtual void initialize() override
    {
		m_time.resetToNow();

		auto lena = loadImage("lena.png");

		m_atlas = RefNew<canvas::DynamicAtlas>(1024, 1);
		m_atlasEntry = m_atlas->registerImage(lena, true);
    }

	virtual void shutdown() override
	{
		m_atlas.reset();
	}

    virtual void render(canvas::Canvas& c) override
    {
		canvas::Geometry g;
		{
			canvas::GeometryBuilder b(g);

			CanvasGridBuilder grid(3, 3, 30, 1024, 1024);

			float time = m_time.timeTillNow().toSeconds();

			// simple image
			{
				auto r = grid.cell();

				b.resetTransform();
				b.translatei(r.left(), r.top());

				b.blending(canvas::BlendOp::Copy);
				b.beginPath();
				b.roundedRecti(0, 0, r.width(), r.height(), 20);
				b.fillPaint(canvas::ImagePattern(m_atlasEntry));
				b.fill();
			}

			// scaled image pattern
			{
				auto r = grid.cell();

				b.resetTransform();
				b.translatei(r.left(), r.top());

				b.blending(canvas::BlendOp::Copy);
				b.beginPath();
				b.roundedRecti(0, 0, r.width(), r.height(), 20);
				b.fillPaint(canvas::ImagePattern(canvas::ImagePatternSettings(m_atlasEntry).scale(2.0f)));
				b.fill();
			}

			// scaled image pattern with offset
			{
				auto r = grid.cell();

				b.resetTransform();
				b.translatei(r.left(), r.top());

				b.blending(canvas::BlendOp::Copy);
				b.beginPath();
				b.roundedRecti(0, 0, r.width(), r.height(), 20);
				b.fillPaint(canvas::ImagePattern(canvas::ImagePatternSettings(m_atlasEntry).scale(2.0f).offset(20.f, 20.0f)));
				b.fill();
			}

			// scaled image rotated with no pivot
			{
				auto r = grid.cell();

				b.resetTransform();
				b.translatei(r.left(), r.top());

				b.blending(canvas::BlendOp::Copy);
				b.beginPath();
				b.roundedRecti(0, 0, r.width(), r.height(), 20);
				b.fillPaint(canvas::ImagePattern(canvas::ImagePatternSettings(m_atlasEntry).scale(2.0f).angle(1.0f * time)));
				b.fill();
			}

			// scaled image rotated around center
			{
				auto r = grid.cell();

				b.resetTransform();
				b.translatei(r.left(), r.top());

				float cx = m_atlasEntry.width / 2;
				float cy = m_atlasEntry.height / 2;

				b.blending(canvas::BlendOp::Copy);
				b.beginPath();
				b.roundedRecti(0, 0, r.width(), r.height(), 20);
				b.fillPaint(canvas::ImagePattern(canvas::ImagePatternSettings(m_atlasEntry).scale(2.0f).pivot(cx, cy).angle(1.0f * time)));
				b.fill();
			}

			// scaled image rotated around center
			{
				auto r = grid.cell();

				b.resetTransform();
				b.translatei(r.left(), r.top());

				float cx = m_atlasEntry.width;
				float cy = m_atlasEntry.height;

				float ox = (m_atlasEntry.width / 1.5f) - r.width();
				float oy = (m_atlasEntry.height / 1.5f) - r.height();

				b.blending(canvas::BlendOp::Copy);
				b.beginPath();
				b.roundedRecti(0, 0, r.width(), r.height(), 20);
				b.fillPaint(canvas::ImagePattern(canvas::ImagePatternSettings(m_atlasEntry).scale(1.5f).pivot(cx, cy).offset(-ox, -oy).angle(1.0f * time)));
				b.fill();
			}

			// scaled image rotated with no pivot
			{
				auto r = grid.cell();

				b.resetTransform();
				b.translatei(r.left(), r.top());

				b.blending(canvas::BlendOp::Copy);
				b.beginPath();
				b.roundedRecti(0, 0, r.width(), r.height(), 20);
				b.fillPaint(canvas::ImagePattern(canvas::ImagePatternSettings(m_atlasEntry).scale(3.0f).angle(-1.0f * time).wrap()));
				b.fill();
			}

			// scaled image rotated around center
			{
				auto r = grid.cell();

				b.resetTransform();
				b.translatei(r.left(), r.top());

				float cx = m_atlasEntry.width / 2;
				float cy = m_atlasEntry.height / 2;

				b.blending(canvas::BlendOp::Copy);
				b.beginPath();
				b.roundedRecti(0, 0, r.width(), r.height(), 20);
				b.fillPaint(canvas::ImagePattern(canvas::ImagePatternSettings(m_atlasEntry).scale(2.0f).pivot(cx, cy).angle(1.0f * time).wrap()));
				b.fill();
			}

			// scaled image rotated around center
			{
				auto r = grid.cell();

				b.resetTransform();
				b.translatei(r.left(), r.top());

				float cx = m_atlasEntry.width;
				float cy = m_atlasEntry.height;

				float ox = (m_atlasEntry.width / 1.5f) - r.width();
				float oy = (m_atlasEntry.height / 1.5f) - r.height();

				b.blending(canvas::BlendOp::Copy);
				b.beginPath();
				b.roundedRecti(0, 0, r.width(), r.height(), 20);
				b.fillPaint(canvas::ImagePattern(canvas::ImagePatternSettings(m_atlasEntry).scale(1.5f).pivot(cx, cy).offset(-ox, -oy).angle(1.0f * time).wrap()));
				b.fill();
			}
		}

		//c.placement(0.0f, 0.0f, c.width() / 1024.0f, c.height() / 1024.0f);
		c.place(Vector2(0,0), g);
    }
};

RTTI_BEGIN_TYPE_CLASS(SceneTest_CanvasTexture);
RTTI_METADATA(CanvasTestOrderMetadata).order(100);
RTTI_END_TYPE();

//--

END_BOOMER_NAMESPACE_EX(test)
