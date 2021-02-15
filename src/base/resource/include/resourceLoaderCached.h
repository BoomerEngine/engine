/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource #]
***/

#pragma once

#include "base/containers/include/array.h"

#include "resourceLoader.h"

namespace base
{
    namespace res
    {

        //---

        /// Resource loader with basic caching functionality and multi threaded "gating" 
        class BASE_RESOURCE_API IResourceLoaderCached : public IResourceLoader
        {
            RTTI_DECLARE_VIRTUAL_CLASS(IResourceLoaderCached, IResourceLoader);

        public:
            virtual ~IResourceLoaderCached();

            //----

            /// Load a resource from specified path, may return NULL resource
            /// NOTE: this function can wait for other jobs and thus yield the active fiber
            virtual ResourceHandle loadResource(const ResourceKey& key) override CAN_YIELD;

            /// Try to get already loaded resource IF IT EXISTS
            /// NOTE: this can be used to optimize calls to async loading functions by first querying if resource exists
            /// NOTE: this ONLY returns fully loaded resources (so if the resource is actually being loaded now it's not returned, as the name of the function states)
            virtual bool acquireLoadedResource(const ResourceKey& key, ResourceHandle& outLoadedResource) override;

        protected:
            /// Internal interface - load a single resource
            virtual ResourceHandle loadResourceOnce(const ResourceKey& key) CAN_YIELD = 0;

            /// Internal interface - check if internal resource can be used (ie. is still up to date)
            virtual bool validateExistingResource(const ResourceHandle& res, const ResourceKey& key) const;

            /// Internal interface - notify that new version of resource was loaded
            virtual void notifyResourceReloaded(const ResourceHandle& currentResource, const ResourceHandle& newResource);

            /// Internal interface - translate one resource key to other key, can be used to alias resources
            virtual ResourceKey translateResourceKey(const ResourceKey& key) const;

            //--
        
            struct LoadingJob : public IReferencable
            {
                ResourceKey m_key;
                fibers::WaitCounter m_signal;
                ResourceHandle m_loadedResource;
            };

            SpinLock m_lock;

            typedef HashMap<ResourceKey, RefWeakPtr<LoadingJob>>    TLoadingJobMap;
            TLoadingJobMap m_loadingJobs;       // map for active loading jobs

            typedef HashMap<ResourceKey, RefWeakPtr<IResource>>    TResourceMap;
            TResourceMap m_loadedResources;       // map for active loading jobs
        };

        //-----

    } // res
} // base