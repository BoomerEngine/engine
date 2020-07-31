/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
***/

#include "build.h"
#include "hl2FileSystem.h"
#include "hl2FileSystemIndex.h"

#include "base/io/include/ioSystem.h"
#include "base/io/include/ioFileHandle.h"
#include "base/io/include/ioFileHandleMemory.h"

namespace hl2
{

    PackedFileSystem::PackedFileSystem(const base::io::AbsolutePath& rootPath, base::UniquePtr<FileSystemIndex>&& indexData)
        : m_indexData(std::move(indexData))
        , m_rootPath(rootPath)
    {
        setupPackages();
    }

    PackedFileSystem::~PackedFileSystem()
    {}

    void PackedFileSystem::setupPackages()
    {
        auto numPackages = m_indexData->numPackages();
        m_openedPackages.reserve(numPackages);

        // open packages
        for (uint32_t i=0; i<numPackages; ++i)
        {
            auto& srcPackage = m_indexData->packages()[i];
            auto srcPackageName  = m_indexData->stringTable() + srcPackage.m_name;

            // create file path
            auto& entry = m_openedPackages.emplaceBack();
            entry.m_fullPath = m_rootPath.addFile(srcPackageName);
            entry.m_fileHandle = base::io::OpenForReading(entry.m_fullPath);
            if (!entry.m_fileHandle)
            {
                TRACE_WARNING("Unable to open package '{}', some files will not load", entry.m_fullPath);
            }
        }
    }

    bool PackedFileSystem::isPhysical() const
    {
        return false;
    }

    bool PackedFileSystem::isWritable() const
    {
        return false;
    }

    bool PackedFileSystem::ownsFile(StringView<char> rawFilePath) const
    {
        auto fileIndex = m_indexData->findFileEntry(rawFilePath);
        return fileIndex != -1;
    }

    bool PackedFileSystem::contextName(StringView<char> rawFilePath, base::StringBuf& outContextName) const
    {
        outContextName = base::TempString("HL2:{}", rawFilePath);
        return true;
    }

    bool PackedFileSystem::timestamp(StringView<char> rawFilePath, base::io::TimeStamp& outTimestamp) const
    {
        auto fileIndex = m_indexData->findFileEntry(rawFilePath);
        if (fileIndex == -1)
            return false;

        const auto& entry = m_indexData->files() + fileIndex;
        const auto& packageEntry = m_indexData->packages() + entry->m_packageIndex;
        outTimestamp = base::io::TimeStamp(packageEntry->m_timeStamp);
        return true;
    }
    
    bool PackedFileSystem::absolutePath(StringView<char> rawFilePath, base::io::AbsolutePath& outAbsolutePath) const
    {
        return false;
    }

    bool PackedFileSystem::enumDirectoriesAtPath(StringView<char> rawDirectoryPath, const std::function<bool(StringView<char>)>& enumFunc) const
    {
        ASSERT(rawDirectoryPath.empty() || rawDirectoryPath.endsWith("/"));

        auto dirIndex = m_indexData->findDirectoryEntry(rawDirectoryPath);
        if (dirIndex == -1)
            return false;

        const auto& dirEntry = m_indexData->directories() + dirIndex;
        auto childDirIndex = dirEntry->m_firstDir;
        while (childDirIndex != -1)
        {
            const auto& childDirEntry = m_indexData->directories() + childDirIndex;
            auto childDirName  = m_indexData->stringTable() + childDirEntry->m_name;
            if (enumFunc(childDirName))
                return true;
            childDirIndex = childDirEntry->m_nextDir;
        }

        return false;
    }

    bool PackedFileSystem::enumFilesAtPath(StringView<char> rawDirectoryPath, const std::function<bool(StringView<char>)>& enumFunc) const
    {
        ASSERT(rawDirectoryPath.empty() || rawDirectoryPath.endsWith("/"));

        auto dirIndex = m_indexData->findDirectoryEntry(rawDirectoryPath);
        if (dirIndex == -1)
            return false;

        const auto& dirEntry = m_indexData->directories() + dirIndex;
        auto fileIndex = dirEntry->m_firstFile;
        while (fileIndex != -1)
        {
            const auto& fileEntry = m_indexData->files() + fileIndex;
            auto fileName  = m_indexData->stringTable() + fileEntry->m_name;
            if (enumFunc(fileName))
                return true;

            fileIndex = fileEntry->m_nextFile;
        }

        return false;
    }

    /*base::io::FileHandlePtr PackedFileSystem::createReader(base::StringView<char> rawFilePath) const
    {
        // find file
        auto fileIndex = m_indexData->findFileEntry(rawFilePath);
        if (fileIndex == -1)
        {
            TRACE_ERROR("File '{}' is not in any archive", rawFilePath);
            return nullptr;
        }

        // get entry data
        const auto& entry = m_indexData->files() + fileIndex;
        auto& openedPackage = m_openedPackages[entry->m_packageIndex];
        if (!openedPackage.m_fileHandle)
        {
            TRACE_ERROR("File '{}' can't be loaded because archive is missing", rawFilePath);
            return nullptr;
        }

        // allocate memory buffer
        auto buffer = base::Buffer::Create(POOL_TEMP, entry->m_dataSize);

        // load file content
        {
            auto lock = base::CreateLock(openedPackage.m_lock);
            openedPackage.m_fileHandle->pos(entry->m_dataOffset);
            if (entry->m_dataSize != openedPackage.m_fileHandle->readSync(buffer.data(), entry->m_dataSize))
            {
                TRACE_ERROR("File '{}' can't be loaded because data can't be loaded from archive", rawFilePath);
                return nullptr;
            }
        }

        // create wrapper
        return base::CreateSharedPtr<base::io::MemoryReaderFileHandle>(buffer, base::StringBuf(rawFilePath));
    }

    bool PackedFileSystem::writeFile(base::StringView<char> rawFilePath, const base::Buffer& data, const base::io::TimeStamp* overrideTimeStamp, uint64_t overrideCRC)
    {
        return false;
    }

    bool PackedFileSystem::enableWriteOperations()
    {
        return true; // writes are done to parent FS any way...
    }*/

    void PackedFileSystem::enableFileSystemObservers()
    {
        // not used
    }

} // hl2