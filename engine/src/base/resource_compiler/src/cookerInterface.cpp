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
#include "base/resource/include/resourceLoader.h"
#include "base/io/include/ioSystem.h"
#include "base/io/include/ioFileHandle.h"
#include "base/io/include/timestamp.h"
#include "base/resource_compiler/include/depotStructure.h"

namespace base
{
    namespace res
    {

        //--

        CookerInterface::CookerInterface(const depot::DepotStructure& depot, IResourceLoader* dependencyLoader, StringView referenceFilePath, const ResourceMountPoint& referenceMountingPoint, bool finalCooker, IProgressTracker* externalProgressTracker)
            : m_referencePath(referenceFilePath)
            , m_referenceMountingPoint(referenceMountingPoint)
            , m_externalProgressTracker(externalProgressTracker)
            , m_finalCooker(finalCooker)
            , m_depot(depot)
            , m_loader(dependencyLoader)
        {
            m_referenceMountingPoint.translatePathToRelative(m_referencePath.view(), m_referencePathBase);
            queryContextName(queryResourcePath(), m_referenceContextName);

            if (m_referenceContextName.empty())
                m_referenceContextName = queryResourcePath();
        }

        CookerInterface::~CookerInterface()
        {}

        bool CookerInterface::checkCancelation() const
        {
            if (m_externalProgressTracker)
                return m_externalProgressTracker->checkCancelation();
            return false;
        }

        void CookerInterface::reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text)
        {
            if (m_externalProgressTracker)
                m_externalProgressTracker->reportProgress(currentCount, totalCount, text);
        }

        const StringBuf& CookerInterface::queryResourcePath() const
        {
            return m_referencePath;
        }

        const StringBuf& CookerInterface::queryResourceContextName() const
        {
            return m_referenceContextName;
        }

        const ResourceMountPoint& CookerInterface::queryResourceMountPoint() const
        {
            return m_referenceMountingPoint;
        }       

        void CookerInterface::enumFiles(StringView systemPath, bool recurse, StringView extension, Array<StringBuf>& outFileSystemPaths, io::TimeStamp& outNewestTimeStamp)
        {
            ASSERT(systemPath.endsWith("/"));

            // directories to check
            InplaceArray<StringBuf, 32> directoriesToCheck;
            directoriesToCheck.pushBack(StringBuf(systemPath));

            // CRC hash of the file names
            CRC64 filePathsCRC;
            io::TimeStamp newestFileTimestamp;

            // split extensions
            InplaceArray<StringView, 20> extensions;
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
                    io::TimeStamp fileTimestamp;
                    auto fullPath = StringBuf(TempString("{}{}", dirPath, file));
                    if (m_depot.queryFileTimestamp(fullPath, fileTimestamp))
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
            outNewestTimeStamp = newestFileTimestamp;
        }

        bool CookerInterface::discoverResolvedPaths(StringView relativePath, bool recurse, StringView extension, Array<StringBuf>& outFileSystemPaths)
        {
            // not a directory :)
            if (!relativePath.endsWith("/"))
                return false;

            // get the system path to look at
            StringBuf systemRootPath;
            if (!ApplyRelativePath(m_referencePath, relativePath, systemRootPath))
                return false;

            // list files
            io::TimeStamp newsestFileTimestamp;
            enumFiles(systemRootPath, recurse, extension, outFileSystemPaths, newsestFileTimestamp);

            // list a dependency
            auto& dep = m_dependencies.emplaceBack();
            dep.timestamp = newsestFileTimestamp.value();
            if (extension)
                dep.sourcePath = StringBuf(TempString("{}{}*", systemRootPath, extension));
            else
                dep.sourcePath = systemRootPath;

            return true;
        }

        bool CookerInterface::queryResolvedPath(StringView relativePath, StringView contextFileSystemPath, bool isLocal, StringBuf& outResourcePath)
        {
            if (isLocal)
            {
                StringBuf ret;
                auto contextPathToUse = contextFileSystemPath.empty() ? m_referencePath : contextFileSystemPath;
                if (ApplyRelativePath(contextPathToUse, relativePath, ret))
                {
                    outResourcePath = ret;
                    return true;
                }
            }
            else
            {
                // find mount point for reference path
                ResourceMountPoint referenceMountPoint;
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

        bool CookerInterface::queryContextName(StringView fileSystemPath, StringBuf& contextName)
        {
            return m_depot.queryContextName(fileSystemPath, contextName);
        }

        bool CookerInterface::queryFileExists(StringView fileSystemPath) const
        {
            io::TimeStamp unused;
            return m_depot.queryFileTimestamp(fileSystemPath, unused);
        }

        bool CookerInterface::touchFile(StringView fileSystemPath)
        {
            for (auto& dep : m_dependencies)
                if (dep.sourcePath == fileSystemPath)
                    return dep.timestamp != 0;

            io::TimeStamp fileTimestamp;
            auto ret = m_depot.queryFileTimestamp(fileSystemPath, fileTimestamp);

            auto& entry = m_dependencies.emplaceBack();
            entry.timestamp = fileTimestamp.value();
            entry.sourcePath = StringBuf(fileSystemPath);

            if (ret)
                TRACE_SPAM("Discovered dependency for '{}' on '{}'", m_referencePath, fileSystemPath);

            return ret;
        }

        bool CookerInterface::finalCooker() const
        {
            return m_finalCooker;
        }

        bool CookerInterface::findFile(StringView contextPath, StringView inputPath, StringBuf& outFileSystemPath, uint32_t maxScanDepth /*= 2*/)
        {
            return ScanRelativePaths(contextPath, inputPath, maxScanDepth, outFileSystemPath, [this](StringView testPath)
                {
                    return touchFile(testPath);
                });
        }

        io::ReadFileHandlePtr CookerInterface::createReader(StringView fileSystemPath)
        {
            touchFile(fileSystemPath);
            return m_depot.createFileReader(fileSystemPath);
        }

        Buffer CookerInterface::loadToBuffer(StringView fileSystemPath)
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

        bool CookerInterface::loadToString(StringView fileSystemPath, StringBuf& outContent)
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

        ResourceHandle CookerInterface::loadDependencyResource(const ResourceKey& key)
        {
            ResourceHandle ret;

            TRACE_INFO("Discovered dependency on another cooked resource '{}'", key);

            if (!key.cls()->is<IResource>())
            {
                TRACE_ERROR("Dependency on a non-resource object '{}'", key);
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
