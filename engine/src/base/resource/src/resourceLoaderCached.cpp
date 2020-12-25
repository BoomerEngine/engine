/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource #]
***/

#include "build.h"

#include "resource.h"
#include "resourceLoaderCached.h"
#include "resourceClassLookup.h"

#include "base/system/include/scopeLock.h"
#include "base/object/include/public.h"
#include "base/fibers/include/fiberSystem.h"
#include "base/containers/include/inplaceArray.h"
#include "base/system/include/thread.h"

namespace base
{
    namespace res
    {

        //--

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IResourceLoaderCached);
        RTTI_END_TYPE();

        IResourceLoaderCached::~IResourceLoaderCached()
        {
            for (auto& weakRef : m_loadedResources.values())
            {
                auto resource = weakRef.lock();
                if (resource)
                    resource->m_loader = nullptr;
            }

            m_loadedResources.clear();
        }

        ResourceHandle IResourceLoaderCached::loadResource(const ResourceKey& key)
        {
            // invalid resource path always produces invalid resource
            if (key.empty())
                return nullptr;

            auto scopeLock = CreateLock(m_lock);

            // get the loaded resource, that's the most common case so do it first
            // NOTE: if this is disabled we will always create the job to process the resource
            ResourceHandle existingLoadedResource;
            {
                base::RefWeakPtr<IResource> loadedResourceWeakRef;
                if (m_loadedResources.find(key, loadedResourceWeakRef))
                    if (existingLoadedResource = loadedResourceWeakRef.lock())
                        if (validateExistingResource(existingLoadedResource, key))
                            return existingLoadedResource;
            }

            // lookup the resource from the loaded list
            // NOTE: there may be different resources loadable from a given file
            {
                base::RefWeakPtr<LoadingJob> weakJobRef;
                if (m_loadingJobs.find(key, weakJobRef))
                {
                    // get the lock to the loading job
                    auto loadingJob = weakJobRef.lock();
                    if (loadingJob) // there's an active job for this resource key
                    {
                        scopeLock.release();

                        // wait for the job to finish
                        Fibers::GetInstance().waitForCounterAndRelease(loadingJob->m_signal);

                        // return the resource pointer produced by the job
                        return loadingJob->m_loadedResource;
                    }
                }
            }

            // there's no loading job, create one, this will gate all other threads to wait for us to finish
            auto loadingJob = RefNew<LoadingJob>();
            loadingJob->m_key = key;
            loadingJob->m_signal = Fibers::GetInstance().createCounter("LoadingJob", 1);
            m_loadingJobs[key] = loadingJob;

            // unlock the system so other threads can start other loading jobs or join waiting on the one we just created
            scopeLock.release();

            // notify anybody interested
            notifyResourceLoading(key);

            // ask the raw loader to load the content of the resource
            if (auto resource = loadResourceOnce(key))
            {
                // add for safe keeping so we can return it
                // NOTE: we may decide to reuse the same resource, but it does not change the logic here
                {
                    auto scopeLock2 = CreateLock(m_lock);
                    m_loadedResources[key] = resource;
                }

                // signal the job as finished, this will unblock other threads
                ASSERT_EX(!loadingJob->m_loadedResource, "Resource aparently already loaded");
                loadingJob->m_loadedResource = resource;

                // notify loader
                notifyResourceLoaded(key, resource);

                // post a reload
                if (existingLoadedResource)
                    notifyResourceReloaded(existingLoadedResource, resource);                
            }
            else
            {
                // whoops
                notifyResourceFailed(key);
            }

            // finish with result
            Fibers::GetInstance().signalCounter(loadingJob->m_signal);
            return loadingJob->m_loadedResource;
        }

        bool IResourceLoaderCached::acquireLoadedResource(const ResourceKey& key, ResourceHandle& outLoadedPtr)
        {
            // class and path are required for lookup
            if (!key.empty())
            {
                auto lock = base::CreateLock(m_lock);

                // get the loaded resource, that's the most common case so do it first
                base::RefWeakPtr<IResource> loadedResourceWeakRef;
                if (m_loadedResources.find(key, loadedResourceWeakRef))
                {
                    if (auto loadedResource = loadedResourceWeakRef.lock())
                    {
                        outLoadedPtr = loadedResource;
                        return true;
                    }
                }

                // are we loading it ?
                base::RefWeakPtr<LoadingJob> weakJobRef;
                if (m_loadingJobs.find(key, weakJobRef))
                {
                    // get the lock to the loading job
                    if (auto loadingJob = weakJobRef.lock())
                    {
                        outLoadedPtr = loadingJob->m_loadedResource;
                        return true;
                    }
                }
            }

            // not found or loading
            return false;
        }

        //---

        void IResourceLoaderCached::notifyResourceReloaded(const ResourceHandle& currentResource, const ResourceHandle& newResource)
        {

        }

        bool IResourceLoaderCached::validateExistingResource(const ResourceHandle& res, const ResourceKey& key) const
        {
            return true;
        }

        //---

    } // res
} // base

