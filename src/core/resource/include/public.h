/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

// glue code
#include "core_resource_glue.inl"

BEGIN_BOOMER_NAMESPACE()

//--

class ObjectIndirectTemplate;
typedef RefPtr<ObjectIndirectTemplate> ObjectIndirectTemplatePtr;
typedef RefWeakPtr<ObjectIndirectTemplate> ObjectIndirectTemplateWeakPtr;

//--

enum class DepotType : uint8_t
{
    Engine = 0,
    Project = 1,
};

class DepotService;
class DepotStructure;
struct DepotPathAppendBuffer;

//--



typedef uint32_t ResourceUniqueID;
typedef uint32_t ResourceRuntimeVersion;

class ResourceID;

class IResource;
typedef RefPtr<IResource> ResourcePtr;

class BaseReference;
class BaseAsyncReference;
          
template< typename T >
class ResourceRef;

template< typename T >
class ResourceAsyncRef;

typedef SpecificClassType<IResource> ResourceClass;

//--

class IResourceFactory;
typedef RefPtr<IResourceFactory> FactoryPtr;

class ResourceMetadata;
typedef RefPtr<ResourceMetadata> ResourceMetadataPtr;

class ResourceConfiguration;
typedef RefPtr<ResourceConfiguration> ResourceConfigurationPtr;

//--

/// load resource by path from the default depot directory
/// NOTE: this will yield the current job until the resource is loaded
extern CORE_RESOURCE_API CAN_YIELD ResourcePtr LoadResource(StringView path, ClassType expectedClass = nullptr);

/// load resource by ID (requires the .xmeta file to exist) 
/// NOTE: this will yield the current job until the resource is loaded
extern CORE_RESOURCE_API CAN_YIELD ResourcePtr LoadResource(const ResourceID& id, ClassType expectedClass = nullptr);

/// load resource of given expected type by path from the default depot directory
/// NOTE: this will yield the current job until the resource is loaded
template< typename T >
INLINE RefPtr<T> LoadResource(StringView path)
{
    return rtti_cast<T>(LoadResource(path, T::GetStaticClass()));
}

/// load resource of given expected type by ID (requires the .xmeta file to exist) 
/// NOTE: this will yield the current job until the resource is loaded
template< typename T >
INLINE RefPtr<T> LoadResource(const ResourceID& id)
{
    return rtti_cast<T>(LoadResource(path, T::GetStaticClass()));
}

//--

// extract list of resources used by given object (and child objects)
extern CORE_RESOURCE_API void ExtractUsedResources(const IObject* object, HashMap<ResourceID, uint32_t>& outResourceCounts);

//--

// clone object
extern CORE_RESOURCE_API ObjectPtr CloneObjectUntyped(const IObject* object);

// clone object
template< typename T >
INLINE RefPtr<T> CloneObject(const T* object)
{
    return rtti_cast<T>(CloneObjectUntyped(static_cast<const IObject*>(object)));
}

// clone object
template< typename T >
INLINE RefPtr<T> CloneObject(const RefPtr<T>& object)
{
    return rtti_cast<T>(CloneObjectUntyped(static_cast<const IObject*>(object)));
}

// save object to binary buffer
extern CORE_RESOURCE_API Buffer SaveObjectToBuffer(const IObject* object);

// load object from binary buffer
extern CORE_RESOURCE_API ObjectPtr LoadObjectFromBuffer(const void* data, uint64_t size, bool loadImports=false);

//--

/// load a valid resource reference from a depot path
extern CORE_RESOURCE_API BaseReference LoadResourceRef(StringView depotPath, ClassType cls);

/// check if depot file can be loaded into given class
template< typename T >
INLINE ResourceRef<T> LoadResourceRef(StringView depotPath)
{
    const auto ref = LoadResourceRef(depotPath, T::GetStaticClass());
    return *(const ResourceRef<T>*) & ref;
}

/// build a valid async resource reference from a depot path
extern CORE_RESOURCE_API BaseAsyncReference BuildAsyncResourceRef(StringView depotPath, ClassType cls);

/// check if depot file can be loaded into given class
template< typename T >
INLINE ResourceAsyncRef<T> BuildAsyncResourceRef(StringView depotPath)
{
    const auto ref = BuildAsyncResourceRef(depotPath, T::GetStaticClass());
    return *(const ResourceAsyncRef<T>*) & ref;
}

//--

END_BOOMER_NAMESPACE()

// very commonly used stuff
#include "resource.h"
#include "reference.h"
#include "asyncReference.h"
#include "staticResource.h"