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

DECLARE_GLOBAL_EVENT(EVENT_RESOURCE_LOADER_FILE_LOADING, res::ResourceKey)
DECLARE_GLOBAL_EVENT(EVENT_RESOURCE_LOADER_FILE_LOADED, res::ResourceHandle)
DECLARE_GLOBAL_EVENT(EVENT_RESOURCE_LOADER_FILE_UNLOADED, res::ResourceKey)
DECLARE_GLOBAL_EVENT(EVENT_RESOURCE_LOADER_FILE_FAILED, res::ResourceKey)

DECLARE_GLOBAL_EVENT(EVENT_RESOURCE_MODIFIED)
DECLARE_GLOBAL_EVENT(EVENT_RESOURCE_RELOADED)

DECLARE_GLOBAL_EVENT(EVENT_DEPOT_FILE_CHANGED, StringBuf)
DECLARE_GLOBAL_EVENT(EVENT_DEPOT_FILE_ADDED, StringBuf)
DECLARE_GLOBAL_EVENT(EVENT_DEPOT_FILE_REMOVED, StringBuf)
DECLARE_GLOBAL_EVENT(EVENT_DEPOT_FILE_RELOADED, StringBuf)
DECLARE_GLOBAL_EVENT(EVENT_DEPOT_DIRECTORY_ADDED, StringBuf)
DECLARE_GLOBAL_EVENT(EVENT_DEPOT_DIRECTORY_REMOVED, StringBuf)

END_BOOMER_NAMESPACE()

BEGIN_BOOMER_NAMESPACE_EX(res)

class PathResolver;
        
class BaseReference;
class BaseAsyncReference;
          
class ResourcePath;
class ResourceThumbnail;

class ResourceLoader;
        
typedef uint32_t ResourceUniqueID;
typedef uint32_t ResourceRuntimeVersion;

class IResource;
typedef RefPtr< IResource > ResourceHandle;
typedef RefWeakPtr< IResource > ResourceWeakHandle;
typedef RefPtr< IResource > ResourcePtr;
typedef RefWeakPtr< IResource > ResourceWeakPtr;

template< typename T >
class Ref;

typedef fibers::WaitCounter ResourceLoadingFence;
typedef std::function< void(const ResourceHandle& res) > ResourceLoadedCallback;

typedef uint64_t ResourceClassHash; // works as FormatCC

class IFactory;
typedef RefPtr<IFactory> FactoryPtr;

class Metadata;
typedef RefPtr<Metadata> MetadataPtr;

class MetadataCache;
class DependencyCache;

class BlobCache;

class ICookingErrorReporter;
class IResourceCookerInterface;
class ResourceCookerMemoryBlob;

class ResourceConfiguration;
typedef RefPtr<ResourceConfiguration> ResourceConfigurationPtr;

class IResourceCacheBlob;
typedef RefPtr<IResourceCacheBlob> ResourceCacheBlobPtr;

class IResourceCookingDetailedSettings;
typedef RefPtr<IResourceCookingDetailedSettings> ResourceCookingDetailedSettingsPtr;

END_BOOMER_NAMESPACE_EX(res)

BEGIN_BOOMER_NAMESPACE()

// get global loader
extern CORE_RESOURCE_API res::ResourceLoader* GlobalLoader();

/// load resource from the default depot directory using the application loading service
/// NOTE: this will yield the current job until the resource is loaded
extern CORE_RESOURCE_API CAN_YIELD res::ResourcePtr LoadResource(const res::ResourcePath& path);

/// nice helper for async loading of resources if the resource exists it's returned right away without any extra fibers created (it's the major performance win)
/// if resource does not exist it's queued for loading and internal fiber is created to service it
/// NOTE: if the resource exists at the moment of the call the callback function is called right away
extern CORE_RESOURCE_API void LoadResourceAsync(const res::ResourcePath& key, const std::function<void(const res::ResourcePtr&)>& funcLoaded);

/// load resource from the default depot directory
    /// NOTE: this will yield the current job until the resource is loaded
template< typename T >
INLINE RefPtr<T> LoadResource(StringView path)
{
    return rtti_cast<T>(LoadResource(res::ResourcePath(path)));
}

/// typed wrapper for loadResourceAsync
template< typename T >
INLINE void LoadResourceAsync(StringView path, const std::function<void(const RefPtr<T>&)>& funcLoaded)
{
    auto funcWrapper = [funcLoaded](const res::BaseReference& loaded)
    {
        funcLoaded(rtti_cast<T>(loaded));
    };

    LoadResourceAsync(path, funcWrapper);
}

/// typed wrapper for loadResourceAsync
template< typename T >
INLINE void LoadResourceAsync(const res::ResourcePath& key, const std::function<void(const RefPtr<T>&)>& funcLoaded)
{
    auto funcWrapper = [funcLoaded](const res::ResourcePtr& loaded)
    {
        funcLoaded(rtti_cast<T>(loaded));
    };

    LoadResourceAsync(key, funcWrapper);
}

// clone object
extern CORE_RESOURCE_API ObjectPtr CloneObjectUntyped(const IObject* object, const IObject* newParent = nullptr, res::ResourceLoader* loader = nullptr, SpecificClassType<IObject> mutatedClass = nullptr);

// clone object
template< typename T >
INLINE RefPtr<T> CloneObject(const T* object, const IObject* newParent = nullptr, res::ResourceLoader* loader = nullptr, SpecificClassType<IObject> mutatedClass = nullptr)
{
    return rtti_cast<T>(CloneObjectUntyped(static_cast<const IObject*>(object), newParent, loader, mutatedClass));
}

// clone object
template< typename T >
INLINE RefPtr<T> CloneObject(const RefPtr<T>& object, const IObject* newParent = nullptr, res::ResourceLoader* loader = nullptr, SpecificClassType<IObject> mutatedClass = nullptr)
{
    return rtti_cast<T>(CloneObjectUntyped(static_cast<const IObject*>(object), newParent, loader, mutatedClass));
}

// save object to binary buffer
extern CORE_RESOURCE_API Buffer SaveObjectToBuffer(const IObject* object);

// load object from binary buffer
extern CORE_RESOURCE_API ObjectPtr LoadObjectFromBuffer(const void* data, uint64_t size, res::ResourceLoader* loader=nullptr, SpecificClassType<IObject> mutatedClass = nullptr);

//--

// extract list of resources used by given object (and child objects)
extern CORE_RESOURCE_API void ExtractUsedResources(const IObject* object, HashMap<res::ResourcePath, uint32_t>& outResourceCounts);

END_BOOMER_NAMESPACE()

// very commonly used stuff
#include "resource.h"
#include "resourceReference.h"
#include "resourceReflection.h"
#include "resourceStaticResource.h"