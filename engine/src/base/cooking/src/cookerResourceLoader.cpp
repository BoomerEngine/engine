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

#include "base/depot/include/depotStructure.h"
#include "base/resources/include/resourceUncached.h"
#include "base/resources/include/resourceMetadata.h"
#include "base/resources/include/resourcePath.h"
#include "base/resources/include/resourceMountPoint.h"
#include "base/resources/include/resourceStaticResource.h"
#include "base/object/include/nativeFileReader.h"
#include "base/object/include/objectGlobalRegistry.h"
#include "base/io/include/ioSystem.h"


namespace base
{
     namespace cooker
    {
        ///--

        RTTI_BEGIN_TYPE_CLASS(ResourceLoaderCooker);
        RTTI_END_TYPE();
        
        ///--

        ResourceLoaderCooker::ResourceLoaderCooker()
        {
            // create depot
            m_depot.create();

            // create dependency tracker
            m_depTracker.create(*m_depot);
            m_depot->attachObserver(m_depTracker.get());

            // create cooker
            m_cooker.create(*m_depot, this);
        }

        ResourceLoaderCooker::~ResourceLoaderCooker()
        {
            m_cooker.reset();

            m_depot->detttachObserver(m_depTracker.get());
            m_depTracker.reset();

            m_depot.reset();
        }

        bool ResourceLoaderCooker::initialize(const app::CommandLine& cmdLine)
        {
            // look for "depot.xml"
            // TODO: change to project.xml
            auto startPath = IO::GetInstance().systemPath(io::PathCategory::ExecutableDir);
            while (!startPath.empty() && startPath.view() != L"/" && startPath.view() != L"\\")
            {
                const auto depotManifestPath = startPath.addFile("depot.xml");
                if (IO::GetInstance().fileExists(depotManifestPath))
                {
                    if (m_depot->populateFromManifest(depotManifestPath))
                        break;
                }
                startPath = startPath.parentPath();
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
                res::ResourcePtr currentResource, newResource;
                if (tryFinishReloading(currentResource, newResource))
                    applyReloading(currentResource, newResource);
            }
        }
        
        //--

        void ResourceLoaderCooker::checkChangedFiles()
        {
            base::Array<res::ResourceKey> changedFiles;
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
                        RefWeakPtr<res::IResource> loadedResourceWeak;
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

        res::ResourcePtr ResourceLoaderCooker::pickupNextResourceForReloading_NoLock()
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
                res::ResourceHandle loadedResource;
                {
                    RefWeakPtr<res::IResource> loadedResourceWeak;
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


        void ResourceLoaderCooker::processReloading(res::ResourceKey key)
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

        bool ResourceLoaderCooker::tryFinishReloading(res::ResourcePtr& outReloadedResource, res::ResourcePtr& outNewResource)
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

                m_resourceBeingReloadedKey = res::ResourceKey();
                m_reloadedResource.reset();
                m_resourceBeingReloaded.reset();
                m_reloadFinished = false;
                return true;
            }

            // reloading still going
            return false;
        }

        void ResourceLoaderCooker::applyReloading(res::ResourcePtr currentResource, res::ResourcePtr newResource)
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

                res::IStaticResource::ApplyReload(currentResource, newResource);

                TRACE_INFO("Reload to '{}' applied in {}, {} of {} objects pached", currentResource->key(), timer, affectedObjects.size(), numObjectsVisited);
            }    
        }

        void ResourceLoaderCooker::updateEmptyFileDependencies(res::ResourceKey key, base::StringView<char> depotPath)
        {
            InplaceArray<base::res::SourceDependency, 1> dependencies;

            auto& dep = dependencies.emplaceBack();
            dep.sourcePath = base::StringBuf(depotPath);

            m_depTracker->notifyDependenciesChanged(key, dependencies);
        }

        void ResourceLoaderCooker::updateDirectFileDependencies(res::ResourceKey key, base::StringView<char> depotPath)
        {
            uint64_t fileSize = 0;
            io::TimeStamp fileTimeStamp;
            if (m_depot->queryFileInfo(depotPath, nullptr, &fileSize, &fileTimeStamp))
            {
                InplaceArray<base::res::SourceDependency, 1> dependencies;

                auto& dep = dependencies.emplaceBack();
                dep.sourcePath = base::StringBuf(depotPath);
                dep.timestamp = fileTimeStamp.value();
                dep.size = fileSize;
                dep.crc = 0;

                m_depTracker->notifyDependenciesChanged(key, dependencies);
            }
        }

        res::ResourcePtr ResourceLoaderCooker::loadInternalDirectly(ClassType resClass, base::StringView<char> filePath)
        {
            // load file content
            if (auto content = m_depot->createFileReader(filePath))
            {
                stream::NativeFileReader fileReader(*content);
                if (auto data = res::LoadUncached(filePath, resClass, fileReader, this))
                {
                    if (!data->metadata())
                    {
                        uint64_t fileSize = 0;
                        io::TimeStamp fileTimeStamp;
                        if (m_depot->queryFileInfo(filePath, nullptr, &fileSize, &fileTimeStamp))
                        {
                            auto metadata = base::CreateSharedPtr<res::Metadata>();

                            auto& dep = metadata->sourceDependencies.emplaceBack();
                            dep.sourcePath = base::StringBuf(filePath);
                            dep.timestamp = fileTimeStamp.value();
                            dep.size = fileSize;
                            dep.crc = 0;

                            data->metadata(metadata);
                        }
                    }

                    return data;
                }
            }

            return nullptr;
        }

        static res::ResourcePtr CreateValidStub(ClassType resClass)
        {
            TRACE_INFO("Creating stub for '{}'", resClass);

            if (!resClass->isAbstract())
                return resClass->create<res::IResource>();

            // TODO: metadata

            base::InplaceArray<ClassType, 10> resourceClasses;
            RTTI::GetInstance().enumClasses(resClass, resourceClasses);
            DEBUG_CHECK_EX(!resourceClasses.empty(), "Resource class with no actual implementation");

            if (!resourceClasses.empty())
                return resourceClasses[0]->create<res::IResource>();

            return nullptr;
        }

        res::ResourcePtr ResourceLoaderCooker::loadInternal(res::ResourceKey key, bool normalLoading)
        {
            // determine mount point of the resource
            res::ResourceMountPoint mountPoint;
            m_depot->queryFileMountPoint(key.path().view(), mountPoint);

            // abstract class case
            /*if (key.cls()->isAbstract())
            {
                const auto fileExtension = key.path().extension();
                const auto resourceByExtension = base::res::IResource::FindResourceClassByExtension(fileExtension);
                if (resourceByExtension)
                    key = res::ResourceKey(key.path(), resourceByExtension);
            }*/

            // direct load 
            const auto fileExtension = key.path().extension();
            const auto loadExtension = res::IResource::GetResourceExtensionForClass(key.cls());
            if (fileExtension == loadExtension)
            {
                auto depotPath = key.path().path();
                if (const auto loadedRes = loadInternalDirectly(key.cls(), depotPath))
                {
                    loadedRes->bindToLoader(this, key, mountPoint, false);
                    updateDirectFileDependencies(key, depotPath);
                    return loadedRes;
                }
            }

            // file can't be directly loaded, can we cook it ?
            SpecificClassType<res::IResource> cookedResourceClass;
            if (m_cooker->canCook(key, cookedResourceClass))
            {
                // get the extension of the target (cooked) class
                const auto loadExtension = res::IResource::GetResourceExtensionForClass(cookedResourceClass);

                // some resource classes are only allowed to "slow bake" as the runtime cost of cooking them is to high
                if (cookedResourceClass->findMetadata<res::ResourceBakedOnlyMetadata>())
                {
                    DEBUG_CHECK_EX(loadExtension, "Baked resource should have an extension");
                    if (loadExtension)
                    {
                        base::StringBuilder path;
                        path << key.path().directory();
                        path << ".boomer/";
                        path << key.path().fileName();
                        path << "." << loadExtension;
                        auto bakedDepotPath = path.toString();

                        // do we have this file in the depot ? if so, load it
                        if (auto ret = loadInternalDirectly(key.cls(), bakedDepotPath))
                        {
                            ret->bindToLoader(this, key, mountPoint, false);
                            updateDirectFileDependencies(key, bakedDepotPath);
                            return ret;
                        }

                        // we are missing a baked file
                        if (normalLoading)
                        {
                            // notify that we are missing a baked file
                            const auto bakedResourceKey = res::ResourceKey(key.path(), cookedResourceClass);
                            notifyMissingBakedResource(bakedResourceKey);

                            // create a stub
                            if (auto stub = CreateValidStub(cookedResourceClass))
                            {
                                stub->bindToLoader(this, key, mountPoint, true);
                                updateEmptyFileDependencies(key, bakedDepotPath);
                                return stub;
                            }
                        }
                    }

                    TRACE_ERROR("Failed to handle baked resource '{}' stuff may break", key);
                    return nullptr;
                }

                // load using cooker
                if (auto cookedFile = m_cooker->cook(key))
                {
                    cookedFile->bindToLoader(this, key, mountPoint, false);

                    if (cookedFile->metadata())
                        m_depTracker->notifyDependenciesChanged(key, cookedFile->metadata()->sourceDependencies);

                    return cookedFile;
                }
            }

            // loading failed with no stub as there's no hope of recovery
            return nullptr;
        }

        res::ResourceHandle ResourceLoaderCooker::loadResourceOnce(const res::ResourceKey& key)
        {
            // if we are loading the resource remove it from the queue
            if (m_pendingReloadSet.remove(key))
            {
                TRACE_INFO("Resource '{}' was loaded again before reload queue was processed", key);
            }

            // load the resource
            return loadInternal(key);
        }

        bool ResourceLoaderCooker::validateExistingResource(const res::ResourceHandle& res, const res::ResourceKey& key) const
        {
            // this resource is valid until it's reloaded by the system
            if (const auto bakableOnly = res->cls()->findMetadata<res::ResourceBakedOnlyMetadata>())
                return true;

            // resource did not generate a metadata file
            if (!res->metadata() || res->metadata()->sourceDependencies.empty())
                return true;

            // check dependencies
            for (const auto& dep : res->metadata()->sourceDependencies)
            {
                uint64_t fileSize = 0;
                io::TimeStamp fileTimeStamp;
                m_depot->queryFileInfo(dep.sourcePath, nullptr, &fileSize, &fileTimeStamp);

                if (fileSize != dep.size || fileTimeStamp.value() != dep.timestamp)
                {
                    TRACE_WARNING("Dependency of file '{}' a file ({}) has changed. We will force resource to load a new version.", key, dep.sourcePath);
                    return false;
                }
            }

            // still valid
            return true;
        }

        //---
        
    } // cooker
} // base


