/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#include "build.h"
#include "sceneTest.h"
#include "sceneTestUtil.h"

#include "game/world/include/world.h"
#include "game/world/include/worldEntity.h"
#include "game/world/include/worldDefinition.h"

#include "rendering/mesh/include/renderingMesh.h"
#include "game/world/include/worldMeshComponent.h"
#include "game/world/include/worldMeshComponentTemplate.h"
#include "game/world/include/worldNodePath.h"

namespace game
{
    namespace test
    {

        //--

        PlaneGround::PlaneGround(World* world, const rendering::MeshPtr& planeMesh)
            : m_world(world)
            , m_planeMesh(planeMesh)
            , m_planeSize(10.0f)
        {
            if (m_planeMesh)
                m_planeSize = planeMesh->bounds().size().x;
        }

        void PlaneGround::ensureGroundUnder(float x, float y)
        {
            if (m_planeMesh)
            {
                int planeX = (int)std::roundf(x / m_planeSize);
                int planeY = (int)std::roundf(y / m_planeSize);
                const auto planeCode = (planeX + 32000) + (planeY + 32000) * 65535;
                if (m_planeCoordinatesSet.insert(planeCode))
                {
                    base::AbsoluteTransform entityTransform;
                    entityTransform.position(planeX * m_planeSize, planeY * m_planeSize, 0);

                    auto entity = base::CreateSharedPtr<Entity>();
                    entity->requestTransform(entityTransform);

                    auto mc = base::CreateSharedPtr<MeshComponent>(m_planeMesh);
                    entity->attachComponent(mc);

                    m_world->attachEntity(entity);
                }
            }
        }

        //--

        PrefabBuilder::PrefabBuilder()
        {
            m_container = base::CreateSharedPtr<NodeTemplateContainer>();
        }

        int PrefabBuilder::addMeshEntity(const rendering::MeshPtr& mesh, const NodeTemplatePlacement& placement, int parentNode, base::Color color)
        {
            if (!mesh)
                return 0;

            NodeTemplateConstructionInfo data;
            data.name = "MeshNode"_id;

            {
                auto componentTemplate = base::CreateSharedPtr<MeshComponentTemplate>();
                componentTemplate->m_mesh = mesh->key();
                componentTemplate->m_placement = placement;
                componentTemplate->m_color = color;
                data.componentData.emplaceBack("Mesh"_id, componentTemplate);
            }

            auto node = base::CreateSharedPtr<NodeTemplate>(data);
            return m_container->addNode(node, parentNode);
        }

        int PrefabBuilder::addPrefab(const game::PrefabPtr& prefab, const NodeTemplatePlacement& placement, int parentNode)
        {
            if (!prefab)
                return 0;

            NodeTemplateConstructionInfo data;
            data.name = "PrefabBone"_id;
            data.prefabAssets.pushBack(prefab);

            auto node = base::CreateSharedPtr<NodeTemplate>(data);
            return m_container->addNode(node, parentNode);
        }

        PrefabPtr PrefabBuilder::extractPrefab()
        {
            return base::CreateSharedPtr<Prefab>(m_container);
        }

        //--

        base::Point UlamSpiral(uint32_t n)
        {
            auto k = std::ceilf((std::sqrtf(n) - 1) / 2);
            auto t = 2 * k + 1;
            auto m = t * t;

            t = t - 1;

            if (n >= m - t)
                return base::Point(k - (m - n), -k);
            else
                m = m - t;

            if (n >= m - t)
                return base::Point(-k, -k + (m - n));
            else
                m = m - t;

            if (n >= m - t)
                return base::Point(-k + (m - n), k);
            else
                return base::Point(k, k - (m - n - t));
        }

        //--

        void AttachPrefab(World* world, const PrefabPtr& prefab, const base::AbsoluteTransform& placement)
        {
            if (prefab)
            {
                if (auto nodes = prefab->compile())
                {
                    NodeTemplateCreatedEntities entities;
                    nodes->createSingleRoot(0, game::NodePath(), placement, entities);
                    AttachEntities(world, entities);
                }
            }
        }

        //--

        void AttachEntities(World* world, const NodeTemplateCreatedEntities& nodes)
        {
            for (const auto& entity : nodes.allEntities)
                world->attachEntity(entity);
        }

        //--

    } // test
} // rendering