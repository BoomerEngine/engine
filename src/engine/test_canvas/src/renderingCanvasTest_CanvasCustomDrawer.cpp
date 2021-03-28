/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/

#include "build.h"
#include "renderingCanvasTest.h"

#include "gpu/device/include/commandWriter.h"
#include "gpu/device/include/shader.h"
#include "gpu/device/include/descriptor.h"

#include "engine/canvas/include/batchRenderer.h"
#include "engine/canvas/include/geometryBuilder.h"
#include "engine/canvas/include/geometry.h"
#include "engine/canvas/include/canvas.h"
#include "engine/canvas/include/style.h"
#include "engine/font/include/font.h"

BEGIN_BOOMER_NAMESPACE_EX(test)

//--

/// custom rendering handler
class SceneTest_SimpleSpriteOutline : public ICanvasSimpleBatchRenderer
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_SimpleSpriteOutline, ICanvasSimpleBatchRenderer);

public:
	struct PrivateData
	{
		Color color = Color::WHITE;
		float radius = 1.0f;

		INLINE PrivateData() {};
		INLINE PrivateData(Color _color, float _radius)
			: color(_color)
			, radius(_radius)
		{}
	};

	virtual gpu::ShaderObjectPtr loadMainShaderFile() override final
	{
		return gpu::LoadStaticShaderDeviceObject("canvas/canvas_sprite_outline.fx");
	}

	virtual void render(gpu::CommandWriter& cmd, const RenderData& data, uint32_t firstVertex, uint32_t numVertices) const override
	{
		const auto* params = (PrivateData*)data.customData;

		struct
		{
			Color color;
			float radius;
			float invRadiusMinOne;
		} consts;

		consts.color = params->color;
		consts.radius = params->radius;
		consts.invRadiusMinOne = (params->radius > 1.0f) ? 1.0f / (params->radius - 1.0f) : 0.0f;

		gpu::DescriptorEntry desc[1];
		desc[0].constants(consts);
		cmd.opBindDescriptor("OutlineParameters"_id, desc);

		TBaseClass::render(cmd, data, firstVertex, numVertices);
	}
};

RTTI_BEGIN_TYPE_CLASS(SceneTest_SimpleSpriteOutline);
RTTI_END_TYPE();

//--

/// test of custom drawer functionality
class SceneTest_CanvasCustomDrawer : public ICanvasTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_CanvasCustomDrawer, ICanvasTest);

public:
	CanvasImagePtr m_testImages[3];
	FontPtr m_font;

	virtual void initialize() override
	{
		m_font = loadFont("aileron_regular.otf");

		m_testImages[0] = loadCanvasImage("tree.png");
		m_testImages[1] = loadCanvasImage("sign.png");
		m_testImages[2] = loadCanvasImage("crate.png");
	}

	virtual void shutdown() override
	{
	}

    virtual void render(Canvas& c) override
    {
		const auto margin = 7.0f;

		CanvasGeometry g;

		{
			CanvasGeometryBuilder b(g);

			static float t = 0.0f;
			t += 0.05f;
			float width = 5.0f + 4.0 * cos(t);

			b.selectRenderer<SceneTest_SimpleSpriteOutline>(Color::MAGENTA, width);

			if (m_testImages[0])
			{
				auto pattern = CanvasStyle_ImagePattern(m_testImages[0]);
				b.fillPaint(pattern);
				b.beginPath();
				b.stylePivot(100, 100);
				b.rect(100-margin, 100 -margin, m_testImages[0]->width() + 2.0f* margin, m_testImages[0]->height() + 2.0f * margin);
				b.fill();
			}

			b.selectRenderer<SceneTest_SimpleSpriteOutline>(Color::RED, 1.0f);

			if (m_testImages[2])
			{
				auto pattern = CanvasStyle_ImagePattern(m_testImages[2]);
				b.fillPaint(pattern);
				b.beginPath();
				b.stylePivot(500, 300);
				b.rect(500 - margin, 300 - margin, m_testImages[2]->width() + 2.0f * margin, m_testImages[2]->height() + 2.0f * margin);
				b.fill();
			}
		}

		c.place(Vector2(0, 0), g);

		{
			CanvasGeometryBuilder b(g);

			b.selectRenderer<SceneTest_SimpleSpriteOutline>(Color::YELLOW, 5.0f);

			if (m_testImages[1])
			{
				auto pattern = CanvasStyle_ImagePattern(m_testImages[1]);
				b.fillPaint(pattern);
				b.beginPath();
				b.rect(-margin, -margin, m_testImages[1]->width() + 2.0f * margin, m_testImages[1]->height() + 2.0f * margin);
				b.fill();
			}
		}

		float w = m_testImages[1] ? m_testImages[1]->width() : 0.0f;
		float h = m_testImages[1] ? m_testImages[1]->height() : 0.0f;
		static float a = 0.0;
		a += 0.01f;

		auto pos = XForm2D::BuildRotationAround(a, w * 0.5f, h * 0.5f);
		c.place(pos + Vector2(500, 100), g);

		{
			CanvasGeometryBuilder b(g);

			b.selectRenderer<SceneTest_SimpleSpriteOutline>(Color::CYAN, 2.0f);
			b.print(m_font, 70.0f, "Outline #%$&");
		}

		c.place(Vector2(100, 400), g);

		{
			CanvasGeometryBuilder b(g);

			b.selectRenderer<SceneTest_SimpleSpriteOutline>(Color::GREEN, 4.0f);
			b.print(m_font, 230.0f, "Outline #%$&");
		}

		c.place(Vector2(100, 600), g);
    }
};

RTTI_BEGIN_TYPE_CLASS(SceneTest_CanvasCustomDrawer);
    RTTI_METADATA(CanvasTestOrderMetadata).order(107);
RTTI_END_TYPE();

//--

END_BOOMER_NAMESPACE_EX(test)
