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

BEGIN_BOOMER_NAMESPACE_EX(test)

//---

/// a simple box on plane test, can be upscaled to more shapes
class SceneTest_BoxOnPlane : public ISceneTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_BoxOnPlane, ISceneTest);

public:
    virtual void configure() override
    {
        TBaseClass::configure();
    }

    virtual void update(float dt) override
    {
        if (auto mesh = world()->findEntityByStaticPath("/box"))
        {
            m_meshYaw += dt * 90.0f;

            auto transform = mesh->cachedWorldTransform();
            transform.R = Angles(0, m_meshYaw, 0).toQuat();
            mesh->requestTransformChange(transform);
        }

        TBaseClass::update(dt);
    }

    virtual CompiledWorldDataPtr createStaticContent()
    {
        SceneBuilder b;

        {
            const auto plane = b.mapResource("/engine/meshes/plane.xmeta");
            b.buildMeshNode(plane);
        }

        {
            const auto box = b.mapResource("/engine/meshes/cube.xmeta");
            b.deltaTranslate(0, 0, 0.5f);
            b.buildMeshNode(box, "box");
        }

        return b.extractWorld();
    }
    
protected:
    float m_meshYaw = 0.0f;
};

RTTI_BEGIN_TYPE_CLASS(SceneTest_BoxOnPlane);
    RTTI_METADATA(SceneTestOrderMetadata).order(10);
RTTI_END_TYPE();

//---

END_BOOMER_NAMESPACE_EX(test)
