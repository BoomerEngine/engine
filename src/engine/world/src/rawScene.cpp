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
#include "core/resource/include/factory.h"
#include "core/resource/include/tags.h"

BEGIN_BOOMER_NAMESPACE()

///----

// factory class for the scene template
class SceneMainFileFactory : public IResourceFactory
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneMainFileFactory, IResourceFactory);

public:
    virtual ResourcePtr createResource() const override final
    {
        return RefNew<RawScene>();
    }
};

RTTI_BEGIN_TYPE_CLASS(SceneMainFileFactory);
    RTTI_METADATA(ResourceFactoryClassMetadata).bindResourceClass<RawScene>();
RTTI_END_TYPE();

///----

RTTI_BEGIN_TYPE_CLASS(RawScene);
    RTTI_METADATA(ResourceDescriptionMetadata).description("Scene");
    //RTTI_PROPERTY(m_allLayers)
RTTI_END_TYPE();

RawScene::RawScene()
{
}

///----

END_BOOMER_NAMESPACE()
