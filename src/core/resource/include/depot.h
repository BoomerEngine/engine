/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: service #]
***/

#pragma once

#include "core/app/include/localService.h"
#include "core/io/include/directoryWatcher.h"

BEGIN_BOOMER_NAMESPACE()

//---

/// depot data service
class CORE_RESOURCE_API DepotService : public app::ILocalService
{
    RTTI_DECLARE_VIRTUAL_CLASS(DepotService, app::ILocalService);

public:
    DepotService();

    //--

    /// get absolute path to engine depot directory
    INLINE const StringBuf& engineDepotPath() const { return m_engineDepotPath; }

    /// get absolute path to project depot directory
    INLINE const StringBuf& projectDepotPath() const { return m_projectDepotPath; }

    //--

    /// get the file information
    bool queryFileTimestamp(StringView depotPath, TimeStamp& outTimestamp) const;

    /// if the file is loaded from a physical file query it's path
    /// NOTE: is present only for files physically on disk, not in archives or over the network
    bool queryFileAbsolutePath(StringView depotPath, StringBuf& outAbsolutePath) const;

    /// if the given absolute file is loadable via the depot path than find it
    /// NOTE: is present only for files physically on disk, not in archives or over the network
    bool queryFileDepotPath(StringView absolutePath, StringBuf& outDeptotPath) const;

    /// create a reader for the file's content
    /// NOTE: creating a reader may take some time
    ReadFileHandlePtr createFileReader(StringView depotPath) const;

    /// create a writer for the file's content
    /// NOTE: fails if the file system was not writable
    WriteFileHandlePtr createFileWriter(StringView depotPath) const;

    /// create an ASYNC reader for the file's content
    /// NOTE: creating a reader may take some time
    AsyncFileHandlePtr createFileAsyncReader(StringView depotPath) const;

    //--

    struct DirectoryInfo
    {
        StringView name;
        bool fileSystemRoot = false;

        INLINE bool operator<(const DirectoryInfo& other) const
        {
            return name < other.name;
        }
    };

    /// get child directories at given path
    /// NOTE: virtual directories for the mounted file systems are reported as well
    bool enumDirectoriesAtPath(StringView rawDirectoryPath, const std::function<bool(const DirectoryInfo& info) >& enumFunc) const;

    struct FileInfo
    {
        StringView name;

        INLINE bool operator<(const FileInfo& other) const
        {
            return name < other.name;
        }
    };

    /// get files at given path (should be absolute depot DIRECTORY path)
    bool enumFilesAtPath(StringView path, const std::function<bool(const FileInfo& info)>& enumFunc) const;

    //--

    // find file in depot
    bool findFile(StringView depotPath, StringView fileName, uint32_t maxDepth, StringBuf& outFoundFileDepotPath) const;

    //--

    // load file content to buffer
    bool loadFileToBuffer(StringView depotPath, Buffer& outContent, TimeStamp* timestamp = nullptr) const;

    // load file content to string
    bool loadFileToString(StringView depotPath, StringBuf& outContent, TimeStamp* timestamp = nullptr) const;

    //--

    // find load path for given resource ID
    bool resolvePathForID(const ResourceID& id, StringBuf& outLoadPath) const;

    // create or retrieve a resource ID for given depot path
    bool resolveIDForPath(StringView depotPath, ResourceID& outID) const;

    //--

private:
    StringBuf m_engineDepotPath;
    StringBuf m_projectDepotPath;

    DepotStructure* m_engineDepotStructure = nullptr;
    DepotStructure* m_projectDepotStructure = nullptr;

    //--

    virtual app::ServiceInitializationResult onInitializeService(const app::CommandLine& cmdLine) override final;
    virtual void onShutdownService() override final;
    virtual void onSyncUpdate() override final;
};

//---

END_BOOMER_NAMESPACE()
