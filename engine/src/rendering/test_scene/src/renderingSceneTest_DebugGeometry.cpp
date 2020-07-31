/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: tests #]
***/

#include "build.h"
#include "renderingSceneTest.h"


#include "base/image/include/image.h"

#include "rendering/scene/include/renderingFrameDebug.h"
#include "rendering/scene/include/renderingFrameDebugGeometry.h"

namespace rendering
{
    namespace test
    {

        //---

        //static base::res::StaticResource<rendering::texture::StaticTexture> resIconPointLight("/engine/icons/point_light.png", true);
        //static base::res::StaticResource<rendering::texture::StaticTexture> resLena("/engine/tests/textures/lena.png");

        //--

        class SceneTest_DebugGeometry : public ISceneTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_DebugGeometry, ISceneTest);

        public:
            SceneTest_DebugGeometry()
            {            
            }

            virtual void setupFrame(scene::FrameParams& frame) override
            {
                base::Vector3 pos(2, 0, 0);
                drawSpites(frame.geometry.solid, pos);
                drawLines(frame.geometry.solid, pos);
                drawWireShapes(frame.geometry.solid, pos);
                drawSolidShapes(frame.geometry.solid, pos);

                base::Vector2 screenPos(10, 10);
                drawSceenShapes(frame.geometry.screen, screenPos);
            }

            const float SEPARATION = 2.5f;

            void drawSpites(scene::DebugGeometry& debugGeometry, base::Vector3& rowPos)
            {
                scene::DebugSpriteDrawer dd(debugGeometry);

                dd.color(base::Color::RED);
                dd.size(16.0f);
                dd.sprite(base::Vector3(0.0f, -1.0f, 0.0f));

                dd.color(base::Color::BLUE);
                dd.size(32.0f);
                dd.sprite(base::Vector3(0.0f, 1.0f, 0.0f));

                dd.color(base::Color::WHITE);
                dd.size(32.0f);
                //dd.texture(resIconPointLight.loadAndGet() ? resIconPointLight.loadAndGet()->getRenderingObject() : nullptr);
                dd.sprite(base::Vector3::ZERO());
            }

            void drawLines(scene::DebugGeometry& debugGeometry, base::Vector3& rowPos)
            {
                scene::DebugLineDrawer dd(debugGeometry);

                // initial shift
                base::Vector3 pos = rowPos - base::Vector3::EY() * SEPARATION * 2.0f;
                rowPos += base::Vector3::EX() * SEPARATION;

                // arrows
                {
                    const auto start = pos + base::Vector3(0, 0, 0.5f);
                    pos += base::Vector3::EY() * SEPARATION;

                    dd.color(base::Color::BLUE);
                    dd.arrow(start, start + base::Vector3(0, 0, 0.5f));
                    dd.arrow(start, start - base::Vector3(0, 0, 0.5f));
                    dd.color(base::Color::RED);
                    dd.arrow(start, start + base::Vector3(0.5f, 0, 0));
                    dd.arrow(start, start - base::Vector3(0.5f, 0, 0));
                    dd.color(base::Color::GREEN);
                    dd.arrow(start, start + base::Vector3(0, 0.5f, 0));
                    dd.arrow(start, start - base::Vector3(0, 0.5f, 0));
                }

                // axes
                {
                    const auto start = pos + base::Vector3(0, 0, 0.5f);
                    pos += base::Vector3::EY() * SEPARATION;

                    auto matrix = base::Angles(45.0f, 45.0f, 45.0f).toMatrix();
                    matrix.translation(start);

                    dd.axes(matrix, 0.5f);
                }

                // fat line
                {
                    const auto start = pos + base::Vector3(0, 0, 0.5f);
                    pos += base::Vector3::EY() * SEPARATION;

                    dd.size(10.0f);
                    dd.color(base::Color::BLUE);
                    dd.line(start - base::Vector3(0, 0, 0.5f), start + base::Vector3(0, 0, 0.5f));
                    dd.color(base::Color::RED);
                    dd.line(start - base::Vector3(0.5f, 0, 0), start + base::Vector3(0.5f, 0, 0));
                    dd.color(base::Color::GREEN);
                    dd.line(start - base::Vector3(0, 0.5f, 0), start + base::Vector3(0, 0.5f, 0));
                    dd.size(1.0f);
                }

                // polygon
                {
                    const auto start = pos + base::Vector3(0, 0, 0.5f);
                    pos += base::Vector3::EY() * SEPARATION;

                    {
                        base::Vector3 v[3];
                        v[0] = start + base::Vector3(0, 0, 0.5f);
                        v[1] = start + base::Vector3(0, -0.5f, -0.5f);
                        v[2] = start + base::Vector3(0, +0.5f, -0.5f);
                        dd.color(base::Color::CYAN);
                        dd.loop(v, 3);
                    }

                    {
                        scene::DebugVertex v[3];
                        v[0].pos(start - base::Vector3(0, 0, 0.5f)).color(255, 0, 0);
                        v[1].pos(start - base::Vector3(0, -0.5f, -0.5f)).color(0, 255, 0);
                        v[2].pos(start - base::Vector3(0, +0.5f, -0.5f)).color(0, 0, 255);
                        dd.loop(v, 3);
                    }
                }
            }

            void drawWireShapes(scene::DebugGeometry& debugGeometry, base::Vector3& rowPos)
            {
                scene::DebugLineDrawer dd(debugGeometry);

                // initial shift
                base::Vector3 pos = rowPos - base::Vector3::EY() * SEPARATION * 3.5f;
                rowPos += base::Vector3::EX() * SEPARATION;

                // box with corners
                {
                    const auto center = pos + base::Vector3(0, 0, 0.5f);
                    const auto extents = base::Vector3(0.4f, 0.4f, 0.4f);
                    dd.color(base::Color::CYAN);
                    dd.box(base::Box(center - extents, center + extents));

                    dd.color(base::Color::RED);
                    dd.brackets(base::Box(center - extents * 1.1f, center + extents * 1.1f));

                    pos += base::Vector3::EY() * SEPARATION;
                }

                // sphere
                {
                    const auto center = pos + base::Vector3(0, 0, 0.5f);
                    dd.color(base::Color::YELLOW);
                    dd.sphere(center, 0.4f);
                    pos += base::Vector3::EY() * SEPARATION;
                }

                // capsule
                {
                    const auto center = pos + base::Vector3(0, 0, 0.5f);
                    dd.color(base::Color::MAGENTA);
                    dd.capsule(center, 0.4f, 0.3f);
                    pos += base::Vector3::EY() * SEPARATION;
                }

                // cylinder
                {
                    const auto top = pos + base::Vector3(0, 0, 0.9f);
                    const auto bottom = pos + base::Vector3(0, 0, 0.1f);
                    dd.color(base::Color(255, 128, 64));
                    dd.cylinder(bottom, top, 0.2f, 0.4f);
                    pos += base::Vector3::EY() * SEPARATION;
                }

                // cone
                {
                    const auto top = pos + base::Vector3(0, 0, 0.9f);
                    dd.color(base::Color::BLUE);
                    dd.cone(top, base::Vector3(0, 0, -1.0f), 0.8f, 10.0f);
                    dd.color(base::Color::GREEN);
                    dd.cone(top, base::Vector3(0, 0, -1.0f), 0.8f, 45.0f);
                    dd.color(base::Color::RED);
                    dd.cone(top, base::Vector3(0, 0, -1.0f), 0.8f, 85.0f);

                    pos += base::Vector3::EY() * SEPARATION;
                }

                // plane
                {
                    const auto center = pos + base::Vector3(0, 0, 0.5f);
                    dd.color(base::Color::BLUE);
                    dd.plane(center, base::Vector3::EZ(), 0.5f);
                    dd.color(base::Color::GREEN);
                    dd.plane(center, base::Vector3::EY(), 0.5f);
                    dd.color(base::Color::RED);
                    dd.plane(center, base::Vector3::EX(), 0.5f);

                    pos += base::Vector3::EY() * SEPARATION;
                }

                // full circles
                {
                    const auto center = pos + base::Vector3(0, 0, 0.5f);
                    dd.color(base::Color::BLUE);
                    dd.circle(center, base::Vector3::EZ(), 0.5f);
                    dd.color(base::Color::GREEN);
                    dd.circle(center, base::Vector3::EY(), 0.5f);
                    dd.color(base::Color::RED);
                    dd.circle(center, base::Vector3::EX(), 0.5f);

                    pos += base::Vector3::EY() * SEPARATION;
                }

                // fraction circles
                {
                    const auto center = pos + base::Vector3(0, 0, 0.1f);
                    float z = 0.0f;
                    dd.color(base::Color::CYAN);
                    dd.circle(center + base::Vector3(0.0f, 0.0f, z), base::Vector3::EZ(), 0.4f, -170.0f, 170.0f);
                    z += 0.3f;
                    dd.color(base::Color::MAGENTA);
                    dd.circle(center + base::Vector3(0.0f, 0.0f, z), base::Vector3::EZ(), 0.4f, -100.0f, 100.0f);
                    z += 0.3f;
                    dd.color(base::Color::YELLOW);
                    dd.circle(center + base::Vector3(0.0f, 0.0f, z), base::Vector3::EZ(), 0.4f, -30.0f, 30.0f);
                    z += 0.3f;
                    dd.color(base::Color::BLACK);
                    dd.circle(center + base::Vector3(0.0f, 0.0f, z), base::Vector3::EZ(), 0.4f, -5.0f, 5.0f);
                    z += 0.3f;
                    pos += base::Vector3::EY() * SEPARATION;
                }
            }

            void drawSolidShapes(scene::DebugGeometry& debugGeometry, base::Vector3& rowPos)
            {
                scene::DebugSolidDrawer dd(debugGeometry);
                scene::DebugLineDrawer dl(debugGeometry);

                // initial shift
                base::Vector3 pos = rowPos - base::Vector3::EY() * SEPARATION * 3.5f;
                rowPos += base::Vector3::EX() * SEPARATION;

                // box with corners
                {
                    const auto center = pos + base::Vector3(0, 0, 0.5f);
                    const auto extents = base::Vector3(0.4f, 0.4f, 0.4f);

                    dd.color(base::Color::CYAN);
                    dd.box(base::Box(center - extents, center + extents));

                    dl.color(base::Color::WHITE);
                    dl.box(base::Box(center - extents, center + extents));

                    pos += base::Vector3::EY() * SEPARATION;
                }

                // sphere
                {
                    const auto center = pos + base::Vector3(0, 0, 0.5f);

                    dd.color(base::Color(200, 200, 50));
                    dd.sphere(center, 0.4f);

                    dl.color(base::Color::WHITE);
                    dl.sphere(center, 0.4f);

                    pos += base::Vector3::EY() * SEPARATION;
                }

                // capsule
                {
                    const auto center = pos + base::Vector3(0, 0, 0.5f);

                    dd.color(base::Color::MAGENTA);
                    dd.capsule(center, 0.4f, 0.3f);

                    dl.color(base::Color::WHITE);
                    dl.capsule(center, 0.4f, 0.3f);

                    pos += base::Vector3::EY() * SEPARATION;
                }

                // cylinder
                {
                    const auto top = pos + base::Vector3(0, 0, 0.9f);
                    const auto bottom = pos + base::Vector3(0, 0, 0.1f);

                    dd.color(base::Color(255, 128, 64));
                    dd.cylinder(bottom, top, 0.5f, 0.3f);

                    dl.color(base::Color::WHITE);
                    dl.cylinder(bottom, top, 0.5f, 0.3f);

                    pos += base::Vector3::EY() * SEPARATION;
                }

                // cone
                {
                    const auto top = pos + base::Vector3(0, 0, 0.0f);

                    {
                        const auto dir = base::Vector3(1, 0, 3).normalized();

                        dd.color(base::Color::BLUE);
                        dd.cone(top, dir, 0.8f, 24.0f);

                        dl.color(base::Color::WHITE);
                        dl.cone(top, dir, 0.8f, 24.0f);
                    }

                    {
                        const auto dir = base::Vector3(-1, -1, 3).normalized();

                        dd.color(base::Color::GREEN);
                        dd.cone(top, dir, 0.8f, 16.0f);

                        dl.color(base::Color::WHITE);
                        dl.cone(top, dir, 0.8f, 16.0f);
                    }

                    {
                        const auto dir = base::Vector3(-1, 2, 3).normalized();

                        dd.color(base::Color::RED);
                        dd.cone(top, dir, 0.8f, 8.0f);

                        dl.color(base::Color::WHITE);
                        dl.cone(top, dir, 0.8f, 8.0f);
                    }

                    pos += base::Vector3::EY() * SEPARATION;
                }

                // planes
                {
                    const auto top = pos + base::Vector3(0, 0, 0.4f);

                    {
                        const auto dir = base::Vector3(-3, 0, 1).normalized();

                        dd.color(base::Color::BLUE);
                        dd.plane(top, dir, 0.8f, 1);

                        dl.color(base::Color::WHITE);
                        dl.plane(top, dir, 0.8f, 2);

                        dd.color(base::Color::GREEN);
                        dd.plane(top, dir, 0.6f, 1);

                        dl.color(base::Color::WHITE);
                        dl.plane(top, dir, 0.6f, 2);

                        dd.color(base::Color::RED);
                        dd.plane(top, dir, 0.4f, 1);

                        dl.color(base::Color::WHITE);
                        dl.plane(top, dir, 0.4f, 2);
                    }

                    pos += base::Vector3::EY() * SEPARATION;
                }

                // full circles
                {
                    const auto center = pos + base::Vector3(0, 0, 0.5f);

                    dd.color(base::Color::BLUE);
                    dd.circle(center, base::Vector3::EZ(), 0.5f);
                    dd.color(base::Color::GREEN);
                    dd.circle(center, base::Vector3::EY(), 0.5f);
                    dd.color(base::Color::RED);
                    dd.circle(center, base::Vector3::EX(), 0.5f);

                    dl.color(base::Color::WHITE);
                    dl.circle(center, base::Vector3::EZ(), 0.5f);
                    dl.circle(center, base::Vector3::EY(), 0.5f);
                    dl.circle(center, base::Vector3::EX(), 0.5f);

                    pos += base::Vector3::EY() * SEPARATION;
                }

                // fraction circles
                {
                    const auto center = pos + base::Vector3(0, 0, 0.1f);
                    float z = 0.0f;

                    dd.color(base::Color::CYAN);
                    dd.circle(center + base::Vector3(0.0f, 0.0f, z), base::Vector3::EZ(), 0.4f, -70.0f, 170.0f);

                    dl.color(base::Color::WHITE);
                    dl.circle(center + base::Vector3(0.0f, 0.0f, z), base::Vector3::EZ(), 0.4f, -70.0f, 170.0f);

                    z += 0.3f;

                    dd.color(base::Color::MAGENTA);
                    dd.circle(center + base::Vector3(0.0f, 0.0f, z), base::Vector3::EZ(), 0.4f, -100.0f, 100.0f);

                    dl.color(base::Color::WHITE);
                    dl.circle(center + base::Vector3(0.0f, 0.0f, z), base::Vector3::EZ(), 0.4f, -100.0f, 100.0f);

                    z += 0.3f;

                    dd.color(base::Color::YELLOW);
                    dd.circle(center + base::Vector3(0.0f, 0.0f, z), base::Vector3::EZ(), 0.4f, -30.0f, 30.0f);

                    dl.color(base::Color::WHITE);
                    dl.circle(center + base::Vector3(0.0f, 0.0f, z), base::Vector3::EZ(), 0.4f, -30.0f, 30.0f);

                    z += 0.3f;

                    dd.color(base::Color::BLACK);
                    dd.circle(center + base::Vector3(0.0f, 0.0f, z), base::Vector3::EZ(), 0.4f, -5.0f, 5.0f);

                    dl.color(base::Color::WHITE);
                    dl.circle(center + base::Vector3(0.0f, 0.0f, z), base::Vector3::EZ(), 0.4f, -5.0f, 5.0f);

                    z += 0.3f;
                    pos += base::Vector3::EY() * SEPARATION;
                }

                dd.flush();
                dl.flush();
            }

            void drawSceenShapes(scene::DebugGeometry& debugGeometry, base::Vector2& rowPos)
            {
                scene::DebugSolidDrawer dd(debugGeometry);
                scene::DebugLineDrawer dl(debugGeometry);

                dd.color(base::Color::RED);
                dd.rect(10, 10, 100, 100);
                dl.color(base::Color::WHITE);
                dl.rect(10, 10, 100, 100);

                dd.color(base::Color::WHITE);
                //dd.texture(resLena.loadAndGet() ? resLena.loadAndGet()->getRenderingObject() : nullptr);
                dd.rect(120, 10, 256, 256);

                dd.color(base::Color::WHITE);
                dd.text(10, 150, "Hello World!");

                base::Vector3 screenPos;
                //if (dd.screenPosition(base::Vector3(0.0f, 0.0f, 1.0f), screenPos))
                {
                    /*dd.color(base::Color::WHITE);
                    dd.color(base::Color::BLACK);
                    dd.textBox(screenPos.x, screenPos.y, base::TempString("Placed Text at {}", screenPos.z));*/
                }

                //if (dd.screenPosition(base::Vector3(0.0f, 0.0f, 0.5f), screenPos))
                {
                    /*dd.alignVertical(0);
                    dd.alignHorizontal(0);
                    dd.color(base::Color::WHITE);
                    dd.color(base::Color::BLACK);
                    dd.textBox(screenPos.x, screenPos.y, "First line: ^1RED^F\nSecond line: ^2GREEN");*/
                }
            }

        };

        RTTI_BEGIN_TYPE_CLASS(SceneTest_DebugGeometry);
            RTTI_METADATA(SceneTestOrderMetadata).order(20);
        RTTI_END_TYPE();
        
        //--

    } // test
} // rendering