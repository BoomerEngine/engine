/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\data #]
*
***/

#include "build.h"
#include "worldStreamingData.h"
#include "base/resource/include/resourceTags.h"

namespace game
{
    ///---

    WorldSectorUnpackedData::WorldSectorUnpackedData()
    {}

    WorldSectorUnpackedData::~WorldSectorUnpackedData()
    {}

    ///---

    RTTI_BEGIN_TYPE_CLASS(WorldSectorData);
        RTTI_PROPERTY(m_gridLevel);
        RTTI_PROPERTY(m_gridX);
        RTTI_PROPERTY(m_gridY);
        RTTI_PROPERTY(m_worldBox);
        RTTI_PROPERTY(m_streamingBox);
        RTTI_PROPERTY(m_entityCount);
        RTTI_PROPERTY(m_entityData);
    RTTI_END_TYPE();

    WorldSectorData::WorldSectorData()
    {
    }

    WorldSectorData::WorldSectorData(uint8_t level, int x, int y, const base::Box& worldBox, const base::Box& streamingBox, uint32_t entityCount, base::Buffer entityData)
        : m_gridLevel(level)
        , m_gridX(x)
        , m_gridY(y)
        , m_worldBox(worldBox)
        , m_streamingBox(streamingBox)
        , m_entityCount(entityCount)
        , m_entityData(entityData)
    {}

    WorldSectorUnpackedDataPtr WorldSectorData::unpack(base::res::IResourceLoader* loader) const
    {
        return nullptr;
    }

    ///---

    RTTI_BEGIN_TYPE_CLASS(WorldSectorGridLevel);
        RTTI_PROPERTY(m_gridLevel);
        RTTI_PROPERTY(m_sectorsPerSide);
        RTTI_PROPERTY(m_sectorSize);
        RTTI_PROPERTY(m_sectors);
    RTTI_END_TYPE();

    WorldSectorGridLevel::WorldSectorGridLevel()
    {}

    WorldSectorGridLevel::WorldSectorGridLevel(uint8_t level, uint32_t sectorPerSize, float sectorSize, base::Array<WorldSectorDataPtr>&& sectors)
        : m_gridLevel(level)
        , m_sectorsPerSide(sectorPerSize)
        , m_sectorSize(sectorSize)
        , m_sectors(std::move(sectors))
    {
    }

    WorldSectorGridLevel::~WorldSectorGridLevel()
    {}

    const WorldSectorData* WorldSectorGridLevel::sector(int x, int y) const
    {
        x += m_sectorsPerSide / 2;
        y += m_sectorsPerSide / 2;
        if (x >= 0 && y >= 0 && x < m_sectorsPerSide && y < m_sectorsPerSide)
            return m_sectors[x + y * m_sectorsPerSide];

        return nullptr;
    }

    ///---

    RTTI_BEGIN_TYPE_CLASS(WorldSectors);
        RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4sec");
        RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("World Streaming");
        RTTI_METADATA(base::res::ResourceTagColorMetadata).color(0x99, 0x22, 0x44);
        RTTI_PROPERTY(m_worldSize);
        RTTI_PROPERTY(m_levels);
    RTTI_END_TYPE();

    WorldSectors::WorldSectors()
    {}

    WorldSectors::WorldSectors(float worldSize, base::Array<WorldSectorGridLevelPtr>&& levels)
        : m_worldSize(worldSize)
        , m_levels(std::move(levels))
    {}

    WorldSectors::~WorldSectors()
    {}

    ///---

} // game


