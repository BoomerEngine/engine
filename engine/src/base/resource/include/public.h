/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

// glue code
#include "base_resource_glue.inl"

DECLARE_GLOBAL_EVENT(EVENT_RESOURCE_LOADER_FILE_LOADING, base::res::ResourceKey)
DECLARE_GLOBAL_EVENT(EVENT_RESOURCE_LOADER_FILE_LOADED, base::res::ResourceHandle)
DECLARE_GLOBAL_EVENT(EVENT_RESOURCE_LOADER_FILE_UNLOADED, base::res::ResourceKey)
DECLARE_GLOBAL_EVENT(EVENT_RESOURCE_LOADER_FILE_FAILED, base::res::ResourceKey)

DECLARE_GLOBAL_EVENT(EVENT_RESOURCE_MODIFIED)
DECLARE_GLOBAL_EVENT(EVENT_RESOURCE_RELOADED)

namespace base
{
    class ObjectIndirectTemplate;
    typedef RefPtr<ObjectIndirectTemplate> ObjectIndirectTemplatePtr;
    typedef RefWeakPtr<ObjectIndirectTemplate> ObjectIndirectTemplateWeakPtr;

    namespace res
    {
        class PathResolver;
        
        class BaseReference;
        class BaseAsyncReference;
          
        class ResourceKey;
        class ResourceThumbnail;

        class IResourceLoader;
        class IResourcePackageLoader;
        class IResourceReloadingHook;

        typedef uint32_t ResourceUniqueID;
        typedef uint32_t ResourceRuntimeVersion;

        class IResource;
        typedef RefPtr< IResource > ResourceHandle;
        typedef RefWeakPtr< IResource > ResourceWeakHandle;
        typedef RefPtr< IResource > ResourcePtr;
        typedef RefWeakPtr< IResource > ResourceWeakPtr;

        template< typename T >
        class Ref;

        class IResourceLoader;

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

    } // res

    // get global loader
    extern BASE_RESOURCE_API res::IResourceLoader* GlobalLoader();

    /// load resource from the default depot directory using the application loading service
    /// NOTE: this will yield the current job until the resource is loaded
    extern BASE_RESOURCE_API CAN_YIELD res::BaseReference LoadResource(const res::ResourceKey& key);

    /// nice helper for async loading of resources if the resource exists it's returned right away without any extra fibers created (it's the major performance win)
    /// if resource does not exist it's queued for loading and internal fiber is created to service it
    /// NOTE: if the resource exists at the moment of the call the callback function is called right away
    extern BASE_RESOURCE_API void LoadResourceAsync(const res::ResourceKey& key, const std::function<void(const res::BaseReference&)>& funcLoaded);

    /// load resource from the default depot directory
     /// NOTE: this will yield the current job until the resource is loaded
    template< typename T >
    INLINE res::Ref<T> LoadResource(StringView path)
    {
        return rtti_cast<T>(LoadResource(res::ResourceKey(path, T::GetStaticClass())));
    }

    /// typed wrapper for loadResourceAsync
    template< typename T >
    INLINE void LoadResourceAsync(StringView path, const std::function<void(const res::Ref<T>&)>& funcLoaded)
    {
        auto funcWrapper = [funcLoaded](const res::BaseReference& loaded)
        {
            funcLoaded(rtti_cast<T>(loaded.acquire()));
        };

        LoadResourceAsync(res::ResourceKey(path, T::GetStaticClass()), funcWrapper);
    }

    /// typed wrapper for loadResourceAsync
    template< typename T >
    INLINE void LoadResourceAsync(const res::ResourceKey& key, const std::function<void(const res::Ref<T>&)>& funcLoaded)
    {
        auto funcWrapper = [funcLoaded](const res::BaseReference& loaded)
        {
            funcLoaded(rtti_cast<T>(loaded.acquire()));
        };

        LoadResourceAsync(key, funcWrapper);
    }

    // clone object
    extern BASE_RESOURCE_API ObjectPtr CloneObjectUntyped(const IObject* object, const IObject* newParent = nullptr, res::IResourceLoader* loader = nullptr, SpecificClassType<IObject> mutatedClass = nullptr);

    // clone object
    template< typename T >
    INLINE RefPtr<T> CloneObject(const T* object, const IObject* newParent = nullptr, res::IResourceLoader* loader = nullptr, SpecificClassType<IObject> mutatedClass = nullptr)
    {
        return rtti_cast<T>(CloneObjectUntyped(static_cast<const IObject*>(object), newParent, loader, mutatedClass));
    }

    // clone object
    template< typename T >
    INLINE RefPtr<T> CloneObject(const RefPtr<T>& object, const IObject* newParent = nullptr, res::IResourceLoader* loader = nullptr, SpecificClassType<IObject> mutatedClass = nullptr)
    {
        return rtti_cast<T>(CloneObjectUntyped(static_cast<const IObject*>(object), newParent, loader, mutatedClass));
    }

    // save object to binary buffer
    extern BASE_RESOURCE_API Buffer SaveObjectToBuffer(const IObject* object);

    // load object from binary buffer
    extern BASE_RESOURCE_API ObjectPtr LoadObjectFromBuffer(const void* data, uint64_t size, res::IResourceLoader* loader=nullptr, SpecificClassType<IObject> mutatedClass = nullptr);

    //--

    // extract list of resources used by given object (and child objects)
    extern BASE_RESOURCE_API void ExtractUsedResources(const IObject* object, HashMap<res::ResourceKey, uint32_t>& outResourceCounts);

    //--

} // base

// very commonly used stuff
#include "resource.h"
#include "resourceReference.h"
#include "resourceReflection.h"
#include "resourceStaticResource.h"