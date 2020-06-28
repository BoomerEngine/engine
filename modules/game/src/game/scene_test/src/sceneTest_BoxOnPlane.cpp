/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: tests #]
***/

#include "build.h"
#include "sceneTest.h"

#include "game/scene/include/gameMeshComponent.h"
#include "game/scene/include/gameEntity.h"
#include "game/scene/include/world.h"

namespace game
{
    namespace test
    {
        //---

        /// a simple box on plane test, can be upscaled to more shapes
        class SceneTest_BoxOnPlane : public ISceneTestEmptyWorld
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_BoxOnPlane, ISceneTestEmptyWorld);

        public:
            virtual void configure() override
            {

            }

            virtual void update(float dt) override
            {
                if (m_mesh)
                {
                    m_meshYaw += dt * 90.0f;
                    m_mesh->relativeRotation(base::Angles(0, m_meshYaw, 0).toQuat());
                }

                TBaseClass::update(dt);
            }

            virtual void createWorldContent() override
            {
                if (auto mesh = loadMesh("plane.obj"))
                {
                    auto mc = base::CreateSharedPtr<MeshComponent>(mesh);

                    auto entity = base::CreateSharedPtr<Entity>();
                    entity->attachComponent(mc);

                    m_world->attachEntity(entity);
                }

                if (auto mesh = loadMesh("cube.obj"))
                {
                    auto mc = base::CreateSharedPtr<MeshComponent>(mesh);
                    mc->relativePosition(base::Vector3(0, 0, 0.5f));
                    m_mesh = mc;

                    auto entity = base::CreateSharedPtr<Entity>();
                    entity->attachComponent(mc);

                    m_world->attachEntity(entity);
                }
            }

        protected:
            base::RefPtr<MeshComponent> m_mesh;
            float m_meshYaw = 0.0f;
        };

        RTTI_BEGIN_TYPE_CLASS(SceneTest_BoxOnPlane);
            RTTI_METADATA(SceneTestOrderMetadata).order(10);
        RTTI_END_TYPE();

        //---

    } // test
} // game