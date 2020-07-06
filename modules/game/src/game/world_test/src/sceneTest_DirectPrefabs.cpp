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

namespace game
{
    namespace test
    {
        //---

        /// a simple one-layer deep prefab instantiation
        class SceneTest_DirectPrefabs : public ISceneTestEmptyWorld
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_DirectPrefabs, ISceneTestEmptyWorld);

        public:
            int m_numPrefabs = 1;
            int m_sceneType = 0;
            float m_placementDistance = 3.0f;

            virtual void configure() override
            {
                TBaseClass::configure();

                if (ImGui::Begin("Scene"))
                {
                    bool changed = false;

                    ImGui::Text("Scene configuration");
                    ImGui::SliderInt("Num prefabs", &m_numPrefabs, 0, 500);
                    ImGui::SliderFloat("Prefab distance", &m_placementDistance, 0.1f, 20.0f);
                    ImGui::Combo("Scene type", &m_sceneType, "Simple prefab\0Prefab in prefab\0Prefab with child entity");
                    
                    if (ImGui::Button("Recreate"))
                        recreateWorld();

                    ImGui::End();
                }
            }

            virtual void update(float dt) override
            {
                TBaseClass::update(dt);
            }

            PrefabPtr buildPrefab()
            {
                PrefabBuilder prefabBuilder;

                if (auto mesh = loadMesh("cube.obj"))
                {
                    NodeTemplatePlacement placement;
                    placement.T = base::Vector3(0, 0, 0.5f);
                    prefabBuilder.addMeshEntity(mesh, placement);
                }

                return prefabBuilder.extractPrefab();
            }

            PrefabPtr buildPrefabInPrefab()
            {
                PrefabBuilder prefabBuilder;

                const auto innerPrefab = buildPrefab();

                NodeTemplatePlacement placement;
                placement.T = base::Vector3(0, 0, 0.5f);
                prefabBuilder.addPrefab(innerPrefab, placement);

                return prefabBuilder.extractPrefab();
            }

            PrefabPtr buildPrefabChildEntity()
            {
                PrefabBuilder prefabBuilder;

                if (auto mesh = loadMesh("cube.obj"))
                {
                    NodeTemplatePlacement placement;
                    placement.T = base::Vector3(0, 0, 0.5f);
                    int parent = prefabBuilder.addMeshEntity(mesh, placement);

                    if (auto mesh = loadMesh("sphere.obj"))
                    {
                        NodeTemplatePlacement placement;
                        placement.T = base::Vector3(0, 0, 1.5f);
                        prefabBuilder.addMeshEntity(mesh, placement, parent);
                    }
                }

                return prefabBuilder.extractPrefab();
            }

            PrefabPtr selectPrefab()
            {
                switch (m_sceneType)
                {
                    case 0: return buildPrefab();
                    case 1: return buildPrefabInPrefab();
                    case 2: return buildPrefabChildEntity();
                }

                return nullptr;
            }

            virtual void createWorldContent() override
            {
                PlaneGround ground(m_world, loadMesh("plane.obj"));

                const auto prefab = selectPrefab();

                for (uint32_t i=0; i<m_numPrefabs; ++i)
                {
                    AbsoluteTransform placement;
                    placement.position(UlamSpiral(i).toVector().xyz() * m_placementDistance);
                    ground.ensureGroundUnder(placement.position().approximate().x, placement.position().approximate().y);

                    AttachPrefab(m_world, prefab, placement);
                }
            }

        protected:
            base::RefPtr<MeshComponent> m_mesh;
        };

        RTTI_BEGIN_TYPE_CLASS(SceneTest_DirectPrefabs);
            RTTI_METADATA(SceneTestOrderMetadata).order(20);
        RTTI_END_TYPE();

        //---

    } // test
} // game