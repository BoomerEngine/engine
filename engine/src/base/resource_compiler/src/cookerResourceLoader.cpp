/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: cooking #]
***/

#include "build.h"
#include "cooker.h"
#include "cookerResourceLoader.h"
#include "cookerDependencyTracking.h"

#include "base/resource_compiler/include/depotStructure.h"
#include "base/resource/include/resourceMetadata.h"
#include "base/resource/include/resourcePath.h"
#include "base/resource/include/resourceMountPoint.h"
#include "base/resource/include/resourceStaticResource.h"
#include "base/object/include/objectGlobalRegistry.h"
#include "base/io/include/ioSystem.h"
#include "base/resource/include/resourceFileLoader.h"


namespace base
{
     namespace res
    {
        ///--

        RTTI_BEGIN_TYPE_CLASS(ResourceLoaderCooker);
        RTTI_END_TYPE();
        
        ///--

        ResourceLoaderCooker::ResourceLoaderCooker()
        {
            m_depot.create();
            m_depTracker.create(*m_depot);
            m_cooker.create(*m_depot, this);
        }

        ResourceLoaderCooker::~ResourceLoaderCooker()
        {
            m_cooker.reset();
            m_depTracker.reset();
            m_depot.reset();
        }

        bool ResourceLoaderCooker::initialize(const app::CommandLine& cmdLine)
        {
            // look for "project.xml"
            auto startPath = base::io::SystemPath(io::PathCategory::ExecutableDir).view().baseDirectory();
            while (!startPath.empty())
            {
                TRACE_INFO("Checking directory '{}' for project manifest", startPath);

                const auto depotManifestPath = StringBuf(TempString("{}project.xml", startPath));
                if (base::io::FileExists(depotManifestPath))
                {
                    if (m_depot->populateFromManifest(depotManifestPath))
                        break;
                }

                startPath = startPath.parentDirectory();
            }

            // done
            return true;
        }

        depot::DepotStructure* ResourceLoaderCooker::queryUncookedDepot() const
        {
            return m_depot.get();
        }

        void ResourceLoaderCooker::update()
        {
            ASSERT_EX(Fibers::GetInstance().isMainThread(), "Reloading can only happen on main thread");
            if (!Fibers::GetInstance().isMainThread())
                return; // don't risk it even in release

            // check what files changed and add the resources that depend on those changed files to the reload queue
            checkChangedFiles();

            // update reloading state
            if (!m_resourceBeingReloaded)
            {
                startReloading();
            }
            else
            {
                ResourcePtr currentResource, newResource;
                if (tryFinishReloading(currentResource, newResource))
                    applyReloading(currentResource, newResource);
            }
        }
        
        //--

        void ResourceLoaderCooker::checkChangedFiles()
        {
            Array<ResourceKey> changedFiles;
            m_depTracker->queryFilesForReloading(changedFiles);

            if (!changedFiles.empty())
            {
                TRACE_INFO("Dependency tracker reported {} file(s) to reload", changedFiles.size());

                // update reload queue
                {
                    auto lock = CreateLock(m_lock);
                    for (const auto& key : changedFiles)
                    {
                        // already in reloading queue
                        if (m_pendingReloadSet.contains(key))
                        {
                            TRACE_INFO("File '{}' is already on the reloading queue", key);
                            continue;
                        }

                        // check if already have a file
                        RefWeakPtr<LoadingJob> loadingJobWeak;
                        m_loadingJobs.find(key, loadingJobWeak);
                        if (auto loadingJob = loadingJobWeak.lock())
                        {
                            TRACE_INFO("File '{}' was changed while loaded, file will be reloaded once loading is done");
                            if (m_pendingReloadSet.insert(key))
                                m_pendingReloadQueue.push(key);
                            continue;
                        }

                        // already loaded ?
                        RefWeakPtr<IResource> loadedResourceWeak;
                        m_loadedResources.find(key, loadedResourceWeak);
                        if (auto loadedResource = loadedResourceWeak.lock())
                        {
                            TRACE_INFO("Resource '{}' flagged for reloading", key);
                            if (m_pendingReloadSet.insert(key))
                                m_pendingReloadQueue.push(key);
                        }
                        else
                        {
                            TRACE_INFO("Resource '{}' not currently loaded, new version will be automaticall loaded on next resource load", key);
                        }
                    }
                }
            }
        }

        ResourcePtr ResourceLoaderCooker::pickupNextResourceForReloading_NoLock()
        {
            while (!m_pendingReloadQueue.empty())
            {
                // get the resource to reload
                auto key = m_pendingReloadQueue.top();

                // not in set
                if (!m_pendingReloadSet.contains(key))
                {
                    TRACE_INFO("Resource '{}' is no longer scheduled for reloading", key);
                    m_pendingReloadQueue.pop();
                    continue;
                }

                // get the current resource
                ResourceHandle loadedResource;
                {
                    RefWeakPtr<IResource> loadedResourceWeak;
                    m_loadedResources.find(key, loadedResourceWeak);
                    loadedResource = loadedResourceWeak.lock();
                }

                // resource is not loaded, reload may not needed
                if (!loadedResource)
                {
                    RefWeakPtr<LoadingJob> loadingJobWeak;
                    m_loadingJobs.find(key, loadingJobWeak);
                    if (auto loadingJob = loadingJobWeak.lock())
                    {
                        // we are loading this resource, retry in future
                        TRACE_INFO("Resource '{}' scheduled for reload is still loading", key);
                        break;
                    }
                    else
                    {
                        // resource no longer loaded
                        TRACE_INFO("Resource '{}' scheduled for reload is no longer loaded", key);
                        m_pendingReloadSet.remove(key);
                        m_pendingReloadQueue.pop();
                        continue;
                    }
                }

                // use this resource for reloading
                return loadedResource;
            }

            // nothing to reload
            return nullptr;
        }

        void ResourceLoaderCooker::startReloading()
        {
            auto lock = CreateLock(m_lock);

            // pick first resource from the reload queue
            if (auto nextResourceToReload = pickupNextResourceForReloading_NoLock())
            {
                const auto key = nextResourceToReload->key();

                // start reloading
                DEBUG_CHECK(!m_resourceBeingReloaded);
                DEBUG_CHECK(!m_reloadFinished);
                m_resourceBeingReloadedKey = key;
                m_resourceBeingReloaded = nextResourceToReload;
                m_reloadedResource = nullptr;
                m_reloadFinished = false;
                m_pendingReloadSet.remove(key);
                m_pendingReloadQueue.pop();

                // start reloading
                RunFiber("ResourceReload") << [this, key](FIBER_FUNC)
                {
                    processReloading(key);
                };
            }
        }


        void ResourceLoaderCooker::processReloading(ResourceKey key)
        {
            TRACE_INFO("Reloading of '{}' started", key);

            // load the file
            if (auto reloadedResource = loadInternal(key,false))
            {
                TRACE_INFO("Reloading of '{}' finished", key);

                auto lock = CreateLock(m_reloadLock);
                m_reloadedResource = reloadedResource;
                m_reloadFinished = true;
            }
            else
            {
                TRACE_INFO("Reloading of '{}' failed", key);

                auto lock = CreateLock(m_reloadLock);
                m_reloadedResource = nullptr;
                m_reloadFinished = true;
            }
        }

        bool ResourceLoaderCooker::tryFinishReloading(ResourcePtr& outReloadedResource, ResourcePtr& outNewResource)
        {
            // finish reloading if it really did finish :)
            auto lock = CreateLock(m_reloadLock);
            if (m_reloadFinished)
            {
                outReloadedResource = m_resourceBeingReloaded;
                outNewResource = m_reloadedResource;

                if (m_reloadedResource)
                {
                    auto lock = CreateLock(m_lock);
                    m_loadedResources[m_resourceBeingReloadedKey] = m_reloadedResource;
                }

                m_resourceBeingReloadedKey = ResourceKey();
                m_reloadedResource.reset();
                m_resourceBeingReloaded.reset();
                m_reloadFinished = false;
                return true;
            }

            // reloading still going
            return false;
        }

        void ResourceLoaderCooker::applyReloading(ResourcePtr currentResource, ResourcePtr newResource)
        {
            if (currentResource && newResource)
            {
                ScopeTimer timer;

                TRACE_INFO("Applying reload to '{}'", currentResource->key());
                currentResource->applyReload(newResource);

                uint32_t numObjectsVisited = 0;
                Array<ObjectPtr> affectedObjects;
                ObjectGlobalRegistry::GetInstance().iterateAllObjects([&numObjectsVisited, &affectedObjects, &currentResource, &newResource](IObject* obj)
                {
                        numObjectsVisited += 1;

                        if (obj->onResourceReloading(currentResource, newResource))
                            affectedObjects.pushBack(ObjectPtr(AddRef(obj)));

                        return false;
                });

                for (const auto obj : affectedObjects)
                    obj->onResourceReloadFinished(currentResource, newResource);

                IStaticResource::ApplyReload(currentResource, newResource);

                if (const auto depotPath = StringBuf(currentResource->key().path().view()))
                    DispatchGlobalEvent(m_depot->eventKey(), EVENT_DEPOT_FILE_RELOADED, depotPath);

                TRACE_INFO("Reload to '{}' applied in {}, {} of {} objects pached", currentResource->key(), timer, affectedObjects.size(), numObjectsVisited);
            }    
        }

        void ResourceLoaderCooker::updateEmptyFileDependencies(ResourceKey key, StringView<char> depotPath)
        {
            InplaceArray<SourceDependency, 1> dependencies;

            auto& dep = dependencies.emplaceBack();
            dep.sourcePath = StringBuf(depotPath);

            m_depTracker->notifyDependenciesChanged(key, dependencies);
        }

        void ResourceLoaderCooker::updateDirectFileDependencies(ResourceKey key, StringView<char> depotPath)
        {
            io::TimeStamp fileTimeStamp;
            if (m_depot->queryFileTimestamp(depotPath, fileTimeStamp))
            {
                InplaceArray<SourceDependency, 1> dependencies;

                auto& dep = dependencies.emplaceBack();
                dep.sourcePath = StringBuf(depotPath);
                dep.timestamp = fileTimeStamp.value();

                m_depTracker->notifyDependenciesChanged(key, dependencies);
            }
        }

        ResourcePtr ResourceLoaderCooker::loadInternalDirectly(ClassType resClass, StringView<char> filePath)
        {
            // load file content
            if (auto file = m_depot->createFileAsyncReader(filePath))
            {
                ResourceMountPoint mountPoint;
                m_depot->queryFileMountPoint(filePath, mountPoint);

                FileLoadingContext context;
                context.basePath = mountPoint.path();
                context.resourceLoader = this;

                if (LoadFile(file, context))
                {
                    if (const auto ret = context.root<IResource>())
                        return ret;
                }
            }

            return nullptr;
        }

        static ResourcePtr CreateValidStub(ClassType resClass)
        {
            TRACE_INFO("Creating stub for '{}'", resClass);

            if (!resClass->isAbstract())
                return resClass->create<IResource>();

            // TODO: metadata

            InplaceArray<ClassType, 10> resourceClasses;
            RTTI::GetInstance().enumClasses(resClass, resourceClasses);
            DEBUG_CHECK_EX(!resourceClasses.empty(), "Resource class with no actual implementation");

            if (!resourceClasses.empty())
                return resourceClasses[0]->create<IResource>();

            return nullptr;
        }

        ResourcePtr ResourceLoaderCooker::loadInternal(ResourceKey key, bool normalLoading)
        {
            // determine mount point of the resource
            ResourceMountPoint mountPoint;
            m_depot->queryFileMountPoint(key.path().view(), mountPoint);

            // abstract class case
            /*if (key.cls()->isAbstract())
            {
                const auto fileExtension = key.path().extension();
                const auto resourceByExtension = IResource::FindResourceClassByExtension(fileExtension);
                if (resourceByExtension)
                    key = ResourceKey(key.path(), resourceByExtension);
            }*/

            // direct load 
            const auto fileLoadClass = IResource::FindResourceClassByExtension(key.extension());
            if (fileLoadClass && fileLoadClass->is(key.cls()))
            {
                const auto& depotPath = key.path();
                if (const auto loadedRes = loadInternalDirectly(key.cls(), depotPath))
                {
                    loadedRes->bindToLoader(this, key, mountPoint, false);
                    updateDirectFileDependencies(key, depotPath);
                    return loadedRes;
                }
            }

            // file can't be directly loaded, can we cook it ?
            SpecificClassType<IResource> cookedResourceClass;
            if (m_cooker->canCook(key, cookedResourceClass))
            {
                if (auto cookedFile = m_cooker->cook(key))
                {
                    cookedFile->bindToLoader(this, key, mountPoint, false);

                    if (cookedFile->metadata())
                        m_depTracker->notifyDependenciesChanged(key, cookedFile->metadata()->sourceDependencies);

                    return cookedFile;
                }
            }

            // create stub
            if (fileLoadClass && fileLoadClass->is(key.cls()))
            {
                const auto& depotPath = key.path();
                auto stubRes = fileLoadClass->create<IResource>();
                stubRes->bindToLoader(this, key, mountPoint, true);
                updateEmptyFileDependencies(key, depotPath);
                return stubRes;
            }

            // loading failed with no stub as there's no hope of recovery
            return nullptr;
        }

        ResourceHandle ResourceLoaderCooker::loadResourceOnce(const ResourceKey& key)
        {
            // if we are loading the resource remove it from the queue
            if (m_pendingReloadSet.remove(key))
            {
                TRACE_INFO("Resource '{}' was loaded again before reload queue was processed", key);
            }

            // load the resource
            return loadInternal(key);
        }

        bool ResourceLoaderCooker::validateExistingResource(const ResourceHandle& res, const ResourceKey& key) const
        {
            // resource did not generate a metadata file
            if (!res->metadata() || res->metadata()->sourceDependencies.empty())
                return true;

            // check dependencies
            for (const auto& dep : res->metadata()->sourceDependencies)
            {
                io::TimeStamp fileTimeStamp;
                m_depot->queryFileTimestamp(dep.sourcePath, fileTimeStamp);

                if (fileTimeStamp.value() != dep.timestamp)
                {
                    TRACE_WARNING("Dependency of file '{}' a file ({}) has changed. We will force resource to load a new version.", key, dep.sourcePath);
                    return false;
                }
            }

            // still valid
            return true;
        }

        //---
        
    } // res
} // base


