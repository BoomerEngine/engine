/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource #]
***/

#include "build.h"
#include "resource.h"
#include "resourceFactory.h"

BEGIN_BOOMER_NAMESPACE_EX(res)

//---

FactoryClassMetadata::FactoryClassMetadata()
    : m_resourceClass(nullptr)
{}

RTTI_BEGIN_TYPE_CLASS(FactoryClassMetadata);
RTTI_END_TYPE();

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IFactory);
    RTTI_CATEGORY("Metadata");
    RTTI_PROPERTY(m_author).editable("Author of this asset");
    RTTI_PROPERTY(m_copyright).editable("Copyright of this asset");;
    RTTI_CATEGORY("Cooking");
    RTTI_PROPERTY(m_allowInCompiledGame).editable("Is this asset allowed in the compiled game (false for temp/QA assets)");
RTTI_END_TYPE();

IFactory::IFactory()
    : m_allowInCompiledGame(true)
{}

IFactory::~IFactory()
{}

void IFactory::GetAllFactories(Array<ClassType>& outFactories)
{
    RTTI::GetInstance().enumClasses(IFactory::GetStaticClass(), outFactories);
}

void IFactory::GetAllResourceClasses(Array<ClassType>& outResourceClasses)
{
    // get all known resource factories
    Array<ClassType> factoryClasses;
    GetAllFactories(factoryClasses);

    // extract supported resource types
    for (auto factoryClass : factoryClasses)
    {
        auto metaData  = factoryClass->findMetadata<FactoryClassMetadata>();
        if (metaData != nullptr)
        {
            DEBUG_CHECK_EX(metaData->resourceClass() != nullptr, "Factory without bound resource class");
            if (metaData->resourceClass())
            {
                DEBUG_CHECK_EX(!outResourceClasses.contains(metaData->resourceClass()), "Many factories for the same resource class");
                outResourceClasses.pushBack(metaData->resourceClass());
            }
        }
    }
}

FactoryPtr IFactory::CreateFactoryForResource(ClassType resourceClass)
{
    // get all known resource factories
    Array<ClassType> factoryClasses;
    GetAllFactories(factoryClasses);

    // extract supported resource types
    for (auto factoryClass : factoryClasses)
    {
        auto metaData = factoryClass->findMetadata<FactoryClassMetadata>();
        if (metaData != nullptr)
        {
            DEBUG_CHECK_EX(metaData->resourceClass() != nullptr, "Factory without bound resource class");
            if (resourceClass == metaData->resourceClass())
            {
                return factoryClass->create<IFactory>();
            }
        }
    }

    // factory found found
    return nullptr;
}

//---

END_BOOMER_NAMESPACE_EX(res)
