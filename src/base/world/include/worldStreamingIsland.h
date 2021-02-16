/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\streaming #]
***/

#pragma once

#include "base/resource/include/resource.h"

namespace base
{
    namespace world
    {

        //---

        // single packed entity
        struct StreamingIslandPackedEntity
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(StreamingIslandPackedEntity);

        public:
            NodeGlobalID id = 0;
            EntityPtr data;
        };

        //---

        // Instance of streaming island - contains created and lined entities that can be attached to the world
        class BASE_WORLD_API StreamingIslandInstance : public IReferencable
        {
            RTTI_DECLARE_POOL(POOL_WORLD_STREAMING)

        public:
            StreamingIslandInstance(Array<StreamingIslandPackedEntity>&& ents);
            virtual ~StreamingIslandInstance();

            // TODO: named lookup ?
            // TODO: flag to hide island when children are loaded (mesh proxy)

            void attach(World* world); // tempshit, attaches all entities
            void detach(World* world); // tempshit, detaches all entities

        protected:
            Array<StreamingIslandPackedEntity> m_entites;
        };

        //---

        // group of packed island entities
        struct StreamingIslandPackedEntities : public IObject
        {
            RTTI_DECLARE_VIRTUAL_CLASS(StreamingIslandPackedEntities, IObject);

        public:
            Array<StreamingIslandPackedEntity> m_entities;
        };

        //---

        /// Streamable entity island (group of entities to load together) in the streaming sector
        /// NOTE: island must be attached/detached as a whole (for logic/gameplay/visual reasons)
        /// NOTE: child islands are always attached after parent islands and detached before parent island
        class BASE_WORLD_API StreamingIsland : public IObject
        {
            RTTI_DECLARE_POOL(POOL_WORLD_STREAMING)
            RTTI_DECLARE_VIRTUAL_CLASS(StreamingIsland, IObject);

        public:      
            struct Setup
            {
                Box streamingBox;
                Array<StreamingIslandPackedEntity> entities;
            };

            StreamingIsland();
            StreamingIsland(const Setup& setup);

            // streaming region
            INLINE const Box& streamingBounds() const { return m_streamingBox; }

            // number of entities in the island, never zero as empty islands can't exist
            INLINE uint32_t entityCount() const { return m_entityCount; }

            // child islands
            INLINE const Array<StreamingIslandPtr>& children() const { return m_children; }

            //--

            // attach child island
            void attachChild(StreamingIsland* child);

            //--
        
            // unpack sector data, load entities, etc, can take some time as it will load (most likely) other files on disk as well
            // NOTE: all files are loaded via specified loader to reuse/tracking
            // NOTE: all entities are created together and are linked using the links
            // NOTE: loaded entities are not yet linked to parent entities
            CAN_YIELD StreamingIslandInstancePtr load(res::ResourceLoader* loader) const;

            //--

        private:
            Box m_streamingBox; // computed from actual content locations + streaming distances

            uint32_t m_entityCount = 0; // stats only
            uint32_t m_entityUnpackedDataSize = 0; // decompressed data size
            Buffer m_entityPackedData; // compressed buffer with entity data for this island

            Array<StreamingIslandPtr> m_children; // child islands that can be loaded only after this one is
        };

        //--

    } // world
} // base