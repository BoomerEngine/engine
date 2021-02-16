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

#include "base/world/include/world.h"
#include "base/world/include/worldEntity.h"
#include "base/resource/include/objectIndirectTemplate.h"

#include "rendering/mesh/include/renderingMesh.h"
#include "game/world/include/meshEntity.h"

namespace game
{
    namespace test
    {

        //--

        PlaneGround::PlaneGround(base::world::World* world, const rendering::MeshRef& planeMesh)
            : m_world(world)
            , m_planeMesh(planeMesh)
            , m_planeSize(10.0f)
        {
            if (m_planeMesh)
                m_planeSize = planeMesh.load()->bounds().size().x;
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

                    auto entity = base::RefNew<game::MeshEntity>();
                    entity->requestTransform(entityTransform);
                    entity->mesh(m_planeMesh);

                    m_world->attachEntity(entity);
                }
            }
        }

        //--

        PrefabBuilder::PrefabBuilder()
        {
        }

        int PrefabBuilder::addNode(const base::world::NodeTemplatePtr& node, int parentNode /*= -1*/)
        {
            DEBUG_CHECK_RETURN_V(node, -1);

            if (parentNode < 0)
            {
                m_root = node;
            }
            else
            {
                DEBUG_CHECK_RETURN_V(parentNode <= m_nodes.lastValidIndex(), -1);
                auto& parent = m_nodes[parentNode];
                parent->m_children.pushBack(node);
            }

            m_nodes.pushBack(node);
            return m_nodes.lastValidIndex();
        }

        base::world::NodeTemplatePtr PrefabBuilder::BuildMeshNode(const rendering::MeshRef& mesh, const base::EulerTransform& placement, base::Color color /*= base::Color::WHITE*/)
        {
            DEBUG_CHECK_RETURN_V(mesh, nullptr);

            auto node = base::RefNew<base::world::NodeTemplate>();
            node->m_name = "node"_id;

            {
                node->m_entityTemplate = base::RefNew<base::ObjectIndirectTemplate>();
                node->m_entityTemplate->placement(placement);
                node->m_entityTemplate->templateClass(game::MeshEntity::GetStaticClass());
                node->m_entityTemplate->writeProperty("mesh"_id, rendering::MeshAsyncRef(mesh.path()));
                node->m_entityTemplate->writeProperty("color"_id, color);
            }

            return node;
        }

        base::world::NodeTemplatePtr PrefabBuilder::BuildPrefabNode(const base::world::PrefabPtr& prefab, const base::EulerTransform& placement)
        {
            DEBUG_CHECK_RETURN_V(prefab, nullptr);

            auto node = base::RefNew<base::world::NodeTemplate>();
            node->m_prefabAssets.pushBack(prefab);

            return node;
        }

        base::world::PrefabPtr PrefabBuilder::extractPrefab()
        {
            auto ret = base::RefNew<base::world::Prefab>();
            ret->setup(m_root);
            return ret;
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

    } // test
} // rendering