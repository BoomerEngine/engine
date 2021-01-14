/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "base/world/include/world.h"
#include "base/world/include/worldPrefab.h"

namespace game
{
    namespace test
    {
        //---

        struct PlaneGround
        {
        public:
            PlaneGround(base::world::World* world, const rendering::MeshPtr& planeMesh);

            void ensureGroundUnder(float x, float y);

        private:
            base::HashSet<uint32_t> m_planeCoordinatesSet;

            base::world::World* m_world;

            rendering::MeshPtr m_planeMesh;
            float m_planeSize = 1.0f;
        };

        //---

        class PrefabBuilder
        {
        public:
            PrefabBuilder();

            static base::world::NodeTemplatePtr BuildMeshNode(const rendering::MeshPtr& mesh, const base::EulerTransform& placement, base::Color color = base::Color::WHITE);
            static base::world::NodeTemplatePtr BuildPrefabNode(const base::world::PrefabPtr& prefab, const base::EulerTransform& placement);

            int addNode(const base::world::NodeTemplatePtr& node, int parentNode = -1);

            base::world::PrefabPtr extractPrefab();

        private:
            base::world::NodeTemplatePtr m_root;
            base::Array<base::world::NodeTemplatePtr> m_nodes;
        };

        //---

        extern base::Point UlamSpiral(uint32_t n);

        //---

    } // test
} // game