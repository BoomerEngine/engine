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
#include "worldNodePlacement.h"

namespace game
{
    //----

    /// compilation settings for prefab
    struct PrefabCompilationSettings
    {
        uint32_t m_seed = 0; // must be given, keep at 0 if don't care
        base::StringID m_appearance; // selected appearance for conditional nodes
        base::AbsoluteTransform m_placement; // prefab placement
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
    class GAME_SCENE_API Prefab : public base::res::IResource
    {
        RTTI_DECLARE_VIRTUAL_CLASS(Prefab, base::res::IResource);

    public:
        Prefab();
        virtual ~Prefab();

        //---

        /// get reference data for the prefab, this is the last computed "snapshot" of the prefab
        /// NOTE: this is not flattened (instanced) snapshot but it contains the up to date version number
        NodeTemplateContainerPtr nodeContainer(uint32_t* outDataVersion=nullptr) const;

        /// check if given cached prefab version is up to date
        bool checkVersion(uint32_t version) const;

        //--

        // compile this prefab, generates new data container with flattened nodes
        NodeTemplateContainerPtr compile(const PrefabCompilationSettings& settings, PrefabDependencies* outDependencies = nullptr) const;

        //--

        // set new content for prefab, will re-instance all nodes that use this prefab
        void content(const NodeTemplateContainerPtr& newContent);

    protected:
        // edited data, used by tools when editing prefab, safe to access directly since there's only one editor per prefab (UI ensures of that)
        base::SpinLock m_containerLock;
        NodeTemplateContainerPtr m_container;

        // copy of the editor data kept up to date as often as possible, "snapshoted" when prefab changes, never modified after creation
        // NOTE: the snapshot is done on the main thread whenever prefab changes (this is a shitty deal ATM)
        volatile uint32_t m_dataVersion;

        virtual void onPostLoad() override;

        //---

        base::Array<NodeTemplatePtr> m_nodes; // OLD

        friend class PrefabCompiler;
    };

    //----

} // game