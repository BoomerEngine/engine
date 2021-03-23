/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\resources #]
*
***/

#include "build.h"

#include "world.h"
#include "worldParameters.h"

#include "rawWorldData.h"

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
        return RefNew<RawWorldData>();
    }
};

RTTI_BEGIN_TYPE_CLASS(SceneMainFileFactory);
    RTTI_METADATA(ResourceFactoryClassMetadata).bindResourceClass<RawWorldData>();
RTTI_END_TYPE();

///----

RTTI_BEGIN_TYPE_CLASS(RawWorldData);
    RTTI_METADATA(ResourceDescriptionMetadata).description("World");
    RTTI_PROPERTY(m_parametres);
RTTI_END_TYPE();

RawWorldData::RawWorldData()
{
    if (!IsDefaultObjectCreation())
        createParameters();
}

Array<WorldParametersPtr> RawWorldData::compileWorldParameters() const
{
    return m_parametres;
}

void RawWorldData::createParameters()
{
    m_parametres.removeAll(nullptr);

    InplaceArray<SpecificClassType<IWorldParameters>, 10> parameterClasses;
    RTTI::GetInstance().enumClasses(parameterClasses);

    for (const auto cls : parameterClasses)
    {
        bool exists = false;
        for (const auto& param : m_parametres)
        {
            if (param->cls() == cls)
            {
                exists = true;
                break;
            }
        }

        if (!exists)
        {
            auto param = cls->create<IWorldParameters>();
            param->parent(this);
            m_parametres.pushBack(param);
        }
    }


}

void RawWorldData::onPostLoad()
{
    TBaseClass::onPostLoad();
    createParameters();
}

///----

WorldPtr World::CreateEditorWorld(const RawWorldData* editableWorld)
{
    DEBUG_CHECK_RETURN_EX_V(editableWorld, "No editable world", nullptr);
    return RefNew<World>(WorldType::EditorPreview, editableWorld, nullptr);
}

///----

END_BOOMER_NAMESPACE()
