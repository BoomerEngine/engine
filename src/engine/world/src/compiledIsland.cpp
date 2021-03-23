/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\streaming #]
*
***/

#include "build.h"
#include "world.h"
#include "worldEntity.h"

#include "compiledIsland.h"

BEGIN_BOOMER_NAMESPACE()

///---

CompiledStreamingIslandInstance::CompiledStreamingIslandInstance(Array<CompiledStreamingIslandPackedEntity>&& ents)
    : m_entites(std::move(ents))
{
    for (auto& ent : m_entites)
        if (ent.data)
            ent.data->parent(nullptr);
}

CompiledStreamingIslandInstance::~CompiledStreamingIslandInstance()
{}

void CompiledStreamingIslandInstance::attach(World* world)
{
    {
        PC_SCOPE_LVL1(AttachStreamedIn);
        for (const auto& ent : m_entites)
        {
            if (ent.data)
            {
                EntityAttachmentSetup setup;
                setup.ensureCustomLocalToWorld = true;
                setup.customLocalToWorld = ent.worldPlacement;
                setup.staticID = ent.id;
                setup.name = ent.name;

                if (ent.parentTransform != INDEX_NONE)
                    setup.transformParent = m_entites[ent.parentTransform].data;

                world->attachEntity(ent.data, setup); // TODO: hierarchy
            }
        }
    }

    {
        PC_SCOPE_LVL1(OnStreamIn);
        for (const auto& ent : m_entites)
            if (ent.data)
                ent.data->handleStreamIn(this);
    }
}

void CompiledStreamingIslandInstance::detach(World* world)
{
    {
        PC_SCOPE_LVL1(OnStreamOut);
        for (const auto& ent : m_entites)
            if (ent.data)
                ent.data->handleStreamOut(this);
    }

    {
        PC_SCOPE_LVL1(DetachStreamedOut);
        for (const auto& ent : m_entites)
            if (ent.data)
                world->detachEntity(ent.data);
    }
}

//---

RTTI_BEGIN_TYPE_STRUCT(CompiledStreamingIslandPackedEntity);
    RTTI_PROPERTY(id);
    RTTI_PROPERTY(name);
    RTTI_PROPERTY(data);
    RTTI_PROPERTY(worldPlacement);
    RTTI_PROPERTY(parentTransform);
RTTI_END_TYPE();

RTTI_BEGIN_TYPE_CLASS(CompiledStreamingIslandPackedEntities);
    RTTI_PROPERTY(m_entities);
RTTI_END_TYPE();

///---

RTTI_BEGIN_TYPE_CLASS(CompiledStreamingIsland);
    RTTI_PROPERTY(m_alwaysLoaded);
    RTTI_PROPERTY(m_streamingBox);
    RTTI_PROPERTY(m_entityCount);
    RTTI_PROPERTY(m_entityPackedData);
    RTTI_PROPERTY(m_children);
RTTI_END_TYPE();

CompiledStreamingIsland::CompiledStreamingIsland()
{}

CompiledStreamingIsland::CompiledStreamingIsland(const Setup& setup)
{
    m_streamingBox = setup.streamingBox;
    m_alwaysLoaded = setup.alwaysLoaded;
    m_entityCount = setup.entities.size();

    auto container = RefNew<CompiledStreamingIslandPackedEntities>();
    container->m_entities.reserve(setup.entities.size());

    for (auto index : setup.entities.indexRange())
    {
        const auto& ent = setup.entities[index];

        auto& info = container->m_entities.emplaceBack(ent);
        DEBUG_CHECK(info.parentTransform < index);

        info.data->parent(container);
    }
            
    const auto data = container->toBuffer();
    m_entityPackedData.bind(data, CompressionType::LZ4HC);
}

void CompiledStreamingIsland::attachChild(CompiledStreamingIsland* child)
{
    if (child)
    {
        child->parent(this);
        m_children.pushBack(AddRef(child));
    }
}

CompiledStreamingIslandInstancePtr CompiledStreamingIsland::load(bool loadImports) const
{
    PC_SCOPE_LVL1(LoadCompiledStreamingIsland);

    // decompress buffer
    auto data = m_entityPackedData.decompress(POOL_WORLD_STREAMING);
    DEBUG_CHECK_RETURN_EX_V(data, "Unable to decompress entity data", nullptr);

    // unpack the objects
    auto objects = rtti_cast<CompiledStreamingIslandPackedEntities>(LoadObjectFromBuffer(data.data(), data.size(), loadImports));
    DEBUG_CHECK_RETURN_EX_V(objects, "Unable to load packed entities", nullptr);

    // create runtime island
    return RefNew<CompiledStreamingIslandInstance>(std::move(objects->m_entities));
}

///---

END_BOOMER_NAMESPACE()
