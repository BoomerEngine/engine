/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\streaming #]
***/

#pragma once

#include "core/resource/include/resource.h"
#include "core/object/include/compressedBuffer.h"

BEGIN_BOOMER_NAMESPACE()

//---

// single packed entity
struct CompiledStreamingIslandPackedEntity
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(CompiledStreamingIslandPackedEntity);

public:
    StringID name;
    EntityStaticID id = 0;
    Transform worldPlacement;
    int parentTransform = -1;
    EntityPtr data;
};

//---

// Instance of streaming island - contains created and lined entities that can be attached to the world
class ENGINE_WORLD_API CompiledStreamingIslandInstance : public IReferencable
{
    RTTI_DECLARE_POOL(POOL_WORLD_STREAMING)

public:
    CompiledStreamingIslandInstance(Array<CompiledStreamingIslandPackedEntity>&& ents);
    virtual ~CompiledStreamingIslandInstance();

    INLINE const uint32_t size() const { return m_entites.size(); }

    // TODO: named lookup ?
    // TODO: flag to hide island when children are loaded (mesh proxy)

    void attach(World* world); // tempshit, attaches all entities
    void detach(World* world); // tempshit, detaches all entities

protected:
    Array<CompiledStreamingIslandPackedEntity> m_entites;
};

//---

// group of packed island entities
struct CompiledStreamingIslandPackedEntities : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(CompiledStreamingIslandPackedEntities, IObject);

public:
    Array<CompiledStreamingIslandPackedEntity> m_entities;
};

//---

/// Streamable entity island (group of entities to load together) in the streaming sector
/// NOTE: island must be attached/detached as a whole (for logic/gameplay/visual reasons)
/// NOTE: child islands are always attached after parent islands and detached before parent island
class ENGINE_WORLD_API CompiledStreamingIsland : public IObject
{
    RTTI_DECLARE_POOL(POOL_WORLD_STREAMING)
    RTTI_DECLARE_VIRTUAL_CLASS(CompiledStreamingIsland, IObject);

public:      
    struct Setup
    {
        Box streamingBox;
        bool alwaysLoaded = false;
        Array<CompiledStreamingIslandPackedEntity> entities;
    };

    CompiledStreamingIsland();
    CompiledStreamingIsland(const Setup& setup);

    // is this always loaded island ?
    INLINE bool alwaysLoaded() const { return m_alwaysLoaded; }

    // streaming region
    INLINE const Box& streamingBounds() const { return m_streamingBox; }

    // number of entities in the island, never zero as empty islands can't exist
    INLINE uint32_t entityCount() const { return m_entityCount; }

    // child islands
    INLINE const Array<CompiledStreamingIslandPtr>& children() const { return m_children; }

    //--

    // attach child island
    void attachChild(CompiledStreamingIsland* child);

    //--
        
    // unpack sector data, load entities, etc, can take some time as it will load (most likely) other files on disk as well
    // NOTE: all files are loaded via specified loader to reuse/tracking
    // NOTE: all entities are created together and are linked using the links
    // NOTE: loaded entities are not yet linked to parent entities
    CAN_YIELD CompiledStreamingIslandInstancePtr load(bool loadImports=true) const;

    //--

    // TODO: embedded resources 

private:
    Box m_streamingBox; // computed from actual content locations + streaming distances

    bool m_alwaysLoaded = false; // this island should always be loaded (NOTE: still requires parent to be loaded)

    uint32_t m_entityCount = 0; // stats only
    CompressedBufer m_entityPackedData; // compressed buffer with entity data for this island

    Array<CompiledStreamingIslandPtr> m_children; // child islands that can be loaded only after this one is
};

//--

END_BOOMER_NAMESPACE()
