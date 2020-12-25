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

        bool ResourceLoaderCooker::popNextReload(PendingReload& outReload)
        {
            auto lock = CreateLock(m_pendingReloadQueueLock);

            if (m_pendingReloadQueue.empty())
                return false;

            outReload = m_pendingReloadQueue.top();
            m_pendingReloadQueue.pop();
            return true;
        }

        void ResourceLoaderCooker::update()
        {
            DEBUG_CHECK_RETURN_EX(Fibers::GetInstance().isMainThread(), "Reloading can only happen on main thread");

            // check what files changed and add the resources that depend on those changed files to the reload queue
            checkChangedFiles();

            // apply reloads
            PendingReload reload;
            while (popNextReload(reload))
                applyReload(reload.currentResource, reload.newResource);
        }
        
        //--

        void ResourceLoaderCooker::checkChangedFiles()
        {
            Array<ResourceKey> changedFiles;
            m_depTracker->queryFilesForReloading(changedFiles);

            if (!changedFiles.empty())
            {
                TRACE_INFO("Dependency tracker reported {} file(s) to reload", changedFiles.size());

                /*// update reload queue
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
                            TRACE_INFO("Resource '{}' not currently loaded, new version will be automatically loaded on next resource load", key);
                        }
                    }
                }*/
            }
        }

        void ResourceLoaderCooker::notifyResourceReloaded(const ResourceHandle& currentResource, const ResourceHandle& newResource)
        {
            DEBUG_CHECK_RETURN_EX(currentResource, "Invalid current resource");
            DEBUG_CHECK_RETURN_EX(newResource, "Invalid target resource");

            auto lock = CreateLock(m_pendingReloadQueueLock);

            PendingReload info;
            info.currentResource = currentResource;
            info.newResource = newResource;
            m_pendingReloadQueue.push(info);
        }

        void ResourceLoaderCooker::applyReload(ResourcePtr currentResource, ResourcePtr newResource)
        {
            DEBUG_CHECK_RETURN_EX(currentResource, "Invalid current resource");
            DEBUG_CHECK_RETURN_EX(newResource, "Invalid target resource");

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

        ResourceHandle ResourceLoaderCooker::loadResourceOnce(const ResourceKey& key)
        {
            SpecificClassType<IResource> cookedResourceClass;

            // determine mount point of the resource
            ResourceMountPoint mountPoint;
            m_depot->queryFileMountPoint(key.path().view(), mountPoint);

            // most common case is to directly load serialized data, check that first
            const auto fileLoadClass = IResource::FindResourceClassByExtension(key.extension());
            if (fileLoadClass && fileLoadClass->is(key.cls()))
            {
                // load file content from serialized file
                if (auto file = m_depot->createFileAsyncReader(key.path()))
                {
                    FileLoadingContext context;
                    context.basePath = mountPoint.path();
                    context.resourceLoader = this;

                    if (LoadFile(file, context))
                    {
                        if (const auto ret = context.root<IResource>())
                        {
                            ret->bindToLoader(this, key, mountPoint, false);
                            
                            io::TimeStamp fileTimeStamp;
                            if (m_depot->queryFileTimestamp(key.path(), fileTimeStamp))
                            {
                                InplaceArray<SourceDependency, 1> dependencies;

                                auto& dep = dependencies.emplaceBack();
                                dep.sourcePath = StringBuf(key.path());
                                dep.timestamp = fileTimeStamp.value();

                                m_depTracker->notifyDependenciesChanged(key, dependencies);
                            }

                            return ret;
                        }
                    }
                }
            }

            // we are not loading serialized data directly, we must have a ad-hoc cooker that can cook the resource from the source files
            else if (m_cooker->canCook(key, cookedResourceClass))
            {
                if (auto cookedFile = m_cooker->cook(key))
                {
                    cookedFile->bindToLoader(this, key, mountPoint, false);

                    if (cookedFile->metadata())
                        m_depTracker->notifyDependenciesChanged(key, cookedFile->metadata()->sourceDependencies);

                    return cookedFile;
                }
            }

            /*// create stub
            if (fileLoadClass && fileLoadClass->is(key.cls()))
            {
                const auto& depotPath = key.path();
                auto stubRes = fileLoadClass->create<IResource>();
                stubRes->bindToLoader(this, key, mountPoint, true);
                updateEmptyFileDependencies(key, depotPath);
                return stubRes;
            }*/

            // loading failed with no stub as there's no hope of recovery
            return nullptr;
        }

        bool ResourceLoaderCooker::validateExistingResource(const ResourceHandle& res, const ResourceKey& key) const
        {
            // check stored dependencies
            if (res->metadata())
            {
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
            }

            // check additional dependencies
            if (!m_depTracker->checkUpToDate(key))
                return false;

            // still valid
            return true;
        }

        //---
        
    } // res
} // base


