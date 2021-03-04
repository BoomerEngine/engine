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

END_BOOMER_NAMESPACE()

BEGIN_BOOMER_NAMESPACE()

class PathResolver;
        
class BaseReference;
class BaseAsyncReference;
          
class ResourcePath;
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

/// load resource from the default depot directory using the application loading service
/// NOTE: this will yield the current job until the resource is loaded
extern CORE_RESOURCE_API CAN_YIELD ResourcePtr LoadResource(const ResourcePath& path);

/// nice helper for async loading of resources if the resource exists it's returned right away without any extra fibers created (it's the major performance win)
/// if resource does not exist it's queued for loading and internal fiber is created to service it
/// NOTE: if the resource exists at the moment of the call the callback function is called right away
extern CORE_RESOURCE_API void LoadResourceAsync(const ResourcePath& key, const std::function<void(const ResourcePtr&)>& funcLoaded);

/// load resource from the default depot directory
    /// NOTE: this will yield the current job until the resource is loaded
template< typename T >
INLINE RefPtr<T> LoadResource(StringView path)
{
    return rtti_cast<T>(LoadResource(ResourcePath(path)));
}

/// typed wrapper for loadResourceAsync
template< typename T >
INLINE void LoadResourceAsync(const ResourcePath& key, const std::function<void(const RefPtr<T>&)>& funcLoaded)
{
    auto funcWrapper = [funcLoaded](const ResourcePtr& loaded)
    {
        funcLoaded(rtti_cast<T>(loaded));
    };

    LoadResourceAsync(key, funcWrapper);
}

// clone object
extern CORE_RESOURCE_API ObjectPtr CloneObjectUntyped(const IObject* object, const IObject* newParent = nullptr, ResourceLoader* loader = nullptr, SpecificClassType<IObject> mutatedClass = nullptr);

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
extern CORE_RESOURCE_API ObjectPtr LoadObjectFromBuffer(const void* data, uint64_t size, ResourceLoader* loader=nullptr, SpecificClassType<IObject> mutatedClass = nullptr);

//--

// extract list of resources used by given object (and child objects)
extern CORE_RESOURCE_API void ExtractUsedResources(const IObject* object, HashMap<ResourcePath, uint32_t>& outResourceCounts);

END_BOOMER_NAMESPACE()

// very commonly used stuff
#include "resource.h"
#include "reference.h"
#include "reflection.h"
#include "staticResource.h"