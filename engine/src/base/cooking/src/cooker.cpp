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
 
#include "base/depot/include/depotStructure.h"
#include "base/resources/include/resource.h"
#include "base/resources/include/resourceUncached.h"
#include "base/object/include/nativeFileReader.h"
#include "base/object/include/memoryReader.h"
#include "base/io/include/ioSystem.h"
#include "base/resources/include/resource.h"

namespace base
{
    namespace cooker
    {

        //--

        Cooker::Cooker(depot::DepotStructure& fileLoader, res::IResourceLoader* dependencyLoader, IProgressTracker* externalProgressTracker, bool finalCooker)
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
            Array<SpecificClassType<res::IResourceCooker>> cookerClasses;
            RTTI::GetInstance().enumClasses(cookerClasses);

            // list of extensions usable to cook given class
            HashMap<ClassType, Array<StringBuf>> classSourceExtensionsMap;

            // process the classes
            Array<SpecificClassType<res::IResourceCooker>> dependencCookers;
            for (auto manifestClass  : cookerClasses)
            {
                // get the resource extension this cooker expects on the input
                auto extensionMetadata  = manifestClass->findMetadata<res::ResourceSourceFormatMetadata>();
                if (!extensionMetadata)
                {
                    TRACE_WARNING("Cooker '{}' has no source class specified, nothing will be cooked by it", manifestClass->name());
                    continue;
                }

                // get the class we cook into
                auto targetClassMetadata  = manifestClass->findMetadata<res::ResourceCookedClassMetadata>();
                if (!targetClassMetadata || targetClassMetadata->classList().empty())
                {
                    TRACE_WARNING("Cooker '{}' has no output class specified, nothing will be cooked by it", manifestClass->name());
                    continue;
                }

                // if cooker has no extensions specified it may be cookable from another resource
                if (extensionMetadata->extensions().empty())
                {
                    TRACE_SPAM("Cooker '{}' is a dependant cooker, processing later", manifestClass->name());
                    dependencCookers.pushBack(manifestClass);
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

            // process the cookers that consume already cooked resources not the raw crap
            for (auto manifestClass  : dependencCookers)
            {
                auto extensionMetadata  = manifestClass->findMetadata<res::ResourceSourceFormatMetadata>();
                auto targetClassMetadata  = manifestClass->findMetadata<res::ResourceCookedClassMetadata>();

                // look at each source class we can be cooked from
                for (auto sourceClass  : extensionMetadata->classes())
                {
                    // get all extensions this class can be cooked from
                    auto sourceExtensionsTable = classSourceExtensionsMap[sourceClass];

                    // if the source class is loadable directly use the source extension as well
                    if (const auto* sourceExtensionMetaData = sourceClass->findMetadata<base::res::ResourceExtensionMetadata>())
                        sourceExtensionsTable.pushBackUnique(base::StringBuf(sourceExtensionMetaData->extension()));

                    // create a "trans cooking" entry 
                    for (auto& extension : sourceExtensionsTable)
                    {
                        auto& extensionTable = m_cookableClassmap[extension];

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
                                entry.order = 1;
                                TRACE_SPAM("Found dependent cooking recipe to cook '{}' from extension '{}' using '{}'",
                                    targetClass->name().c_str(), extension, manifestClass->name().c_str());
                            }
                        }
                    }
                }
            }

            // get text resource classes
            Array<SpecificClassType<res::ITextResource>> textResourceClasses;
            RTTI::GetInstance().enumClasses(textResourceClasses);
            TRACE_SPAM("Found {} text based resource classes", textResourceClasses.size());

            // look for text resources that have specific extension set, for those we can load them and save them in binary format
            m_selfCookableClasses.reserve(textResourceClasses.size());
            for (auto cls  : textResourceClasses)
            {
                // do we have an file extension specified ?
                if (auto extensionMetadata  = static_cast<const res::ResourceExtensionMetadata*>(cls->MetadataContainer::metadata(res::ResourceExtensionMetadata::GetStaticClass())))
                {
                    TRACE_SPAM("Found direct cooking recipe to cook '{}' from text '{}'", cls->name(), extensionMetadata->extension());
                    m_selfCookableClasses[extensionMetadata->extension()] = cls;
                }
                else if (!cls->is<res::IResourceManifest>())
                {
                    TRACE_WARNING("Text resoure class '{}' has no file extension specified and will not be cookable to engine format", cls->name());
                }
            }
        }

        bool Cooker::findBestCooker(const res::ResourceKey& key, CookableClass& outBestCooker) const
        {
            // get mount point for the resource path
            res::ResourceMountPoint mountPoint;
            if (!m_depot.queryFileMountPoint(key.path().path(), mountPoint))
                return false;

            // get the extension of the path we want to load from
            auto pathExtension = key.path().extension();
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

            // can we self-cook ?
            if (bestCooker.order == 255)
            {
                SpecificClassType<res::IResource> resourceClassType;
                if (m_selfCookableClasses.find(pathExtension, resourceClassType))
                {
                    if (resourceClassType.is(key.cls()))
                    {
                        bestCooker.cookerClass = nullptr; // no cooking
                        bestCooker.targetResourceClass = resourceClassType; // we cook to the same class
                        bestCooker.order = 2;
                    }
                }
            }

            /*// no cooker found yet
            if (bestCooker.order == 255)
            {
                // get the class of the resource we actually point to
                if (const auto actualSourceResourceClass = base::res::IResource::FindResourceClassByExtension(pathExtension))
                {
                    // look for 
                }
            }*/

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

        bool Cooker::canCook(const res::ResourceKey& key, SpecificClassType<res::IResource>& outCookedResourceClass) const
        {
            CookableClass info;
            if (!findBestCooker(key, info))
                return false;

            // TODO: additional checks ?

            outCookedResourceClass = info.targetResourceClass;
            return true;
        }

        static bool SelfCookingResource(res::ResourceKey key)
        {
            const auto loadExtension = base::res::IResource::GetResourceExtensionForClass(key.cls());
            const auto fileExtension = key.path().extension();
            return loadExtension == fileExtension;
        }

        res::ResourcePtr Cooker::cook(res::ResourceKey key) const
        {
            PC_SCOPE_LVL0(CookResource);
            ASSERT_EX(key, "Invalid resource key");

            // get mount point for the resource path
            res::ResourceMountPoint mountPoint;
            if (m_depot.queryFileMountPoint(key.path().path(), mountPoint))
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

        res::ResourcePtr Cooker::cookUsingCooker(res::ResourceKey key, const res::ResourceMountPoint& mountPoint, const CookableClass& recipe) const
        {
            ASSERT(recipe.cookerClass != nullptr);
            ASSERT(recipe.targetResourceClass != nullptr);

            // TODO: create a separate cooking log for each resource
            //auto cookingLogPath = cookedFilePath.addExtension(".clog");
            //CookingLogStream cookingLog(cookingLogPath);

            CookerInterface helperInterface(m_depot, m_loader, key.path(), mountPoint, m_finalCooker, m_externalProgressTracker);

            // cook the resource
            auto cooker = recipe.cookerClass->create<res::IResourceCooker>();
            if (auto cookedResource = cooker->cook(helperInterface))
            {
                auto metadata = base::CreateSharedPtr<res::Metadata>();
                metadata->sourceDependencies = helperInterface.generatedDependencies();
                //metadata->blackboard = helperInterface.generatedBlackboard();
                metadata->cookerClassVersion = recipe.cookerClass->findMetadataRef<base::res::ResourceCookerVersionMetadata>().version();
                metadata->cookerClass = recipe.cookerClass;
                metadata->resourceClassVersion = cookedResource->cls()->findMetadataRef<base::res::ResourceDataVersionMetadata>().version();
                cookedResource->metadata(metadata);

                return cookedResource;
            }

            return nullptr;
        }

        /*Cooker::Result Cooker::cookFromTextFormat(res::ResourceKey key, const res::ResourceMountPoint& mountPoint) const
        {
            const auto filePath = key.path().view();

            uint64_t fileSize = 0;
            io::TimeStamp fileTimestamp;
            if (!m_depot.queryFileInfo(filePath, nullptr, &fileSize, &fileTimestamp))
                return Cooker::Result();

            if (auto content = m_depot.createFileReader(filePath))
            {
                stream::NativeFileReader fileReader(*content);

                if (auto data = res::LoadUncached(key.path().view(), key.cls(), fileReader, m_loader))
                {
                    Cooker::Result result;
                    result.data = data;
                    result.metadata = base::CreateSharedPtr<res::Metadata>();

                    auto& mainDep = result.metadata->sourceDependencies.emplaceBack();
                    mainDep.size = fileReader.size();
                    mainDep.sourcePath = base::StringBuf(key.path());
                    mainDep.timestamp = fileTimestamp.value();

                    return result;
                }
            }

            return Cooker::Result();
        }*/

        //--

    } // cooker
} // base
