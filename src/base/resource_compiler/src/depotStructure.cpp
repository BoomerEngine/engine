/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: depot #]
***/

#include "build.h"
#include "depotFileSystem.h"
#include "depotFileSystemNative.h"
#include "depotStructure.h"

#include "base/resource/include/resourceMetadata.h"
#include "base/resource/include/resource.h"
#include "base/resource/include/resource.h"
#include "base/containers/include/stringBuilder.h"
#include "base/containers/include/inplaceArray.h"
#include "base/io/include/ioSystem.h"
#include "base/io/include/ioFileHandle.h"
#include "base/app/include/localServiceContainer.h"
#include "base/app/include/commandline.h"
#include "base/system/include/thread.h"
#include "base/xml/include/xmlDocument.h"
#include "base/xml/include/xmlUtils.h"
#include "base/io/include/pathBuilder.h"

namespace base
{
    namespace depot
    {

        //--

        namespace helper
        {

            static StringBuf CombineMountPaths(StringView basePath, StringView relativeMountPath)
            {
                ASSERT_EX(basePath.empty() || basePath.endsWith("/"), "Invalid base path");

                StringBuf mergedPath = TempString("{}{}", basePath, relativeMountPath);
                mergedPath.replaceChar('\\', '/');

                if (!mergedPath.endsWith("/"))
                    mergedPath = TempString("{}/", mergedPath);

                return mergedPath;
            }

        } // helper

           //--

        DepotFileSystem::DepotFileSystem(StringView mountPoint, UniquePtr<IFileSystem> fileSystem, DepotFileSystemType type)
            : m_mountPoint(mountPoint)
            , m_fileSystem(std::move(fileSystem))
            , m_type(type)
        {
        }

        DepotFileSystem::~DepotFileSystem()
        {
        }

        //--

        DepotStructure::DepotStructure()
        {
            m_eventKey = MakeUniqueEventKey("DepotStructure");
        }

        DepotStructure::~DepotStructure()
        {
            m_fileSystems.clear();
            m_fileSystemsPtrs.clear();
        }

        void DepotStructure::enableDepotObservers()
        {
            for (const auto& fs : m_fileSystems)
                fs->fileSystem().enableFileSystemObservers();
        }

        void DepotStructure::rebuildFileSystemMap()
        {
            // clear map
            m_fileSystemsAtDirectory.clear();

            // attach system
            for (auto fs  : m_fileSystemsPtrs)
                registerFileSystemBinding(fs);
        }

        void DepotStructure::registerFileSystemBinding(const DepotFileSystem* fs)
        {
            auto& mountPoint = fs->mountPoint();
            ASSERT(!mountPoint.empty());
            ASSERT(mountPoint.endsWith("/"));

            auto shorterPath = mountPoint.leftPart(mountPoint.length()-1);

            auto basePrefixPath = shorterPath.stringBeforeLast("/");
            if (!basePrefixPath.empty())
                basePrefixPath = TempString("{}/", basePrefixPath);

            auto mountDirectoryName = shorterPath.stringAfterLast("/", true);
            ASSERT(!mountDirectoryName.empty());

            auto& entry = m_fileSystemsAtDirectory[basePrefixPath].emplaceBack();
            entry.name = mountDirectoryName;
            entry.fileSystem = fs;
        }

        void DepotStructure::attachFileSystem(StringView mountPoint, UniquePtr<IFileSystem> fileSystem, DepotFileSystemType type)
        {
            // create wrapper
            auto depotWrapper = CreateUniquePtr<DepotFileSystem>(mountPoint, std::move(fileSystem), type);
            m_fileSystems.pushBack(std::move(depotWrapper));
            m_fileSystemsPtrs.pushBack(m_fileSystems.back().get());

            // get the base path
            registerFileSystemBinding(m_fileSystems.back().get());
        }

        void DepotStructure::detachProjectFileSystems()
        {
            // unmount project dependent file system
            m_fileSystemsPtrs.clear();
            auto fileSystems = std::move(m_fileSystems);
            for (auto& entry : fileSystems)
            {
                if (entry->type() == DepotFileSystemType::Engine)
                {
                    // keep alive
                    m_fileSystems.pushBack(std::move(entry));
                }
                else
                {
                    TRACE_INFO("Unmounting '{}'", entry->mountPoint());
                }
            }
        }

        io::ReadFileHandlePtr DepotStructure::createFileReader(StringView filePath) const
        {
            // resolve location
            StringBuf localFileSystemPath;
            const IFileSystem* localFileSystem = nullptr;
            if (!queryFilePlacement(filePath, localFileSystem, localFileSystemPath))
                return nullptr;

            // get the timestamp
            if (auto ret = localFileSystem->createReader(localFileSystemPath))
                return ret;

            // try the physical system
            if (!localFileSystem->isPhysical())
            {
                if (!queryFilePlacement(filePath, localFileSystem, localFileSystemPath, true))
                    return nullptr;

                // try with the target file system
                if (auto ret = localFileSystem->createReader(localFileSystemPath))
                    return ret;
            }

            return nullptr;
        }

        io::AsyncFileHandlePtr DepotStructure::createFileAsyncReader(StringView filePath) const
        {
            // resolve location
            StringBuf localFileSystemPath;
            const IFileSystem* localFileSystem = nullptr;
            if (!queryFilePlacement(filePath, localFileSystem, localFileSystemPath))
                return nullptr;

            // get the timestamp
            if (auto ret = localFileSystem->createAsyncReader(localFileSystemPath))
                return ret;

            // try the physical system
            if (!localFileSystem->isPhysical())
            {
                if (!queryFilePlacement(filePath, localFileSystem, localFileSystemPath, true))
                    return nullptr;

                // try with the target file system
                if (auto ret = localFileSystem->createAsyncReader(localFileSystemPath))
                    return ret;
            }

            return nullptr;
        }

        io::WriteFileHandlePtr DepotStructure::createFileWriter(StringView filePath) const
        {
            StringBuf localFileSystemPath;
            const IFileSystem* localFileSystem = nullptr;
            if (!queryFilePlacement(filePath, localFileSystem, localFileSystemPath))
                return false;

            // try with the target file system
            if (localFileSystem->isWritable())
                if (auto ret = localFileSystem->createWriter(localFileSystemPath))
                    return ret;

            // try the physical system
            if (!localFileSystem->isPhysical())
            {
                if (!queryFilePlacement(filePath, localFileSystem, localFileSystemPath, true))
                    return false;

                // try with the target file system
                if (localFileSystem->isWritable())
                    if (auto ret = localFileSystem->createWriter(localFileSystemPath))
                        return ret;
            }

            // not written
            return false;
        }

        ///---

        void DepotStructure::notifyFileChanged(IFileSystem* fs, StringView rawFilePath)
        {
            for (auto ptr  : m_fileSystemsPtrs)
            {
                if (&ptr->fileSystem() == fs)
                {
                    StringBuf fullPath = TempString("{}{}", ptr->mountPoint(), rawFilePath);

                    TRACE_INFO("File {} was reported as changed", fullPath);
                    DispatchGlobalEvent(m_eventKey, EVENT_DEPOT_FILE_CHANGED, fullPath);

                    break;
                }
            }
        }

        void DepotStructure::notifyFileAdded(IFileSystem* fs, StringView rawFilePath)
        {
            for (auto ptr : m_fileSystemsPtrs)
            {
                if (&ptr->fileSystem() == fs)
                {
                    StringBuf fullPath = TempString("{}{}", ptr->mountPoint(), rawFilePath);

                    TRACE_INFO("File {} was reported as added", fullPath);
                    DispatchGlobalEvent(m_eventKey, EVENT_DEPOT_FILE_ADDED, fullPath);

                    break;
                }
            }
        }

        void DepotStructure::notifyFileRemoved(IFileSystem* fs, StringView rawFilePath)
        {
            for (auto ptr  : m_fileSystemsPtrs)
            {
                if (&ptr->fileSystem() == fs)
                {
                    StringBuf fullPath = TempString("{}{}", ptr->mountPoint(), rawFilePath);

                    TRACE_INFO("File {} was reported as removed", fullPath);
                    DispatchGlobalEvent(m_eventKey, EVENT_DEPOT_FILE_REMOVED, fullPath);

                    break;
                }
            }
        }

        void DepotStructure::notifyDirAdded(IFileSystem* fs, StringView rawFilePath)
        {
            for (auto ptr  : m_fileSystemsPtrs)
            {
                if (&ptr->fileSystem() == fs)
                {
                    StringBuf fullPath = TempString("{}{}", ptr->mountPoint(), rawFilePath);

                    TRACE_INFO("Directory {} was reported as added", fullPath);
                    DispatchGlobalEvent(m_eventKey, EVENT_DEPOT_DIRECTORY_ADDED, fullPath);

                    break;
                }
            }
        }

        void DepotStructure::notifyDirRemoved(IFileSystem* fs, StringView rawFilePath)
        {
            for (auto ptr  : m_fileSystemsPtrs)
            {
                if (&ptr->fileSystem() == fs)
                {
                    StringBuf fullPath = TempString("{}{}", ptr->mountPoint(), rawFilePath);

                    TRACE_INFO("Directory {} was reported as removed", fullPath);
                    DispatchGlobalEvent(m_eventKey, EVENT_DEPOT_DIRECTORY_REMOVED, fullPath);

                    break;
                }
            }
        }

        //--

    } // depot
} // base
