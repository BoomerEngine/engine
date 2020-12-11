/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/

#include "build.h"
#include "renderingCanvasTest.h"

#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/canvas/include/renderingCanvasBatchRenderer.h"
#include "rendering/device/include/renderingShaderFile.h"
#include "rendering/device/include/renderingDescriptor.h"

#include "base/canvas/include/canvasGeometryBuilder.h"
#include "base/canvas/include/canvasGeometry.h"
#include "base/canvas/include/canvas.h"
#include "base/canvas/include/canvasStyle.h"
#include "base/canvas/include/canvasStorage.h"
#include "base/font/include/font.h"

namespace rendering
{
    namespace test
    {

        //--

        base::res::StaticResource<ShaderFile> resCanvasSimpleOutlineShader("/engine/shaders/canvas/canvas_sprite_outline.fx");

        /// custom rendering handler
        class SceneTest_SimpleSpriteOutline : public rendering::canvas::ICanvasSimpleBatchRenderer
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_SimpleSpriteOutline, rendering::canvas::ICanvasSimpleBatchRenderer);

        public:
			struct PrivateData
			{
				base::Color color = base::Color::WHITE;
				float radius = 1.0f;

				INLINE PrivateData() {};
				INLINE PrivateData(base::Color _color, float _radius)
					: color(_color)
					, radius(_radius)
				{}
			};

			virtual ShaderFilePtr loadMainShaderFile() override final
			{
				return resCanvasSimpleOutlineShader.loadAndGet();
			}

			virtual void render(command::CommandWriter& cmd, const RenderData& data, uint32_t firstVertex, uint32_t numVertices) const override
			{
				const auto* params = (PrivateData*)data.customData;

				struct
				{
					base::Color color;
					float radius;
					float invRadiusMinOne;
				} consts;

				consts.color = params->color;
				consts.radius = params->radius;
				consts.invRadiusMinOne = (params->radius > 1.0f) ? 1.0f / (params->radius - 1.0f) : 0.0f;

				DescriptorEntry desc[1];
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
			base::canvas::ImageAtlasIndex m_atlas;
			base::canvas::ImageEntry m_testImages[3];
			base::FontPtr m_font;

			virtual void initialize() override
			{
				m_font = loadFont("aileron_regular.otf");
				m_atlas = m_storage->createAtlas(1024, 1, "TestAtlas");

				m_testImages[0] = m_storage->registerImage(m_atlas, loadImage("tree.png"));
				m_testImages[1] = m_storage->registerImage(m_atlas, loadImage("sign.png"));
				m_testImages[2] = m_storage->registerImage(m_atlas, loadImage("crate.png"));
			}

			virtual void shutdown() override
			{
				m_storage->destroyAtlas(m_atlas);
			}

            virtual void render(base::canvas::Canvas& c) override
            {
				const auto margin = 7.0f;

				base::canvas::Geometry g;

				{
					base::canvas::GeometryBuilder b(m_storage, g);

					static float t = 0.0f;
					t += 0.05f;
					float width = 5.0f + 4.0 * cos(t);

					b.selectRenderer<SceneTest_SimpleSpriteOutline>(base::Color::MAGENTA, width);

					{
						auto pattern = base::canvas::ImagePattern(m_testImages[0]);
						b.fillPaint(pattern);
						b.beginPath();
						b.stylePivot(100, 100);
						b.rect(100-margin, 100 -margin, m_testImages[0].width + 2.0f* margin, m_testImages[0].height + 2.0f * margin);
						b.fill();
					}

					b.selectRenderer<SceneTest_SimpleSpriteOutline>(base::Color::RED, 1.0f);

					{
						auto pattern = base::canvas::ImagePattern(m_testImages[2]);
						b.fillPaint(pattern);
						b.beginPath();
						b.stylePivot(500, 300);
						b.rect(500 - margin, 300 - margin, m_testImages[2].width + 2.0f * margin, m_testImages[2].height + 2.0f * margin);
						b.fill();
					}
				}

				c.place(base::Vector2(0, 0), g);

				{
					base::canvas::GeometryBuilder b(m_storage, g);

					b.selectRenderer<SceneTest_SimpleSpriteOutline>(base::Color::YELLOW, 5.0f);

					{
						auto pattern = base::canvas::ImagePattern(m_testImages[1]);
						b.fillPaint(pattern);
						b.beginPath();
						b.rect(-margin, -margin, m_testImages[1].width + 2.0f * margin, m_testImages[1].height + 2.0f * margin);
						b.fill();
					}
				}

				float w = m_testImages[1].width;
				float h = m_testImages[1].height;
				static float a = 0.0;
				a += 0.01f;

				XForm2D pos = XForm2D::BuildRotationAround(a, w * 0.5f, h * 0.5f);
				c.place(pos + Vector2(500, 100), g);

				{
					base::canvas::GeometryBuilder b(m_storage, g);

					b.selectRenderer<SceneTest_SimpleSpriteOutline>(base::Color::CYAN, 2.0f);
					b.print(m_font, 70.0f, "Outline #%$&");
				}

				c.place(Vector2(100, 400), g);

				{
					base::canvas::GeometryBuilder b(m_storage, g);

					b.selectRenderer<SceneTest_SimpleSpriteOutline>(base::Color::GREEN, 4.0f);
					b.print(m_font, 230.0f, "Outline #%$&");
				}

				c.place(Vector2(100, 600), g);
            }
        };

        RTTI_BEGIN_TYPE_CLASS(SceneTest_CanvasCustomDrawer);
            RTTI_METADATA(CanvasTestOrderMetadata).order(107);
        RTTI_END_TYPE();

        //--

    } // test
} // rendering