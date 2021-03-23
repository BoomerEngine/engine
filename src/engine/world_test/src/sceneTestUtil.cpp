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

#include "engine/world/include/world.h"
#include "engine/world/include/worldEntity.h"
#include "engine/world/include/rawEntity.h"
#include "engine/world/include/rawPrefab.h"
#include "engine/world/include/compiledWorldData.h"

#include "engine/world_entities/include/entityMeshBase.h"
#include "engine/world_entities/include/entitySolidMesh.h"
#include "engine/world_compiler/include/streamingEntitySoup.h"
#include "engine/world_compiler/include/streamingGrid.h"
#include "engine/world_compiler/include/streamingIslandGeneration.h"

#include "engine/mesh/include/mesh.h"

#include "core/resource/include/indirectTemplate.h"
#include "core/resource/include/depot.h"

BEGIN_BOOMER_NAMESPACE_EX(test)

//--

SceneBuilder::SceneBuilder()
{
    static const auto planeMesh = LoadResourceRef<Mesh>("/engine/meshes/plane.xmeta");
    m_planeMesh = planeMesh;

    if (const auto mesh = m_planeMesh.resource())
        m_planeSize = mesh->bounds().size().xy();
}

SceneBuilder::~SceneBuilder()
{

}

void SceneBuilder::transform(const EulerTransform& placement)
{
    m_transform = placement;
}

void SceneBuilder::pushTransform()
{
    m_transformStack.pushBack(m_transform);
}

void SceneBuilder::popTransform()
{
    DEBUG_CHECK_RETURN(!m_transformStack.empty());
    m_transform = m_transformStack.back();
    m_transformStack.popBack();
}

void SceneBuilder::deltaTranslate(Vector3 delta)
{
    m_transform.T += delta;
}

void SceneBuilder::deltaTranslate(float x, float y, float z)
{
    m_transform.T += Vector3(x, y, z);
}

void SceneBuilder::deltaRotate(Angles delta)
{
    m_transform.R += delta;
}

void SceneBuilder::deltaRotate(float pitch, float yaw, float roll)
{
    m_transform.R += Angles(pitch, yaw, roll);
}

void SceneBuilder::color(Color color)
{
    m_color = color;
}

void SceneBuilder::pushParent(RawEntity* entity)
{
    DEBUG_CHECK_RETURN(entity);
    m_parentStack.pushBack(AddRef(entity));
    m_transformStack.pushBack(m_transform);
    m_transform = EulerTransform::IDENTITY();
}

void SceneBuilder::popParent()
{
    DEBUG_CHECK_RETURN(!m_parentStack.empty());
    m_parentStack.popBack();
    m_transform = m_transformStack.back();
    m_transformStack.popBack();
}

EulerTransform SceneBuilder::transform()
{
    return m_transform;
}

void SceneBuilder::ensureGroundUnder(Vector3 pos)
{
    if (pos.z > 0.0f && m_planeMesh)
    {
        int planeX = (int)std::roundf(pos.x / m_planeSize.x);
        int planeY = (int)std::roundf(pos.y / m_planeSize.y);
        const auto planeCode = (planeX + 32000) + (planeY + 32000) * 65535;
        if (m_planeCoordinatesSet.insert(planeCode))
        {
            EulerTransform entityTransform;
            entityTransform.T = Vector3(planeX * m_planeSize.x, planeY * m_planeSize.y, 0);

            auto entity = RefNew<RawEntity>();
            entity->m_name = StringID(TempString("Plane_{}_{}", planeX, planeY));

            entity->m_entityTemplate = RefNew<ObjectIndirectTemplate>();
            entity->m_entityTemplate->templateClass(SolidMeshEntity::GetStaticClass());
            entity->m_entityTemplate->placement(entityTransform);
            entity->m_entityTemplate->writeProperty<MeshAsyncRef>("mesh"_id, m_planeMesh.id());

            m_rootNodes.pushBack(entity);
            m_allNodes.pushBack(entity);
        }
    }
}

RawEntityPtr SceneBuilder::createCommonNode(ObjectIndirectTemplate* data, StringView customName)
{
    auto ret = RefNew<RawEntity>();
    ret->m_name = StringID(customName);
    
    if (data)
    {
        ret->m_entityTemplate = AddRef(data);
        data->parent(ret);
    }

    commonProcessNode(ret);

    return ret;
}

void SceneBuilder::commonProcessNode(RawEntity* entity)
{
    if (!entity->m_entityTemplate)
    {
        entity->m_entityTemplate = RefNew<ObjectIndirectTemplate>();
        entity->m_entityTemplate->templateClass(Entity::GetStaticClass());
    }

    entity->m_entityTemplate->placement(transform());

    if (m_flagTransformParent)
        entity->m_entityTemplate->writeProperty<bool>("attachToParentEntity"_id, true);

    if (!entity->m_name)
        entity->m_name = StringID(TempString("Node{}", m_nameCounter++));

    m_allNodes.pushBack(AddRef(entity));

    if (m_parentStack.empty())
    {
        m_rootNodes.pushBack(AddRef(entity));
    }
    else
    {
        auto parent = m_parentStack.back();
        parent->m_children.pushBack(AddRef(entity));
        entity->parent(parent);
    }
}

ResourceID SceneBuilder::mapResource(StringView path)
{
    ResourceID id;

    if (path)
    {
        if (!m_localResourceLookup.find(path, id))
        {
            if (GetService<DepotService>()->resolveIDForPath(path, id))
            {
                m_localResourceLookup[StringBuf(path)] = id;
            }
        }
    }

    return id;
}

void SceneBuilder::toggleTransformParent(bool flag)
{
    m_flagTransformParent = flag;
}

RawEntityPtr SceneBuilder::buildMeshNode(ResourceID id, StringView customName)
{
    auto data = RefNew<ObjectIndirectTemplate>();
    data->templateClass(SolidMeshEntity::GetStaticClass());
    data->writeProperty<MeshAsyncRef>("mesh"_id, id);

    return createCommonNode(data, customName);
}

RawEntityPtr SceneBuilder::buildPrefabNode(const PrefabPtr& prefab, StringView customName)
{
    auto ret = RefNew<RawEntity>();
    ret->m_name = StringID(customName);

    if (prefab)
    {
        m_capturedPrefabs.pushBackUnique(prefab);

        auto& info = ret->m_prefabAssets.emplaceBack();
        info.enabled = true;
        info.prefab = PrefabRef(ResourceID(), prefab);
    }

    commonProcessNode(ret);

    return ret;
}

//--

PrefabPtr SceneBuilder::extractPrefab() 
{
    auto ret = RefNew<Prefab>();

    auto root = RefNew<RawEntity>();
    root->m_name = "default"_id;
    root->m_children = m_rootNodes;

    for (const auto& ptr : m_rootNodes)
        ptr->parent(root);

    for (const auto& ptr : m_capturedPrefabs)
        ptr->parent(root);

    m_capturedPrefabs.clear();
    m_rootNodes.clear();
    m_allNodes.clear();

    ret->setup(root);

    return ret;
}

CompiledWorldPtr SceneBuilder::extractWorld()
{
    SourceEntitySoup soup;
    ExtractSourceEntities(m_rootNodes, soup);

    // build entity islands
    SourceIslands islands;
    ExtractSourceIslands(soup, islands);

    // build final islands
    Array<CompiledStreamingIslandPtr> finalIslands;
    finalIslands.reserve(islands.rootIslands.size());
    for (const auto& sourceRootIsland : islands.rootIslands)
    {
        if (auto finalIsland = BuildIsland(sourceRootIsland))
            finalIslands.pushBack(finalIsland);
    }

    // prepare compiled scene object
    Array<WorldParametersPtr> parameters;
    return RefNew<CompiledWorldData>(std::move(finalIslands), std::move(parameters));
}

//--

Point UlamSpiral(uint32_t n)
{
    auto k = std::ceilf((std::sqrtf(n) - 1) / 2);
    auto t = 2 * k + 1;
    auto m = t * t;

    t = t - 1;

    if (n >= m - t)
        return Point(k - (m - n), -k);
    else
        m = m - t;

    if (n >= m - t)
        return Point(-k, -k + (m - n));
    else
        m = m - t;

    if (n >= m - t)
        return Point(-k + (m - n), k);
    else
        return Point(k, k - (m - n - t));
}

//--

END_BOOMER_NAMESPACE_EX(test)
