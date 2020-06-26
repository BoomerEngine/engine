/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
*
***/

#include "build.h"
#include "sceneWorldParameters.h"
#include "base/containers/include/inplaceArray.h"

namespace scene
{

    ///--

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IWorldParameters);
    RTTI_END_TYPE();

    IWorldParameters::IWorldParameters()
    {}

    IWorldParameters::~IWorldParameters()
    {}

    void IWorldParameters::renderDebug(rendering::scene::FrameInfo& info) const
    {}

    ///---

    RTTI_BEGIN_TYPE_CLASS(WorldParameterContainer);
        RTTI_PROPERTY(m_parameters);
    RTTI_END_TYPE();

    WorldParameterContainer::WorldParameterContainer()
    {
        if (!base::rtti::IClassType::IsDefaultObjectCreation())
        {
            createParameters();
        }
    }

    WorldParameterContainer::~WorldParameterContainer()
    {}

    const base::RefPtr<IWorldParameters> WorldParameterContainer::findParameters(base::ClassType paramClass) const
    {
        for (auto& ptr : m_parameters)
            if (ptr->is(paramClass))
                return ptr;

        return nullptr;
    }

    void WorldParameterContainer::createParameters()
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
        for (auto paramClass  : allParamClasses)
        {
            if (!findParameters(paramClass))
            {
                if (auto paramObject = paramClass->createSharedPtr<IWorldParameters>())
                {
                    m_parameters.pushBack(paramObject);
                    paramObject->parent(sharedFromThis());
                }
            }
        }

        // index in table
        m_parametersTable.clear();
        m_parametersTable.resizeWith(NumParamClasses, nullptr);
        for (uint32_t i=0; i<allParamClasses.size(); ++i)
        {
            if (auto paramObject = findParameters(allParamClasses[i]))
                m_parametersTable[paramObject->cls()->userIndex()] = paramObject.get();
        }
    }

    void WorldParameterContainer::onPostLoad()
    {
        TBaseClass::onPostLoad();
        m_parameters.remove(nullptr);
        createParameters();
    }

    ///---

} // scene


