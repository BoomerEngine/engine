/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: tests #]
***/

#include "build.h"
#include "sceneTest.h"
#include "sceneTestUtil.h"

#include "engine/rendering/include/debugGeometryBuilder.h"

BEGIN_BOOMER_NAMESPACE_EX(test)

//---

/// debug geometry
class SceneTest_DebugGeometry : public ISceneTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_DebugGeometry, ISceneTest);

public:
    virtual void configure() override
    {
        TBaseClass::configure();
    }

    virtual void update(float dt) override
    {
        TBaseClass::update(dt);
    }

    virtual void renderDebug(DebugGeometryCollector& debug) override
    {
        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);
            b.color(Color::RED);
            b.solidBox(Box(Vector3(0, 0, 0.2f), 0.2f));
            b.color(Color::GREEN);
            b.solidBox(Box(Vector3(0, 0, 0.6f), 0.2f));
            b.color(Color::BLUE);
            b.solidBox(Box(Vector3(0, 0, 1.0f), 0.2f));
            debug.push(b);
        }

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);
            b.shading(true);
            b.color(Color::RED);
            b.solidBox(Box(Vector3(0, 1, 0.2f), 0.2f));
            b.color(Color::GREEN);
            b.solidBox(Box(Vector3(0, 1, 0.6f), 0.2f));
            b.color(Color::BLUE);
            b.solidBox(Box(Vector3(0, 1, 1.0f), 0.2f));
            debug.push(b);
        }

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);
            b.edges(true);
            b.color(Color::RED);
            b.solidBox(Box(Vector3(0, -1, 0.2f), 0.2f));
            b.color(Color::GREEN);
            b.solidBox(Box(Vector3(0, -1, 0.6f), 0.2f));
            b.color(Color::BLUE);
            b.solidBox(Box(Vector3(0, -1, 1.0f), 0.2f));
            debug.push(b);
        }

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);
            b.edges(true);
            b.shading(true);
            b.color(Color::RED);
            b.solidBox(Box(Vector3(0, -2, 0.2f), 0.2f));
            b.color(Color::GREEN);
            b.solidBox(Box(Vector3(0, -2, 0.6f), 0.2f));
            b.color(Color::BLUE);
            b.solidBox(Box(Vector3(0, -2, 1.0f), 0.2f));
            debug.push(b);
        }

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);
            b.color(Color::LIGHTSTEELBLUE);
            b.size(2.0f);
            b.wireSphere(Vector3(1, -3, 0.5f), 0.5f);
            debug.push(b);
        }

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);
            b.color(Color::LIGHTSTEELBLUE);
            b.wireSphere(Vector3(1, 2, 0.4f), 0.4f);
            debug.push(b);
        }

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);
            b.edges(true);
            b.shading(true);
            b.color(Color::ORANGE);
            b.solidSphere(Vector3(1, 0, 0.2f), 0.2f);
            debug.push(b);
        }

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);
            b.edges(true);
            b.shading(true);
            b.color(Color::CYAN);
            b.solidCapsule(Vector3(1, 1, 0.0f), Vector3(1,1, 0.1f), 0.2f);
            debug.push(b);
        }

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);
            b.edges(true);
            b.shading(true);
            b.color(Color::MAGENTA);
            b.solidCapsule(Vector3(1, -1, 0.0f), Vector3(1, -1, 1.0f), 0.2f);
            debug.push(b);
        }

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);
            b.color(Color::PURPLE);
            b.size(3.0f);
            b.wireCapsule(Vector3(1, -2, 0.0f), Vector3(1, -2, 1.0f), 0.25f);
            debug.push(b);
        }

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);
            b.shading(true);
            b.color(Color::DARKORANGE);
            b.solidSphere(Vector3(2, 0, 0.2f), 0.2f);
            debug.push(b);
        }

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);
            //b.shading(true);
            b.color(Color::DARKCYAN);
            b.solidCapsule(Vector3(2, 1, 0.0f), Vector3(2, 1, 0.1f), 0.2f);
            debug.push(b);
        }

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);
            b.shading(true);
            b.color(Color::DARKMAGENTA);
            b.solidCapsule(Vector3(2, -1, 0.0f), Vector3(2, -1.5, 1.0f), 0.2f);
            debug.push(b);
        }



        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);
            b.shading(true);
            b.edges(true);
            b.color(Color::DARKKHAKI);
            b.solidCylinder(Vector3(3, 0, 0.0f), Vector3(3, 0, 1.0f), 0.3f, 0.2f);
            debug.push(b);
        }

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);
            b.shading(true);
            b.edges(true);
            b.color(Color::YELLOW);
            b.solidCylinder(Vector3(3, 1, 0.0f), Vector3(3, 1, 0.6f), 0.4f, 0.4f);
            debug.push(b);
        }

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);
            b.color(Color::DARKSLATEGRAY);
            b.wireCylinder(Vector3(3, 2, 0.0f), Vector3(3, 2, 0.6f), 0.4f, 0.4f);
            debug.push(b);
        }

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);
            b.shading(true);
            b.color(Color::RED);
            b.solidCylinder(Vector3(3, -1, 0.0f), Vector3(3, -1, 0.33f), 0.3f, 0.2f);
            b.color(Color::GREEN);
            b.solidCylinder(Vector3(3, -1, 0.33f), Vector3(3, -1, 0.66f), 0.2f, 0.1f);
            b.color(Color::BLUE);
            b.solidCylinder(Vector3(3, -1, 0.66f), Vector3(3, -1, 1.0f), 0.1f, 0.0f);
            debug.push(b);
        }

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);
            b.shading(true);
            b.color(Color::RED);
            b.wireCylinder(Vector3(3, -2, 0.0f), Vector3(3, -2, 0.33f), 0.3f, 0.2f);
            b.color(Color::GREEN);
            b.wireCylinder(Vector3(3, -2, 0.33f), Vector3(3, -2, 0.66f), 0.2f, 0.1f);
            b.color(Color::BLUE);
            b.wireCylinder(Vector3(3, -2, 0.66f), Vector3(3, -2, 1.0f), 0.1f, 0.0f);
            debug.push(b);
        }

        {
            float w = 0.02f;
            float l = 0.5f;
            float p = 0.8f;

            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);
            b.shading(true);
            b.color(Color::RED);
            b.solidArrow(Vector3(3, -4, 0.0f), Vector3(3 + 1.0f, -4, 0.0f));
            b.color(Color::GREEN);
            b.solidArrow(Vector3(3, -4, 0.0f), Vector3(3, -4 + 1.0f, 0.0f));
            b.color(Color::BLUE);
            b.solidArrow(Vector3(3, -4, 0.0f), Vector3(3, -4, 1.0f));
            debug.push(b);
        }


        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);
            b.color(Color::BLUE);
            b.wire(Vector3(4, 0, 0.25f), Vector3(4, 0, 0.75f));
            b.color(Color::GREEN);
            b.wire(Vector3(4, 0.25f,0.5f), Vector3(4, -0.25f,0.5f));
            b.color(Color::RED);
            b.wire(Vector3(3.75f, 0, 0.5f), Vector3(4.25f, 0, 0.5f));
            debug.push(b);
        }

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);
            b.size(10.0f);
            b.color(Color::BLUE);
            b.wire(Vector3(4, -1, 0.25f), Vector3(4, -1, 0.75f));
            b.color(Color::GREEN);
            b.wire(Vector3(4, -1+0.25f, 0.5f), Vector3(4, -1 -0.25f, 0.5f));
            b.color(Color::RED);
            b.wire(Vector3(3.75f, -1, 0.5f), Vector3(4.25f, -1, 0.5f));
            debug.push(b);
        }

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);
            b.size(5.0f);
            b.color(Color::CYAN);
            b.wireBox(Box(Vector3(4, 1, 0.5f), 0.25f));
            debug.push(b);
        }

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);
            b.size(2.0f);
            b.color(Color::LIGHTGREY);
            b.wireGrid(Vector3::ZERO(), Vector3::EZ(), 20.0f, 20);
            debug.push(b);
        }

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);
            b.size(64.0f);
            b.color(Color::LIGHTGREEN);
            b.sprite(Vector3(5, 0, 0.5f));
            debug.push(b);
        }

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);

            Vector3 corners[8];
            Box(Vector3(5, -1, 0.5f), 0.2f).corners(corners);

            b.size(10.0f);
            b.color(Color::CYAN);
            b.sprites(corners, 8);
            debug.push(b);
        }

        // TODO: icons

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);

            b.size(1.0f);
            b.color(Color::RED);
            b.wireCircle(Vector3(6, 0, 0.5f), Vector3::EX(), 0.3f, true);
            b.color(Color::GREEN);
            b.wireCircle(Vector3(6, 0, 0.5f), Vector3::EY(), 0.3f, true);
            b.color(Color::BLUE);
            b.wireCircle(Vector3(6, 0, 0.5f), Vector3::EZ(), 0.3f, true);
            debug.push(b);
        }

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);

            b.size(5.0f);
            b.color(Color::RED);
            b.wireCircle(Vector3(6, -1, 0.5f), Vector3::EX(), 0.4f, true);
            b.color(Color::GREEN);
            b.wireCircle(Vector3(6, -1, 0.5f), Vector3::EY(), 0.4f, true);
            b.color(Color::BLUE);
            b.wireCircle(Vector3(6, -1, 0.5f), Vector3::EZ(), 0.4f, true);
            debug.push(b);
        }

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);

            b.color(Color::RED);
            b.solidCircle(Vector3(6, -2, 0.5f), Vector3::EX(), 0.3f);
            b.color(Color::GREEN);
            b.solidCircle(Vector3(6, -2, 0.5f), Vector3::EY(), 0.3f);
            b.color(Color::BLUE);
            b.solidCircle(Vector3(6, -2, 0.5f), Vector3::EZ(), 0.3f);
            debug.push(b);
        }

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);

            b.size(1.0f);
            b.color(Color::RED);
            b.wireCircle(Vector3(6, 1, 0.5f), Vector3::EX(), 0.3f, false);
            b.color(Color::GREEN);
            b.wireCircle(Vector3(6, 1, 0.5f), Vector3::EY(), 0.3f, false);
            b.color(Color::BLUE);
            b.wireCircle(Vector3(6, 1, 0.5f), Vector3::EZ(), 0.3f, false);
            debug.push(b);
        }


        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);

            b.size(3.0f);
            b.color(Color::GRAY);
            b.wireCircle(Vector3(7, 0, 0.5f), Vector3::EX(), 0.3f, -30.0f, 30.0f, false);
            b.wireCircle(Vector3(7, 0, 0.5f), Vector3::EX(), 0.3f, 150.0f, 210.0f, false);
            b.color(Color::YELLOW);
            b.wireCircle(Vector3(7, 0, 0.5f), Vector3::EX(), 0.3f, 60.0f, 120.0f, true);
            b.wireCircle(Vector3(7, 0, 0.5f), Vector3::EX(), 0.3f, 240.0f, 300.0f, true);
            debug.push(b);
        }

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);

            b.size(3.0f);
            b.color(Color::AQUAMARINE);
            b.solidCircleCut(Vector3(7, -1, 0.5f), Vector3::EZ(), 0.42f, 0.48f);
            b.color(Color::ALICEBLUE);
            b.wireCircleCut(Vector3(7, -1, 0.5f), Vector3::EZ(), 0.32f, 0.38f, false);
            b.color(Color::BISQUE);
            b.wireCircleCut(Vector3(7, -1, 0.5f), Vector3::EZ(), 0.21f, 0.29f, true);
            debug.push(b);
        }

        {
            DebugGeometryBuilder b(DebugGeometryLayer::SceneSolid);

            b.size(3.0f);
            b.color(Color::GRAY);
            b.wireCircleCut(Vector3(7, -2, 0.5f), Vector3::EX(), 0.3f, 0.4f,  -30.0f, 30.0f, false);
            b.wireCircleCut(Vector3(7, -2, 0.5f), Vector3::EX(), 0.3f, 0.4f, 150.0f, 210.0f, false);
            b.color(Color::DARKGRAY);
            b.solidCircleCut(Vector3(7, -2, 0.5f), Vector3::EX(), 0.4f, 0.5f, -40.0f, 40.0f);
            b.solidCircleCut(Vector3(7, -2, 0.5f), Vector3::EX(), 0.4f, 0.5f, 140.0f, 220.0f);
            b.color(Color::YELLOW);
            b.wireCircleCut(Vector3(7, -2, 0.5f), Vector3::EX(), 0.3f, 0.4f, 60.0f, 120.0f, true);
            b.wireCircleCut(Vector3(7, -2, 0.5f), Vector3::EX(), 0.3f, 0.4f, 240.0f, 300.0f, true);
            debug.push(b);
        }
    }

};

RTTI_BEGIN_TYPE_CLASS(SceneTest_DebugGeometry);
    RTTI_METADATA(SceneTestOrderMetadata).order(5);
RTTI_END_TYPE();

//---

END_BOOMER_NAMESPACE_EX(test)
