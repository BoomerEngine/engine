/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "engine/world/include/world.h"
#include "engine/world/include/worldPrefab.h"

BEGIN_BOOMER_NAMESPACE_EX(test)

//---

struct PlaneGround
{
public:
    PlaneGround(World* world, const MeshRef& planeMesh);

    void ensureGroundUnder(float x, float y);

private:
    HashSet<uint32_t> m_planeCoordinatesSet;

    World* m_world;

    MeshRef m_planeMesh;
    float m_planeSize = 1.0f;
};

//---

class PrefabBuilder
{
public:
    PrefabBuilder();

    static NodeTemplatePtr BuildMeshNode(const MeshRef& mesh, const EulerTransform& placement, Color color = Color::WHITE);
    static NodeTemplatePtr BuildPrefabNode(const PrefabPtr& prefab, const EulerTransform& placement);

    int addNode(const NodeTemplatePtr& node, int parentNode = -1);

    PrefabPtr extractPrefab();

private:
    NodeTemplatePtr m_root;
    Array<NodeTemplatePtr> m_nodes;
};

//---

extern Point UlamSpiral(uint32_t n);

//---

END_BOOMER_NAMESPACE_EX(test)
