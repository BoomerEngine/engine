/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "game/world/include/world.h"
#include "game/world/include/worldPrefab.h"

namespace game
{
    namespace test
    {
        //---

        struct PlaneGround
        {
        public:
            PlaneGround(World* world, const rendering::MeshPtr& planeMesh);

            void ensureGroundUnder(float x, float y);

        private:
            base::HashSet<uint32_t> m_planeCoordinatesSet;

            World* m_world;

            rendering::MeshPtr m_planeMesh;
            float m_planeSize = 1.0f;
        };

        //---

        class PrefabBuilder
        {
        public:
            PrefabBuilder();

            int addMeshEntity(const rendering::MeshPtr& mesh, const NodeTemplatePlacement& placement, int parentNode = -1, base::Color color = base::Color::WHITE);
            int addPrefab(const game::PrefabPtr& prefab, const NodeTemplatePlacement& placement, int parentNode=-1);

            PrefabPtr extractPrefab();

        private:
            NodeTemplateContainerPtr m_container;
        };

        //---

        extern base::Point UlamSpiral(uint32_t n);

        //---

        extern void AttachPrefab(World* world, const PrefabPtr& prefab, const base::AbsoluteTransform& placement);

        extern void AttachEntities(World* world, const NodeTemplateCreatedEntities& nodes);

        //---

    } // test
} // game