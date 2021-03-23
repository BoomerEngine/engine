/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "engine/world/include/world.h"

BEGIN_BOOMER_NAMESPACE_EX(test)

//---

class SceneBuilder : public NoCopy
{
public:
    SceneBuilder();
    ~SceneBuilder();

    //--

    void transform(const EulerTransform& placement);
    void pushTransform();
    void popTransform();

    void deltaTranslate(Vector3 delta);
    void deltaTranslate(float x = 0.0f, float y = 0.0f, float z = 0.0f);
    void deltaRotate(Angles delta);
    void deltaRotate(float pitch = 0.0f, float yaw = 0.0f, float roll = 0.0f);

    void color(Color color);

    void pushParent(RawEntity* entity);
    void popParent();

    void ensureGroundUnder(Vector3 pos);

    EulerTransform transform();

    ResourceID mapResource(StringView path);

    void toggleTransformParent(bool flag);

    RawEntityPtr buildMeshNode(ResourceID id, StringView customName="");
    RawEntityPtr buildPrefabNode(const PrefabPtr& prefab, StringView customName = "");

    //--

    PrefabPtr extractPrefab();

    CompiledWorldPtr extractWorld();

    //--

private:
    Array<RawEntityPtr> m_rootNodes;
    Array<RawEntityPtr> m_allNodes;

    Array<RawEntityPtr> m_parentStack;
    Array<EulerTransform> m_transformStack;

    EulerTransform m_transform;

    HashSet<uint32_t> m_planeCoordinatesSet;

    bool m_flagTransformParent = false;
    
    Color m_color;

    MeshRef m_planeMesh;
    Vector2 m_planeSize;

    RawEntityPtr createCommonNode(ObjectIndirectTemplate* data, StringView customName);
    void commonProcessNode(RawEntity* entity);

    uint32_t m_nameCounter = 0;

    Array<PrefabPtr> m_capturedPrefabs;

    HashMap<StringBuf, ResourceID> m_localResourceLookup;
};

//---

extern Point UlamSpiral(uint32_t n);

//---

END_BOOMER_NAMESPACE_EX(test)
