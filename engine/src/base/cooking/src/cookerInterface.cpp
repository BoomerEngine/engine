/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: cooking #]
***/

#include "build.h"

#include "cookerInterface.h"

#include "base/containers/include/stringBuilder.h"
#include "base/containers/include/inplaceArray.h"
#include "base/resources/include/resourceUncached.h"
#include "base/resources/include/resourceLoader.h"
#include "base/object/include/memoryReader.h"
#include "base/io/include/crcCache.h"
#include "base/io/include/ioSystem.h"
#include "base/io/include/ioFileHandle.h"
#include "base/io/include/timestamp.h"
#include "base/depot/include/depotStructure.h"

namespace base
{
    namespace cooker
    {

        //--

        CookerInterface::CookerInterface(const depot::DepotStructure& depot, res::IResourceLoader* dependencyLoader, const res::ResourcePath& referenceFilePath, const res::ResourceMountPoint& referenceMountingPoint, bool finalCooker, base::IProgressTracker* externalProgressTracker)
            : m_referencePath(referenceFilePath)
            , m_referenceMountingPoint(referenceMountingPoint)
            , m_externalProgressTracker(externalProgressTracker)
            , m_finalCooker(finalCooker)
            , m_depot(depot)
            , m_loader(dependencyLoader)
        {
            m_referenceMountingPoint.translatePathToRelative(m_referencePath.view(), m_referencePathBase);
            queryContextName(queryResourcePath().path(), m_referenceContextName);

            if (m_referenceContextName.empty())
                m_referenceContextName = StringBuf(queryResourcePath().path());
        }

        CookerInterface::~CookerInterface()
        {}

        bool CookerInterface::checkCancelation() const
        {
            if (m_externalProgressTracker)
                return m_externalProgressTracker->checkCancelation();
            return false;
        }

        void CookerInterface::reportProgress(uint64_t currentCount, uint64_t totalCount, StringView<char> text)
        {
            if (m_externalProgressTracker)
                m_externalProgressTracker->reportProgress(currentCount, totalCount, text);
        }

        const res::ResourcePath& CookerInterface::queryResourcePath() const
        {
            return m_referencePath;
        }

        const StringBuf& CookerInterface::queryResourceContextName() const
        {
            return m_referenceContextName;
        }

        const res::ResourceMountPoint& CookerInterface::queryResourceMountPoint() const
        {
            return m_referenceMountingPoint;
        }       

        void CookerInterface::enumFiles(StringView<char> systemPath, bool recurse, StringView<char> extension, Array<StringBuf>& outFileSystemPaths, uint64_t& outFileNamesHash, io::TimeStamp& outNewestTimeStamp)
        {
            ASSERT(systemPath.endsWith("/"));

            // directories to check
            InplaceArray<StringBuf, 32> directoriesToCheck;
            directoriesToCheck.pushBack(StringBuf(systemPath));

            // CRC hash of the file names
            CRC64 filePathsCRC;
            io::TimeStamp newestFileTimestamp;

            // split extensions
            InplaceArray<StringView<char>, 20> extensions;
            extension.slice(";", false, extensions);

            // check directories
            while (!directoriesToCheck.empty())
            {
                // get directory
                auto dirPath = directoriesToCheck.back();
                directoriesToCheck.popBack();

                // look at files in this directory
                InplaceArray<StringBuf, 200> files;
                m_depot.enumFilesAtPath(dirPath, [&files](const depot::DepotStructure::FileInfo& info)
                    {
                        files.emplaceBack(info.name);
                        return false;
                    });

                // make sure we do not depend on the order of files reported by the system
                std::sort(files.begin(), files.end());

                // capture files names
                for (auto& file : files)
                {
                    // filter files
                    if (!extensions.empty())
                    {
                        bool validExt = false;
                        auto fileExtensions = file.view().afterLast(".");
                        for (auto& ext : extensions)
                        {
                            if (0 == fileExtensions.caseCmp(ext))
                            {
                                validExt = true;
                                break;
                            }
                        }

                        // skip files that don't have matching extension
                        if (!validExt)
                            continue;
                    }

                    // just use the file name
                    filePathsCRC << file;

                    // get the timestamp of the file
                    uint64_t fileSize = 0;
                    io::TimeStamp fileTimestamp;
                    auto fullPath = StringBuf(TempString("{}{}", dirPath, file));
                    if (m_depot.queryFileInfo(fullPath, nullptr, &fileSize, &fileTimestamp))
                    {
                        outFileSystemPaths.pushBack(fullPath);
                        filePathsCRC << file;

                        // get file modification date
                        if (newestFileTimestamp.empty() || fileTimestamp > newestFileTimestamp)
                            newestFileTimestamp = fileTimestamp;
                    }
                }

                // if we want recursion look at the directories as well
                if (recurse)
                {
                    InplaceArray<StringBuf, 20> dirs;
                    m_depot.enumDirectoriesAtPath(dirPath, [&dirs](const depot::DepotStructure::DirectoryInfo& info)
                        {
                            dirs.emplaceBack(info.name);
                            return false;
                        });

                    // make sure we do not depend on the order of dirs reported by the system
                    std::sort(dirs.begin(), dirs.end());

                    // look up the directories
                    for (auto& dir : dirs)
                    {
                        // use the dir name as well for hashing
                        filePathsCRC << dir;

                        // recurse
                        auto fullDirPath = StringBuf(TempString("{}{}/", dirPath, dir));
                        directoriesToCheck.emplaceBack(fullDirPath);
                    }
                }
            }

            // return extra data, especially the hash of all discovered files and the newest of them
            outFileNamesHash = filePathsCRC.crc();
            outNewestTimeStamp = newestFileTimestamp;
        }

        bool CookerInterface::discoverResolvedPaths(StringView<char> relativePath, bool recurse, StringView<char> extension, Array<StringBuf>& outFileSystemPaths)
        {
            // not a directory :)
            if (!relativePath.endsWith("/"))
                return false;

            // get the system path to look at
            StringBuf systemRootPath;
            if (!base::res::ApplyRelativePath(m_referencePath.path(), relativePath, systemRootPath))
                return false;

            // list files
            uint64_t fileNamesCRC = 0;
            io::TimeStamp newsestFileTimestamp;
            enumFiles(systemRootPath, recurse, extension, outFileSystemPaths, fileNamesCRC, newsestFileTimestamp);

            // list a dependency
            auto& dep = m_dependencies.emplaceBack();
            dep.size = fileNamesCRC;
            dep.timestamp = newsestFileTimestamp.value();
            if (extension)
                dep.sourcePath = StringBuf(TempString("{}{}*", systemRootPath, extension));
            else
                dep.sourcePath = systemRootPath;

            return true;
        }

        bool CookerInterface::queryResolvedPath(StringView<char> relativePath, StringView<char> contextFileSystemPath, bool isLocal, StringBuf& outResourcePath)
        {
            if (isLocal)
            {
                StringBuf ret;
                auto contextPathToUse = contextFileSystemPath.empty() ? m_referencePath.path() : contextFileSystemPath;
                if (base::res::ApplyRelativePath(contextPathToUse, relativePath, ret))
                {
                    outResourcePath = ret;
                    return true;
                }
            }
            else
            {
                // find mount point for reference path
                base::res::ResourceMountPoint referenceMountPoint;
                if (m_depot.queryFileMountPoint(contextFileSystemPath, referenceMountPoint))
                {
                    StringBuf ret;
                    referenceMountPoint.expandPathFromRelative(relativePath, ret);
                    outResourcePath = ret;
                    return true;
                }
            }

            return false;
        }

        bool CookerInterface::queryContextName(StringView<char> fileSystemPath, StringBuf& contextName)
        {
            return m_depot.queryContextName(fileSystemPath, contextName);
        }

        bool CookerInterface::touchFile(StringView<char> fileSystemPath)
        {
            for (auto& dep : m_dependencies)
                if (dep.sourcePath == fileSystemPath)
                    return dep.size != 0;

            uint64_t fileSize = 0;
            io::TimeStamp fileTimestamp;
            auto ret = m_depot.queryFileInfo(fileSystemPath, nullptr, &fileSize, &fileTimestamp);

            auto& entry = m_dependencies.emplaceBack();
            entry.size = fileSize;
            entry.timestamp = fileTimestamp.value();
            entry.sourcePath = StringBuf(fileSystemPath);

            if (ret)
                TRACE_INFO("Discovered dependency for '{}' on '{}'", m_referencePath, fileSystemPath);

            return ret;
        }

        bool CookerInterface::queryFileInfo(StringView<char> fileSystemPath, uint64_t* outCRC, uint64_t* outSize, io::TimeStamp* outTimeStamp, bool makeDependency /*= true*/)
        {
            auto ret = m_depot.queryFileInfo(fileSystemPath, outCRC, outSize, outTimeStamp);

            if (makeDependency)
                touchFile(fileSystemPath);

            return ret;
        }

        bool CookerInterface::finalCooker() const
        {
            return m_finalCooker;
        }

        bool CookerInterface::findFile(StringView<char> contextPath, StringView<char> inputPath, StringBuf& outFileSystemPath, uint32_t maxScanDepth /*= 2*/)
        {
            // slice the input path
            base::InplaceArray<base::StringView<char>, 20> inputParts;
            inputPath.slice("\\/", false, inputParts);
            if (inputParts.empty())
                return false;

            // get current path
            base::InplaceArray<base::StringView<char>, 20> referenceParts;
            contextPath.slice("\\/", false, referenceParts);
            if (referenceParts.empty())
                return false;

            // remove the file name of the reference path
            referenceParts.popBack();

            // outer search (on the reference path)
            for (uint32_t i=0; i<maxScanDepth; ++i)
            {
                // try all allowed combinations of reference path as well
                auto innerSearchDepth = std::min<uint32_t>(maxScanDepth, inputParts.size());
                for (uint32_t j=0; j<innerSearchDepth; ++j)
                {
                    StringBuilder pathBuilder;

                    for (auto& str : referenceParts)
                    {
                        if (!pathBuilder.empty()) pathBuilder << "/";
                        pathBuilder << str;
                    }

                    auto firstInputPart = inputParts.size() - j - 1;
                    for (uint32_t k=firstInputPart; k<inputParts.size(); ++k)
                    {
                        if (!pathBuilder.empty()) pathBuilder << "/";
                        pathBuilder << inputParts[k];
                    }

                    // does the file exist ?
                    auto fileSystemPath = pathBuilder.toString();
                    if (touchFile(fileSystemPath))
                    {
                        outFileSystemPath = fileSystemPath;
                        return true;
                    }
                }

                // ok, we didn't found anything, retry with less base directories
                if (referenceParts.empty())
                    break;
                referenceParts.popBack();
            }

            // no matching file found
            return false;
        }

        io::FileHandlePtr CookerInterface::createReader(StringView<char> fileSystemPath)
        {
            touchFile(fileSystemPath);
            return m_depot.createFileReader(fileSystemPath);
        }

        Buffer CookerInterface::loadToBuffer(StringView<char> fileSystemPath)
        {
            auto loader = createReader(fileSystemPath);
            if (!loader)
                return nullptr;

            auto size = loader->size();
			if (size >= INDEX_MAX)
				return nullptr;

            if (auto buffer = Buffer::Create(POOL_TEMP, size)) // allocation can fail
                if (size == loader->readSync(buffer.data(), buffer.size())) // reading can fail
                    return buffer;

            return nullptr;
        }

        bool CookerInterface::loadToString(StringView<char> fileSystemPath, StringBuf& outContent)
        {
            auto loader = createReader(fileSystemPath);
            if (!loader)
                return false;

            auto size = loader->size();
            if (size >= UINT32_MAX)
                return false;

            auto sizeUint32 = range_cast<uint32_t>(size);

            Array<uint8_t> buffer;
            buffer.resize(sizeUint32);

            if (sizeUint32 != loader->readSync(buffer.data(), sizeUint32))
                return false;

            outContent = StringBuf(buffer.data(), buffer.dataSize());
            return true;
        }

        res::ResourceHandle CookerInterface::loadUncachedFile(StringView<char> fileSystemPath, ClassType expectedClassType)
        {
            // load content
            auto fileContent = loadToBuffer(fileSystemPath);
            if (!fileContent)
                return nullptr;

            // deserialize
            TRACE_INFO("Loading uncached resource '{}' as '{}'", fileSystemPath, expectedClassType->name());;
            stream::MemoryReader reader(fileContent.data(), fileContent.size());
            return res::LoadUncached(fileSystemPath, expectedClassType, reader);
        }

        res::ResourceHandle CookerInterface::loadManifestFile(StringView<char> outputPartName, ClassType expectedManifestClass)
        {
            ASSERT(expectedManifestClass && expectedManifestClass->is(res::IResourceManifest::GetStaticClass()));

            // look for the manifest extension in the manifest class
            auto extensionMetadata  = expectedManifestClass->findMetadata<res::ResourceManifestExtensionMetadata>();
            ASSERT(extensionMetadata != nullptr);

            // format a file path
            auto dirName = queryResourcePath().directory();
            auto fileName = queryResourcePath().fileName();
            auto manifestPath = outputPartName
                ? StringBuf(TempString("{}/.boomer/{}_{}.{}", dirName, fileName, outputPartName, extensionMetadata->extension()))
                : StringBuf(TempString("{}/.boomer/{}.{}", dirName, fileName, extensionMetadata->extension()));

            // load existing content
            if (auto ret = loadUncachedFile(manifestPath, expectedManifestClass))
                return ret;

            // create empty one
            return expectedManifestClass->create<res::IResourceManifest>();
        }

        res::ResourceHandle CookerInterface::loadDependencyResource(const res::ResourceKey& key)
        {
            res::ResourceHandle ret;

            TRACE_INFO("Discovered dependency on another cooked resource '{}'", key);

            if (!key.cls()->is<res::IResource>())
            {
                TRACE_ERROR("Dependency on a non-resource object '{}'", key);
                return nullptr;
            }

            if (key.cls()->findMetadata<res::ResourceBakedOnlyMetadata>())
            {
                TRACE_ERROR("Dependency on a baked resource '{}' is not allowed", key);
                return nullptr;
            }

            if (m_loader)
            {
                if (ret = m_loader->loadResource(key))
                {
                    if (auto metadata = ret->metadata())
                    {
                        TRACE_INFO("Discovered {} dependencie(s) in the dependant resource '{}'", metadata->sourceDependencies.size(), key);

                        for (const auto& dep : metadata->sourceDependencies)
                            touchFile(dep.sourcePath);
                   }
                }
            }

            return ret;
        }

        //--

    } // depot
} // base
