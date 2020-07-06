/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: resources #]
*
***/

#include "build.h"
#include "gameResourceMapping.h"

#include "base/resources/include/resourceFactory.h"

namespace game
{
    ///----

    // factory class for the resource mapping file
    class ResourceListFactory : public base::res::IFactory
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ResourceListFactory, base::res::IFactory);

    public:
        virtual base::res::ResourceHandle createResource() const override final
        {
            return base::CreateSharedPtr<ResourceList>();
        }
    };

    RTTI_BEGIN_TYPE_CLASS(ResourceListFactory);
        RTTI_METADATA(base::res::FactoryClassMetadata).bindResourceClass<ResourceList>();
    RTTI_END_TYPE();

    ///--

    RTTI_BEGIN_TYPE_STRUCT(ResourceEntry);
        RTTI_PROPERTY(name);
        RTTI_PROPERTY(resource);
    RTTI_END_TYPE();

    RTTI_BEGIN_TYPE_ENUM(ResourcePreloadMode);
        RTTI_ENUM_OPTION(None);
        RTTI_ENUM_OPTION(GameStart);
        RTTI_ENUM_OPTION(WorldStart);
    RTTI_END_TYPE();

    RTTI_BEGIN_TYPE_STRUCT(ResourceTable);
        RTTI_PROPERTY(name);
        RTTI_PROPERTY(preload);
        RTTI_PROPERTY(resources);
    RTTI_END_TYPE();

    ///--

	RTTI_BEGIN_TYPE_CLASS(ResourceList);
        RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4reslist");
        RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("Game Resources");
        RTTI_PROPERTY(m_tables);
    RTTI_END_TYPE();

    ResourceList::ResourceList()
    {}

    //--
    
} // game
