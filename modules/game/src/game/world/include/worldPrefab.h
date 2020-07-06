/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\resources #]
***/

#pragma once

#include "base/resources/include/resource.h"

#include "worldNodeTemplate.h"
#include "worldNodeContainer.h"

namespace game
{
    //----

    /// compilation settings for prefab
    struct PrefabCompilationSettings
    {
        uint32_t seed = 0; // must be given, keep at 0 if don't care
        base::StringID appearance; // selected appearance for conditional nodes
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

        base::Array<Entry> m_entries;
    };

    //----

    /// scene prefab, self contain data set from which a self contained node group may be spawned
    class GAME_WORLD_API Prefab : public base::res::IResource
    {
        RTTI_DECLARE_VIRTUAL_CLASS(Prefab, base::res::IResource);

    public:
        Prefab();
        Prefab(const NodeTemplateContainerPtr& existingContainer); // NOTE: prefab will take ownership of the container (it is going to be reparented)
        virtual ~Prefab();

        //---

        /// get content of the prefab - that is a node container with nodes ;)
        INLINE NodeTemplateContainerPtr content() const { return m_container; }

        //--

        // compile this prefab, generates new data container with flattened nodes
        NodeTemplateCompiledOutputPtr compile(const PrefabCompilationSettings& settings = PrefabCompilationSettings()) const;

        //--

        // compile entity to spawn at given location
        EntityPtr createEntities(base::Array<EntityPtr>& outAllEntities, const base::AbsoluteTransform& placement, const PrefabCompilationSettings& settings = PrefabCompilationSettings()) const;

        //--

    protected:
        NodeTemplateContainerPtr m_container;
    };

    //----

} // game