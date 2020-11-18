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

        /// "unpacked" entity data
        class BASE_WORLD_API WorldSectorUnpackedData : public IReferencable
        {
        public:
            WorldSectorUnpackedData();
            virtual ~WorldSectorUnpackedData();

            Array<EntityPtr> m_entities;
        };

        //---

        /// sector in the streaming grid
        class BASE_WORLD_API WorldSectorData : public IObject
        {
            RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS)
            RTTI_DECLARE_VIRTUAL_CLASS(WorldSectorData, IObject);

        public:
            WorldSectorData();
            WorldSectorData(uint8_t level, int x, int y, const Box& worldBox, const Box& streamingBox, uint32_t entityCount, Buffer entityData);

            // grid params
            INLINE uint8_t gridLevel() const { return m_gridLevel; }
            INLINE int gridX() const { return m_gridX; }
            INLINE int gridY() const { return m_gridY; }

            // world bounds of the sector itself
            INLINE const Box& worldBounds() const { return m_worldBox; }

            // streaming region
            INLINE const Box& streamingBounds() const { return m_streamingBox; }

            // number of entities in the sector, never zero
            INLINE uint32_t entityCount() const { return m_entityCount; }

            // compressed data of the entities
            INLINE const Buffer& entityData() const { return m_entityData; }

            //--
        
            // unpack sector data, load entities, etc, can take some time as it will load (most likely) other files on disk as well
            CAN_YIELD WorldSectorUnpackedDataPtr unpack(res::IResourceLoader* loader) const;

            //--

        private:
            uint8_t m_gridLevel = 0;
            int m_gridX = 0;
            int m_gridY = 0;

            Box m_worldBox; // computed from grid
            Box m_streamingBox; // computed from actual content locations + streaming distances

            uint32_t m_entityCount = 0; // stats only
            Buffer m_entityData; // compressed
        };

        //--

        /// level in the world streaming grid
        class BASE_WORLD_API WorldSectorGridLevel : public IObject
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(WorldSectorGridLevel);

        public:
            WorldSectorGridLevel();
            WorldSectorGridLevel(uint8_t level, uint32_t sectorPerSize, float sectorSize, Array<WorldSectorDataPtr>&& sectors);
            virtual ~WorldSectorGridLevel();

            /// level in the grid
            INLINE uint8_t gridLevel() const { return m_gridLevel; }

            /// number of sectors per side of the grid
            INLINE uint32_t sectorsPerSide() const { return m_sectorsPerSide; }

            /// size of the sector in world units
            INLINE float sectorSize() const { return m_sectorSize; }

            //--

            /// get sector data for given grid coordinates in the level
            const WorldSectorData* sector(int x, int y) const;

        private:
            uint8_t m_gridLevel = 0;
            uint32_t m_sectorsPerSide = 0; // usually 1 << m_gridLevel
            float m_sectorSize; // size of sector (in world units)

            Array<WorldSectorDataPtr> m_sectors; // data for each sector, can be NULL
        };

        //--

        /// Baked world sectors
        class BASE_WORLD_API WorldSectors : public res::IResource
        {
            RTTI_DECLARE_VIRTUAL_CLASS(WorldSectors, res::IResource);

        public:
            WorldSectors();
            WorldSectors(float worldSize, Array<WorldSectorGridLevelPtr>&& levels);
            virtual ~WorldSectors();

            INLINE float worldSize() const { return m_worldSize; }

            INLINE const Array<WorldSectorGridLevelPtr>& levels() const { return m_levels; }

        private:
            float m_worldSize = 0.0f; // size of the world  
            Array<WorldSectorGridLevelPtr> m_levels;
        };

        //--

    } // world
} // base