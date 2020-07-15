/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\data #]
*
***/

#include "build.h"
#include "worldLayer.h"
#include "worldDefinition.h"
#include "worldStreamingData.h"

#include "base/resource/include/resourceFactory.h"
#include "base/resource/include/resourceTags.h"

namespace game
{
    ///----

    // factory class for the scene world
    class SceneWorldFactory : public base::res::IFactory
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneWorldFactory, base::res::IFactory);

    public:
        virtual base::res::ResourceHandle createResource() const override final
        {
            return base::CreateSharedPtr<WorldDefinition>();
        }
    };

    RTTI_BEGIN_TYPE_CLASS(SceneWorldFactory);
        RTTI_METADATA(base::res::FactoryClassMetadata).bindResourceClass<WorldDefinition>();
    RTTI_END_TYPE();

    ///--

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IWorldParameters);
    RTTI_END_TYPE();

    IWorldParameters::IWorldParameters()
    {}

    IWorldParameters::~IWorldParameters()
    {}

    void IWorldParameters::renderDebug(bool edited, rendering::scene::FrameParams& info) const
    {}

    ///----

	RTTI_BEGIN_TYPE_CLASS(WorldDefinition);
        RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4world");
        RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("World");
        RTTI_PROPERTY(m_unorderedParameters);
        RTTI_PROPERTY(m_sectors);
    RTTI_END_TYPE();

    //--

    WorldDefinition::WorldDefinition()
    {
    }

    WorldDefinition::~WorldDefinition()
    {
    }

    void WorldDefinition::createParametersMap()
    {
        // enumerate all parameter classes that we have
        base::InplaceArray<base::SpecificClassType<IWorldParameters>, 32> allParamClasses;
        RTTI::GetInstance().enumClasses(allParamClasses);

        // index the classes
        static short NumParamClasses = 0;
        if (0 == NumParamClasses)
        {
            for (const auto paramClass : allParamClasses)
                paramClass->assignUserIndex(NumParamClasses++);
        }

        // create missing entries in the param table
        for (auto paramClass : allParamClasses)
        {
            if (!findParameters(paramClass))
            {
                if (auto paramObject = paramClass->create<IWorldParameters>())
                {
                    m_unorderedParameters.pushBack(paramObject);
                    paramObject->parent(this);
                }
            }
        }

        // index in table
        m_parametersTable.clear();
        m_parametersTable.resizeWith(NumParamClasses, nullptr);
        for (uint32_t i = 0; i < allParamClasses.size(); ++i)
        {
            if (auto paramObject = findParameters(allParamClasses[i]))
                m_parametersTable[paramObject->cls()->userIndex()] = paramObject;
        }
    }

    void WorldDefinition::onPostLoad()
    {
        TBaseClass::onPostLoad();

        m_unorderedParameters.remove(nullptr);
        createParametersMap();
    }

    //--

    IWorldParameters* WorldDefinition::findParameters(base::ClassType paramClass) const
    {
        for (auto& ptr : m_unorderedParameters)
            if (ptr->is(paramClass))
                return ptr;

        return nullptr;
    }

    //--
    
} // game
