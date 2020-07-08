/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\data #]
***/

#pragma once

#include "base/resource/include/resource.h"

namespace game
{
    //---

    /// "unpacked" entity data
    class GAME_WORLD_API WorldSectorUnpackedData : public base::IReferencable
    {
    public:
        WorldSectorUnpackedData();
        virtual ~WorldSectorUnpackedData();

        base::Array<EntityPtr> m_entities;
    };

    //---

    /// sector in the streaming grid
    class GAME_WORLD_API WorldSectorData : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(WorldSectorData, base::IObject);

    public:
        WorldSectorData();
        WorldSectorData(uint8_t level, int x, int y, const base::Box& worldBox, const base::Box& streamingBox, uint32_t entityCount, base::Buffer entityData);

        // grid params
        INLINE uint8_t gridLevel() const { return m_gridLevel; }
        INLINE int gridX() const { return m_gridX; }
        INLINE int gridY() const { return m_gridY; }

        // world bounds of the sector itself
        INLINE const base::Box& worldBounds() const { return m_worldBox; }

        // streaming region
        INLINE const base::Box& streamingBounds() const { return m_streamingBox; }

        // number of entities in the sector, never zero
        INLINE uint32_t entityCount() const { return m_entityCount; }

        // compressed data of the entities
        INLINE const base::Buffer& entityData() const { return m_entityData; }

        //--
        
        // unpack sector data, load entities, etc, can take some time as it will load (most likely) other files on disk as well
        CAN_YIELD WorldSectorUnpackedDataPtr unpack(base::res::IResourceLoader* loader) const;

        //--

    private:
        uint8_t m_gridLevel = 0;
        int m_gridX = 0;
        int m_gridY = 0;

        base::Box m_worldBox; // computed from grid
        base::Box m_streamingBox; // computed from actual content locations + streaming distances

        uint32_t m_entityCount = 0; // stats only
        base::Buffer m_entityData; // compressed
    };

    //--

    /// level in the world streaming grid
    class GAME_WORLD_API WorldSectorGridLevel : public base::IObject
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(WorldSectorGridLevel);

    public:
        WorldSectorGridLevel();
        WorldSectorGridLevel(uint8_t level, uint32_t sectorPerSize, float sectorSize, base::Array<WorldSectorDataPtr>&& sectors);
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

        base::Array<WorldSectorDataPtr> m_sectors; // data for each sector, can be NULL
    };

    //--

    /// Baked world sectors
    class GAME_WORLD_API WorldSectors : public base::res::IResource
    {
        RTTI_DECLARE_VIRTUAL_CLASS(WorldSectors, base::res::IResource);

    public:
        WorldSectors();
        WorldSectors(float worldSize, base::Array<WorldSectorGridLevelPtr>&& levels);
        virtual ~WorldSectors();

        INLINE float worldSize() const { return m_worldSize; }

        INLINE const base::Array<WorldSectorGridLevelPtr>& levels() const { return m_levels; }

    private:
        float m_worldSize = 0.0f; // size of the world  
        base::Array<WorldSectorGridLevelPtr> m_levels;
    };

    //--

} // game