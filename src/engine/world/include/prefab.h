/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\resources #]
***/

#pragma once

#include "core/resource/include/resource.h"

#include "nodeTemplate.h"

BEGIN_BOOMER_NAMESPACE()

//----

/// compilation settings for prefab
struct PrefabCompilationSettings
{
    uint32_t seed = 0; // must be given, keep at 0 if don't care
    StringID appearance; // selected appearance for conditional nodes
};

//----

/// prefab compilation dependency
struct PrefabDependencies
{
    struct Entry
    {
        PrefabWeakPtr m_sourcePrefab;
        uint32_t m_dataVersion = 0;
    };

    Array<Entry> m_entries;
};

//----

/// prefab appearance
struct PrefabAppearance
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(PrefabAppearance);

public:
    StringID name;
    float randomWeight = 0.0f;

    PrefabAppearance();
};

//----

/// scene prefab, self contain data set from which a self contained node group may be spawned
class ENGINE_WORLD_API Prefab : public IResource
{
    RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS)
    RTTI_DECLARE_VIRTUAL_CLASS(Prefab, IResource);

public:
    Prefab(uint64_t seed=0);

    INLINE const Array<PrefabAppearance>& appearances() const { return m_appearances; }
    INLINE const NodeTemplatePtr& root() const { return m_root; }

    void setup(NodeTemplate* root);

    EntityPtr compile(StringID appearance, const AbsoluteTransform& placement, Array<EntityPtr>& outAllEntities) const;

private:
    NodeTemplatePtr m_root;
    Array<PrefabAppearance> m_appearances;

    uint64_t m_internalSeed = 0;

    virtual void onPostLoad() override;
};

//----

END_BOOMER_NAMESPACE()
