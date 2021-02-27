/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\resources #]
*
***/

#include "build.h"
#include "rawScene.h"
#include "core/resource/include/resourceFactory.h"
#include "core/resource/include/resourceTags.h"

BEGIN_BOOMER_NAMESPACE()

///----

// factory class for the scene template
class SceneMainFileFactory : public res::IFactory
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneMainFileFactory, res::IFactory);

public:
    virtual res::ResourceHandle createResource() const override final
    {
        return RefNew<RawScene>();
    }
};

RTTI_BEGIN_TYPE_CLASS(SceneMainFileFactory);
    RTTI_METADATA(res::FactoryClassMetadata).bindResourceClass<RawScene>();
RTTI_END_TYPE();

///----

RTTI_BEGIN_TYPE_CLASS(RawScene);
    RTTI_METADATA(res::ResourceExtensionMetadata).extension("v4scene");
    RTTI_METADATA(res::ResourceDescriptionMetadata).description("Scene");
    RTTI_METADATA(res::ResourceTagColorMetadata).color(0x9d, 0x02, 0x08);
    //RTTI_PROPERTY(m_allLayers)
RTTI_END_TYPE();

RawScene::RawScene()
{
}

///----

END_BOOMER_NAMESPACE()
