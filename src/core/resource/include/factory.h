/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource #]
***/

#pragma once

#include "core/object/include/object.h"

BEGIN_BOOMER_NAMESPACE()

//-----

// rtti meta data with information about supported resource class that can be created byh this factory
// NOTE: one factory, one class
class CORE_RESOURCE_API ResourceFactoryClassMetadata : public IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(ResourceFactoryClassMetadata, IMetadata);

public:
    ResourceFactoryClassMetadata();

    // bind the resource class to the factory
    template< typename T >
    INLINE void bindResourceClass()
    {
        m_resourceClass = T::GetStaticClass();
    }

    // get bound resource class
    INLINE ClassType resourceClass() const
    {
        return m_resourceClass;
    }

private:
    ClassType m_resourceClass;
};

//----

// Resource factory, allows to create a resource of given class
class CORE_RESOURCE_API IResourceFactory : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(IResourceFactory, IObject);

public:
    IResourceFactory();
    virtual ~IResourceFactory();

    // create resource based on the factory setup
    virtual ResourcePtr createResource() const = 0;

    //--

    // get all registered resource factory classes
    static void GetAllFactories(Array<ClassType>& outFactories);

    // get all resource classes creatable via factories
    static void GetAllResourceClasses(Array<ResourceClass>& outResourceClasses);

    // create a factory for given resource class
    static FactoryPtr CreateFactoryForResource(ResourceClass resourceClass);

protected:
    StringBuf m_author;
    StringBuf m_copyright;
    bool m_allowInCompiledGame;
};

//------

END_BOOMER_NAMESPACE()
