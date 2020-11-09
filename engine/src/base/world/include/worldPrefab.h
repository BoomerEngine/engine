/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\resources #]
***/

#pragma once

#include "base/resource/include/resource.h"

#include "worldNodeTemplate.h"

namespace base
{
    namespace world
    {

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

        /// scene prefab, self contain data set from which a self contained node group may be spawned
        class BASE_WORLD_API Prefab : public res::IResource
        {
            RTTI_DECLARE_VIRTUAL_CLASS(Prefab, res::IResource);

        public:
            Prefab();

            INLINE const Array<NodeTemplatePtr>& nodes() const { return m_nodes; }

            void setup(const Array<NodeTemplatePtr>& nodes); // reparents the nodes

        private:
            Array<NodeTemplatePtr> m_nodes;
        };

        //----

    } // world
} // base