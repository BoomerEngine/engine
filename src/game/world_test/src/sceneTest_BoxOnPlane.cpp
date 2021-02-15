/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: tests #]
***/

#include "build.h"
#include "sceneTest.h"

#include "game/world/include/meshEntity.h"
#include "base/world/include/worldEntity.h"
#include "base/world/include/world.h"

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
                TBaseClass::configure();
            }

            virtual void update(float dt) override
            {
                if (m_mesh)
                {
                    m_meshYaw += dt * 90.0f;

                    auto transform = m_mesh->absoluteTransform();
                    transform.rotation(base::Angles(0, m_meshYaw, 0));
                    m_mesh->requestTransform(transform);
                }

                TBaseClass::update(dt);
            }

            virtual void createWorldContent() override
            {
                if (auto mesh = loadMesh("/engine/meshes/plane.v4mesh"))
                {
                    auto mc = base::RefNew<game::MeshEntity>();
                    mc->mesh(mesh);
                    m_world->attachEntity(mc);
                }

                if (auto mesh = loadMesh("/engine/meshes/cube.v4mesh"))
                {
                    auto mc = base::RefNew<game::MeshEntity>();
                    mc->requestMove(base::Vector3(0, 0, 0.5f));
                    mc->mesh(mesh);
                    m_world->attachEntity(mc);
                }
            }

        protected:
            base::world::EntityPtr m_mesh;
            float m_meshYaw = 0.0f;
        };

        RTTI_BEGIN_TYPE_CLASS(SceneTest_BoxOnPlane);
            RTTI_METADATA(SceneTestOrderMetadata).order(10);
        RTTI_END_TYPE();

        //---

    } // test
} // game