/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: tests #]
***/

#include "build.h"
#include "sceneTest.h"

//#include "game/world/include/meshEntity.h"
#include "engine/world/include/entity.h"
#include "engine/world/include/world.h"

BEGIN_BOOMER_NAMESPACE_EX(test)

//---

/// a simple box on plane test, can be upscaled to more shapes
class SceneTest_SponzaPhysics : public ISceneTestEmptyWorld
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_SponzaPhysics, ISceneTestEmptyWorld);

public:
    virtual void configure() override
    {
        TBaseClass::configure();
    }

    virtual void update(float dt) override
    {
        TBaseClass::update(dt);
    }

    virtual void createWorldContent() override
    {
        if (auto mesh = loadMesh("/engine/scene/sponza/meshes/sponza.v4mesh"))
        {
            /*auto mc = RefNew<game::MeshEntity>();
            mc->mesh(mesh);
            m_world->attachEntity(mc);*/
        }

        if (auto mesh = loadMesh("/engine/meshes/cube.v4mesh"))
        {
            /*auto mc = RefNew<game::MeshEntity>();
            mc->mesh(mesh);
            mc->requestMove(Vector3(0, 0, 0.5f));
            m_mesh = mc;
            m_world->attachEntity(mc);*/
        }
    }

protected:
    EntityPtr m_mesh;
    float m_meshYaw = 0.0f;
};

RTTI_BEGIN_TYPE_CLASS(SceneTest_SponzaPhysics);
    RTTI_METADATA(SceneTestOrderMetadata).order(200);
RTTI_END_TYPE();

//---

END_BOOMER_NAMESPACE_EX(test)
