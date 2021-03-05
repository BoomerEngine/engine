/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

#include "id.h"

#include "core/system/include/guid.h"
#include "core/io/include/directoryWatcher.h"
#include "core/memory/include/structurePool.h"
#include "core/memory/include/linearAllocator.h"

BEGIN_BOOMER_NAMESPACE()

/// error reporter for any problems
class CORE_RESOURCE_API IDepotProblemReporter : public NoCopy
{
public:
    virtual ~IDepotProblemReporter();

    virtual void reportInvalidFile(StringView path) = 0;
    virtual void reportDuplicatedID(ResourceID id, StringView path, StringView otherPath) = 0;
};

/// helper class that caches resource IDs in given folder structure
class CORE_RESOURCE_API DepotStructure : public IDirectoryWatcherListener
{
public:
    DepotStructure(StringView depotRootPath, bool observerDynamicChanges=false);
    ~DepotStructure();

    /// perform initial scan
    void scan(IDepotProblemReporter& err);

    /// find load path (relative to the root) for given ID
    bool resolveID(const ResourceID& id, StringBuf& outRelativePath) const;

    /// process any pending changes

private:
    StringBuf m_rootPath;

    RefPtr<IDirectoryWatcher> m_watcher;

    struct DirectoryInfo;
    struct FileInfo;
    struct ResourceInfo;

    struct ResourceInfo
    {
        ResourceID id;

        FileInfo* file = nullptr;
        ResourceInfo* next = nullptr;
        ResourceInfo** prev = nullptr;

        ResourceInfo* bucketNext = nullptr;
        ResourceInfo** bucketPrev = nullptr;

        void unlink();
    };

    struct FileInfo
    {
        StringView name;

        DirectoryInfo* dir = nullptr;
        ResourceInfo* resList = nullptr;

        FileInfo* next = nullptr;
        FileInfo** prev = nullptr;

        void link(ResourceInfo* res);
        void unlink();
    };

    struct DirectoryInfo
    {
        StringView name;
        StringView localPath;

        DirectoryInfo* parent = nullptr;
        DirectoryInfo* dirList = nullptr;
        FileInfo* fileList = nullptr;

        uint32_t numDirs = 0;
        uint32_t numFiles = 0;

        DirectoryInfo* next = nullptr;
        DirectoryInfo** prev = nullptr;

        void link(DirectoryInfo* dir);
        void link(FileInfo* file);

        void unlink();
    };

    StructurePool<DirectoryInfo> m_dirPool;

    SpinLock m_filePoolLock;
    StructurePool<FileInfo> m_filePool;

    SpinLock m_resourcePoolLock;
    StructurePool<ResourceInfo> m_resourcePool;

    void exploreDirectories(DirectoryInfo* dir, Array<DirectoryInfo*>& outNewDirs);

    void scanDirContent(DirectoryInfo* dir, IDepotProblemReporter& err);
    void scanDirFiles(DirectoryInfo* dir);
    void scanFileResources(FileInfo* file, IDepotProblemReporter& err);
    bool scanFileResourcesInternal(FileInfo* file, StringView path);

    //--

    struct StringHashEntry
    {
        uint64_t hash;
        StringView data;
        StringHashEntry* next = nullptr;
    };

    LinearAllocator m_stringPool;
    StructurePool<StringHashEntry> m_stringHashPool;

    static const uint32_t NUM_BUCKETS = 65536;
    StringHashEntry* m_stringHashBuckets[NUM_BUCKETS];

    StringView mapString_NoLock(StringView txt);

    //--

    virtual void handleEvent(const DirectoryWatcherEvent& evt) override;
};

END_BOOMER_NAMESPACE()
