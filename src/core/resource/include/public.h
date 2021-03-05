/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

// glue code
#include "core_resource_glue.inl"

BEGIN_BOOMER_NAMESPACE()

class ObjectIndirectTemplate;
typedef RefPtr<ObjectIndirectTemplate> ObjectIndirectTemplatePtr;
typedef RefWeakPtr<ObjectIndirectTemplate> ObjectIndirectTemplateWeakPtr;

class DepotService;
class DepotStructure;

DECLARE_GLOBAL_EVENT(EVENT_RESOURCE_LOADER_FILE_LOADING, ResourceKey)
DECLARE_GLOBAL_EVENT(EVENT_RESOURCE_LOADER_FILE_LOADED, ResourcePtr)
DECLARE_GLOBAL_EVENT(EVENT_RESOURCE_LOADER_FILE_UNLOADED, ResourceKey)
DECLARE_GLOBAL_EVENT(EVENT_RESOURCE_LOADER_FILE_FAILED, ResourceKey)

DECLARE_GLOBAL_EVENT(EVENT_RESOURCE_MODIFIED)
DECLARE_GLOBAL_EVENT(EVENT_RESOURCE_RELOADED)

DECLARE_GLOBAL_EVENT(EVENT_DEPOT_FILE_CHANGED, StringBuf)
DECLARE_GLOBAL_EVENT(EVENT_DEPOT_FILE_ADDED, StringBuf)
DECLARE_GLOBAL_EVENT(EVENT_DEPOT_FILE_REMOVED, StringBuf)
DECLARE_GLOBAL_EVENT(EVENT_DEPOT_FILE_RELOADED, StringBuf)
DECLARE_GLOBAL_EVENT(EVENT_DEPOT_DIRECTORY_ADDED, StringBuf)
DECLARE_GLOBAL_EVENT(EVENT_DEPOT_DIRECTORY_REMOVED, StringBuf)

class PathResolver;
        
class BaseReference;
class BaseAsyncReference;
          
class ResourceID;
class ResourceThumbnail;

class ResourceLoader;
        
typedef uint32_t ResourceUniqueID;
typedef uint32_t ResourceRuntimeVersion;

class IResource;
typedef RefPtr<IResource> ResourcePtr;

template< typename T >
class ResourceRef;

template< typename T >
class ResourceAsyncRef;

typedef FiberSemaphore ResourceLoadingFence;
typedef std::function< void(const ResourcePtr& res) > ResourceLoadedCallback;

typedef uint64_t ResourceClassHash; // works as FormatCC

class IResourceFactory;
typedef RefPtr<IResourceFactory> FactoryPtr;

class ResourceMetadata;
typedef RefPtr<ResourceMetadata> ResourceMetadataPtr;

class ICookingErrorReporter;
class IResourceCookerInterface;

class ResourceConfiguration;
typedef RefPtr<ResourceConfiguration> ResourceConfigurationPtr;

//--

// get global loader
extern CORE_RESOURCE_API ResourceLoader* GlobalLoader();

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
extern CORE_RESOURCE_API ObjectPtr CloneObjectUntyped(const IObject* object, const IObject* newParent = nullptr, bool loadImports = true, SpecificClassType<IObject> mutatedClass = nullptr);

// clone object
template< typename T >
INLINE RefPtr<T> CloneObject(const T* object, const IObject* newParent = nullptr, ResourceLoader* loader = nullptr, SpecificClassType<IObject> mutatedClass = nullptr)
{
    return rtti_cast<T>(CloneObjectUntyped(static_cast<const IObject*>(object), newParent, loader, mutatedClass));
}

// clone object
template< typename T >
INLINE RefPtr<T> CloneObject(const RefPtr<T>& object, const IObject* newParent = nullptr, ResourceLoader* loader = nullptr, SpecificClassType<IObject> mutatedClass = nullptr)
{
    return rtti_cast<T>(CloneObjectUntyped(static_cast<const IObject*>(object), newParent, loader, mutatedClass));
}

// save object to binary buffer
extern CORE_RESOURCE_API Buffer SaveObjectToBuffer(const IObject* object);

// load object from binary buffer
extern CORE_RESOURCE_API ObjectPtr LoadObjectFromBuffer(const void* data, uint64_t size, bool loadImports=false, SpecificClassType<IObject> mutatedClass = nullptr);

//--

END_BOOMER_NAMESPACE()

// very commonly used stuff
#include "resource.h"
#include "reference.h"
#include "reflection.h"
#include "staticResource.h"