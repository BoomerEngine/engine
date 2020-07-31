/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: cooking #]
***/

#include "build.h"
#include "cooker.h"
#include "cookerInterface.h"
 
#include "base/resource_compiler/include/depotStructure.h"
#include "base/resource/include/resource.h"
#include "base/io/include/ioSystem.h"
#include "base/resource/include/resource.h"
#include "base/resource/include/resourceTags.h"

namespace base
{
    namespace res
    {

        //--

        Cooker::Cooker(depot::DepotStructure& fileLoader, IResourceLoader* dependencyLoader, IProgressTracker* externalProgressTracker, bool finalCooker)
            : m_depot(fileLoader)
            , m_loader(dependencyLoader)
            , m_finalCooker(finalCooker)
            , m_externalProgressTracker(externalProgressTracker)
        {
            buildClassMap();
        }

        Cooker::~Cooker()
        {}

        void Cooker::buildClassMap()
        {
            // explore the cooker classes
            Array<SpecificClassType<IResourceCooker>> cookerClasses;
            RTTI::GetInstance().enumClasses(cookerClasses);

            // list of extensions usable to cook given class
            HashMap<ClassType, Array<StringBuf>> classSourceExtensionsMap;

            // process the classes
            for (auto manifestClass  : cookerClasses)
            {
                // get the resource extension this cooker expects on the input
                auto extensionMetadata  = manifestClass->findMetadata<ResourceSourceFormatMetadata>();
                if (!extensionMetadata)
                {
                    TRACE_WARNING("Cooker '{}' has no source class specified, nothing will be cooked by it", manifestClass->name());
                    continue;
                }

                // get the class we cook into
                auto targetClassMetadata  = manifestClass->findMetadata<ResourceCookedClassMetadata>();
                if (!targetClassMetadata || targetClassMetadata->classList().empty())
                {
                    TRACE_WARNING("Cooker '{}' has no output class specified, nothing will be cooked by it", manifestClass->name());
                    continue;
                }

                // if cooker has no extensions specified it may be cookable from another resource
                if (extensionMetadata->extensions().empty())
                {
                    TRACE_SPAM("Cooker '{}' has no source extensions specified, it won't be able to cook any file", manifestClass->name());
                    continue;
                }

                // get table for given extension
                for (auto extension  : extensionMetadata->extensions())
                {
                    auto extensionStr = StringBuf(extension);
                    auto& extensionTable = m_cookableClassmap[extensionStr];

                    // process all target classes
                    for (auto targetClass  : targetClassMetadata->classList())
                    {
                        // make sure we don't have a duplicated entry
                        bool entryExists = false;
                        for (auto& entry : extensionTable)
                        {
                            if (entry.targetResourceClass == targetClass)
                            {
                                TRACE_ERROR("Cooking conflict, the '{}' is cookable from '{}' as well as '{}'",
                                    targetClass->name().c_str(),
                                    entry.cookerClass->name().c_str(),
                                    manifestClass->name().c_str());
                                entryExists = true;
                                break;
                            }
                        }

                        // add new entry
                        if (!entryExists)
                        {
                            auto& entry = extensionTable.emplaceBack();
                            entry.cookerClass = manifestClass;
                            entry.targetResourceClass = targetClass;
                            entry.order = 0;
                            TRACE_SPAM("Found native cooking recipe to cook '{}' from extension '{}' using '{}'",
                                targetClass->name().c_str(), extension, manifestClass->name().c_str());
                        }

                        // report
                        classSourceExtensionsMap[targetClass].pushBackUnique(extensionStr);
                    }
                }
            }
        }

        bool Cooker::findBestCooker(const ResourceKey& key, CookableClass& outBestCooker) const
        {
            // get mount point for the resource path
            ResourceMountPoint mountPoint;
            if (!m_depot.queryFileMountPoint(key.path(), mountPoint))
                return false;

            // get the extension of the path we want to load from
            auto pathExtension = key.extension();
            if (pathExtension.empty())
                return false;

            // look for a cooker
            CookableClass bestCooker;
            if (auto cookingClassMapping = m_cookableClassmap.find(pathExtension))
            {
                // target class must be supported (ie. we can't cook a skeleton from a jpg..)
                for (auto& classEntry : *cookingClassMapping)
                {
                    if (classEntry.targetResourceClass == key.cls())
                    {
                        if (classEntry.order < bestCooker.order)
                            bestCooker = classEntry;
                    }
                }

                // if we still don't have a cooker, relax check a little bit
                if (!bestCooker.cookerClass)
                {
                    for (auto& classEntry : *cookingClassMapping)
                    {
                        if (classEntry.targetResourceClass->is(key.cls())) // allows to cook ImageTexture when a ITexture is requested
                        {
                            if (classEntry.order < bestCooker.order)
                                bestCooker = classEntry;
                        }
                    }
                }
            }

            // if we still haven't found a base cooker try the "*"
            if (auto cookingClassMapping = m_cookableClassmap.find("*"))
            {
                // target class must be supported (ie. we can't cook a skeleton from a jpg..)
                for (auto& classEntry : *cookingClassMapping)
                {
                    if (classEntry.targetResourceClass == key.cls())
                    {
                        if (classEntry.order < bestCooker.order)
                            bestCooker = classEntry;
                    }
                }

                // if we still don't have a cooker, relax check a little bit
                if (!bestCooker.cookerClass)
                {
                    for (auto& classEntry : *cookingClassMapping)
                    {
                        if (classEntry.targetResourceClass->is(key.cls())) // allows to cook ImageTexture when a ITexture is requested
                        {
                            if (classEntry.order < bestCooker.order)
                                bestCooker = classEntry;
                        }
                    }
                }
            }

            // do we have anything
            TRACE_SPAM("Best cooker for '{}' to '{}' found to be '{}'", pathExtension, key.cls(), bestCooker.cookerClass);
            if (bestCooker.targetResourceClass)
            {
                outBestCooker = bestCooker;
                return true;
            }

            // no cooker found
            return false;
        }

        bool Cooker::canCook(const ResourceKey& key, SpecificClassType<IResource>& outCookedResourceClass) const
        {
            CookableClass info;
            if (!findBestCooker(key, info))
                return false;

            // TODO: additional checks ?

            outCookedResourceClass = info.targetResourceClass;
            return true;
        }

        static bool SelfCookingResource(ResourceKey key)
        {
            const auto loadExtension = IResource::GetResourceExtensionForClass(key.cls());
            const auto fileExtension = key.extension();
            return loadExtension == fileExtension;
        }

        ResourcePtr Cooker::cook(ResourceKey key) const
        {
            PC_SCOPE_LVL0(CookResource);
            ASSERT_EX(key, "Invalid resource key");

            // get mount point for the resource path
            ResourceMountPoint mountPoint;
            if (m_depot.queryFileMountPoint(key.path(), mountPoint))
            {
                // find best cooker for the job
                CookableClass info;
                if (findBestCooker(key, info) && info.cookerClass)
                {
                    return cookUsingCooker(key, mountPoint, info);
                }
                else  if (SelfCookingResource(key))
                {
                    return m_loader->loadResource(key);
                }
                else
                {
                    TRACE_ERROR("No cooker found for resource '{}'", key);
                    return nullptr;
                }
            }
            else
            {
                TRACE_ERROR("No mount point found for resource '{}', is the path inside the depot?", key);
                return nullptr;
            }
        }

        ResourcePtr Cooker::cookUsingCooker(ResourceKey key, const ResourceMountPoint& mountPoint, const CookableClass& recipe) const
        {
            ASSERT(recipe.cookerClass != nullptr);
            ASSERT(recipe.targetResourceClass != nullptr);

            CookerInterface helperInterface(m_depot, m_loader, key.path(), mountPoint, m_finalCooker, m_externalProgressTracker);

            // cook the resource
            auto cooker = recipe.cookerClass->create<IResourceCooker>();
            if (auto cookedResource = cooker->cook(helperInterface))
            {
                auto metadata = CreateSharedPtr<Metadata>();
                metadata->sourceDependencies = helperInterface.generatedDependencies();
                //metadata->blackboard = helperInterface.generatedBlackboard();
                metadata->cookerClassVersion = recipe.cookerClass->findMetadataRef<ResourceCookerVersionMetadata>().version();
                metadata->cookerClass = recipe.cookerClass;
                metadata->resourceClassVersion = cookedResource->cls()->findMetadataRef<ResourceDataVersionMetadata>().version();
                cookedResource->metadata(metadata);

                return cookedResource;
            }

            return nullptr;
        }

        //--

    } // res
} // base
