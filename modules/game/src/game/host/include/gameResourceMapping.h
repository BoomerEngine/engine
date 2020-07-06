/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: resources #]
***/

#pragma once

#include "base/containers/include/hashMap.h"

namespace game
{
    //---

    /// resource entry
    struct GAME_HOST_API ResourceEntry
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(ResourceEntry);

        base::StringID name;
        base::res::AsyncRef<base::res::IResource> resource;
    };

    //---

    /// resource preload mode
    enum class ResourcePreloadMode : uint8_t
    {
        None, // do not preload resources
        GameStart, // preload resources on game start (game spawn will take longer)
        WorldStart, // preload resource on world start
    };

    /// table of resources
    struct GAME_HOST_API ResourceTable
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(ResourceTable);

        base::StringID name;
        ResourcePreloadMode preload;
        base::Array<ResourceEntry> resources;
    };

    //---

    /// file with resource definitions
    class GAME_HOST_API ResourceList : public base::res::ITextResource
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ResourceList, base::res::ITextResource);

    public:
        ResourceList();

        //--

        INLINE const base::Array<ResourceTable>& tables() const { return m_tables; }

        //--

    public:
        base::Array<ResourceTable> m_tables;
    };

    //---

} // game