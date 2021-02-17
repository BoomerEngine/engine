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
#include "worldStreamingIsland.h"
#include "worldEntity.h"

namespace base
{
    namespace world
    {
        ///---

        StreamingIslandInstance::StreamingIslandInstance(Array<StreamingIslandPackedEntity>&& ents)
            : m_entites(std::move(ents))
        {
            for (auto& ent : m_entites)
                if (ent.data)
                    ent.data->parent(nullptr);
        }

        StreamingIslandInstance::~StreamingIslandInstance()
        {}

        void StreamingIslandInstance::attach(World* world)
        {
            {
                PC_SCOPE_LVL1(AttachStreamedIn);
                for (const auto& ent : m_entites)
                    if (ent.data)
                        world->attachEntity(ent.data);
            }

            {
                PC_SCOPE_LVL1(OnStreamIn);
                for (const auto& ent : m_entites)
                    if (ent.data)
                        ent.data->handleStreamIn(this);
            }
        }

        void StreamingIslandInstance::detach(World* world)
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

        RTTI_BEGIN_TYPE_STRUCT(StreamingIslandPackedEntity);
            RTTI_PROPERTY(id);
            RTTI_PROPERTY(data);
        RTTI_END_TYPE();

        RTTI_BEGIN_TYPE_CLASS(StreamingIslandPackedEntities);
            RTTI_PROPERTY(m_entities);
        RTTI_END_TYPE();

        ///---

        RTTI_BEGIN_TYPE_CLASS(StreamingIsland);
            RTTI_PROPERTY(m_alwaysLoaded);
            RTTI_PROPERTY(m_streamingBox);
            RTTI_PROPERTY(m_entityCount);
            RTTI_PROPERTY(m_entityUnpackedDataSize);
            RTTI_PROPERTY(m_entityPackedData);
            RTTI_PROPERTY(m_children);
        RTTI_END_TYPE();

        StreamingIsland::StreamingIsland()
        {}

        StreamingIsland::StreamingIsland(const Setup& setup)
        {
            m_streamingBox = setup.streamingBox;
            m_alwaysLoaded = setup.alwaysLoaded;
            m_entityCount = setup.entities.size();

            auto container = RefNew<StreamingIslandPackedEntities>();
            container->m_entities.reserve(setup.entities.size());

            HashSet<EntityPtr> uniqueEntities;
            for (const auto& ent : setup.entities)
            {
                if (ent.data)
                {
                    DEBUG_CHECK(!uniqueEntities.contains(ent.data));
                    uniqueEntities.insert(ent.data);

                    auto& info = container->m_entities.emplaceBack();
                    info.id = ent.id;
                    info.data = ent.data;
                    info.data->parent(container);
                }
            }
            
            const auto data = container->toBuffer();
            m_entityUnpackedDataSize = data.size();
            m_entityPackedData = Compress(CompressionType::LZ4HC, data, POOL_WORLD_STREAMING);
        }

        void StreamingIsland::attachChild(StreamingIsland* child)
        {
            if (child)
            {
                child->parent(this);
                m_children.pushBack(AddRef(child));
            }
        }

        StreamingIslandInstancePtr StreamingIsland::load(res::ResourceLoader* loader) const
        {
            PC_SCOPE_LVL1(LoadStreamingIsland);

            // decompress buffer
            auto data = Decompress(CompressionType::LZ4HC, m_entityPackedData.data(), m_entityPackedData.size(), m_entityUnpackedDataSize, POOL_WORLD_STREAMING);
            DEBUG_CHECK_RETURN_EX_V(data, "Unable to decompress entity data", nullptr);

            // unpack the objects
            auto objects = rtti_cast<StreamingIslandPackedEntities>(LoadObjectFromBuffer(data.data(), data.size(), loader));
            DEBUG_CHECK_RETURN_EX_V(objects, "Unable to load packed entities", nullptr);

            // create runtime island
            return RefNew<StreamingIslandInstance>(std::move(objects->m_entities));
        }

        ///---

    } // world
} // game


