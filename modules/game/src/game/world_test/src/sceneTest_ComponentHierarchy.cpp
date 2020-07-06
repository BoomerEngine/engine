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

#include "game/world/include/worldMeshComponent.h"
#include "game/world/include/worldEntity.h"
#include "game/world/include/world.h"
#include "base/math/include/randomMersenne.h"
#include "rendering/mesh/include/renderingMesh.h"

namespace game
{
    namespace test
    {
        //---

        /// simple transform hierarchy of components
        class SceneTest_ComponentHierarchy : public ISceneTestEmptyWorld
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_ComponentHierarchy, ISceneTestEmptyWorld);

        public:
            virtual void configure() override
            {
                TBaseClass::configure();

                if (ImGui::Begin("Scene"))
                {
                    bool changed = false;

                    ImGui::Text("Scene configuration");
                    changed |= ImGui::SliderInt("Tower count", &st_RootCount, 0, 500);
                    changed |= ImGui::SliderInt("Tower height", &st_TowerHeight, 1, 10);
                    changed |= ImGui::SliderFloat("Separation", &st_RootDistance, 0.1f, 10.0, "%.2f", 2.0f);
                    changed |= ImGui::Combo("Mesh Type", &st_MeshType, "Box\0Cylinder\0Sphere\0Random");
                    ImGui::Checkbox("Rotate meshes", &st_RotateMeshes);                    

                    if (st_TowerHeight * st_RootCount > 300)
                    {
                        if (ImGui::Button("Recreate"))
                            recreateWorld();
                    }
                    else if (changed)
                    {
                        recreateWorld();
                    }

                    ImGui::End();
                }
            }

            virtual void update(float dt) override
            {
                if (st_RotateMeshes)
                {
                    m_totalTime += dt;

                    for (const auto& mesh : m_meshes)
                    {
                        const auto yaw = m_totalTime * 10.0f;
                        mesh->relativeRotation(base::Angles(0, yaw, 0).toQuat());
                    }
                }

                TBaseClass::update(dt);
            }

            static int SelectMesh(base::MTGenerator& rng, int meshCount)
            {
                if (st_MeshType < meshCount)
                    return st_MeshType;

                return rng.nextUint32() % meshCount;
            }

            virtual void createWorldContent() override
            {
                rendering::MeshPtr meshes[3];
                meshes[0] = loadMesh("cube.obj");
                meshes[1] = loadMesh("cylinder.obj");
                meshes[2] = loadMesh("sphere.obj");

                m_meshes.reset();

                PlaneGround groundPlane(m_world, loadMesh("plane.obj"));

                base::MTGenerator rng;

                for (uint32_t n = 0; n < st_RootCount; ++n)
                {
                    base::AbsoluteTransform entityTransform;
                    const auto center = UlamSpiral(n).toVector() * st_RootDistance;
                    entityTransform.position(center.xyz(0.5f));
                    groundPlane.ensureGroundUnder(center.x, center.y);

                    auto entity = base::CreateSharedPtr<Entity>();
                    entity->requestTransform(entityTransform);

                    ComponentPtr prevComponent;
                    for (uint32_t k = 0; k < st_TowerHeight; ++k)
                    {
                        auto meshIndex = SelectMesh(rng, ARRAY_COUNT(meshes));

                        auto mc = base::CreateSharedPtr<MeshComponent>(meshes[meshIndex]);
                        if (prevComponent)
                        {
                            mc->relativePosition(base::Vector3(0, 0, 1.0f));
                            mc->linkTransform(prevComponent);
                        }

                        entity->attachComponent(mc);
                        prevComponent = mc;

                        m_meshes.pushBack(mc);
                    }

                    m_world->attachEntity(entity);
                }
            }

        protected:
            base::Array<base::RefPtr<MeshComponent>> m_meshes;
            float m_totalTime = 0.0f;
            static int st_RootCount;
            static float st_RootDistance;
            static int st_TowerHeight;
            static int st_MeshType;
            static bool st_RotateMeshes;
        };

        int SceneTest_ComponentHierarchy::st_RootCount = 1;
        int SceneTest_ComponentHierarchy::st_TowerHeight = 2;
        int SceneTest_ComponentHierarchy::st_MeshType = 0;
        float SceneTest_ComponentHierarchy::st_RootDistance = 2.0f;
        bool SceneTest_ComponentHierarchy::st_RotateMeshes = true;

        RTTI_BEGIN_TYPE_CLASS(SceneTest_ComponentHierarchy);
            RTTI_METADATA(SceneTestOrderMetadata).order(50);
        RTTI_END_TYPE();

        //---

    } // test
} // game