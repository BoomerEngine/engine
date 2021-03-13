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

    //--

    /// perform initial scan
    void scan(IDepotProblemReporter& err);

    //--

    /// find load path (relative to the root) for given ID
    bool resolvePathForID(const ResourceID& id, StringBuf& outRelativePath) const;

    /// collect resource IDs at given path
    bool resolveIDForPath(StringView path, ResourceID& outId) const;

    //--

    // list directories at given path
    void enumDirectories(StringView path, const std::function<void(StringView)>& func) const;

    // list directories at given path
    void enumFiles(StringView path, const std::function<void(StringView)>& func) const;

    // find file matching given file name, returning "true" from enum function will break
    bool enumFilesRecrusive(StringView path, int maxDepth, const std::function<bool(StringView, StringView)>& func) const;

    //--

    enum class EventType
    {
        FileChanged,
        FileAdded,
        FileRemoved,
        FileReloaded,
        DirectoryAdded,
        DirectoryRemoved,
    };

    struct Event
    {
        EventType type;
        StringBuf localPath;
        StringBuf depotPath;
    };

    /// update structure with incoming low-level FS updates, returns high-level events
    void update(StringView prefixPath, Array<Event>& outEvents, IDepotProblemReporter& err);

    //--

private:
    StringBuf m_rootPath;

    Mutex m_globalLock;

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

        FileInfo* findFile(StringView name) const;
        DirectoryInfo* findDir(StringView name) const;

        void unlink();
    };

    StructurePool<DirectoryInfo> m_dirPool;

    DirectoryInfo* m_rootDir = nullptr;

    SpinLock m_filePoolLock;
    StructurePool<FileInfo> m_filePool;

    SpinLock m_resourcePoolLock;
    StructurePool<ResourceInfo> m_resourcePool;

    void exploreDirectories_NoLock(DirectoryInfo* dir, Array<DirectoryInfo*>& outNewDirs);

    void scanDirContent_NoLock(DirectoryInfo* dir, IDepotProblemReporter& err);
    void scanDirFiles_NoLock(DirectoryInfo* dir);
    void scanFileResources_NoLock(FileInfo* file, IDepotProblemReporter& err);

    bool scanFileResourcesInternal_NoLock(StringView localPath, Array<ResourceID>& outIDs) const;

    const DirectoryInfo* findDir_NoLock(StringView path) const;
    DirectoryInfo* findDir_NoLock(StringView path);

    const FileInfo* findFile_NoLock(StringView path) const;
    FileInfo* findFile_NoLock(StringView path);

    ResourceInfo* findOrCreateResourceInfo_NoLock(ResourceID id);

    FileInfo* createFileInDirectory_NoLock(DirectoryInfo* info, StringView name, IDepotProblemReporter& err);
    DirectoryInfo* createDirectoryInDirectory_NoLock(DirectoryInfo* info, StringView name, IDepotProblemReporter& err);

    void removeFile_NoLock(FileInfo* file);
    void removeDir_NoLock(DirectoryInfo* dir, uint32_t& outNumRemovedDirs, uint32_t& outNumRemovedFiles);

    void unlinkFileResources_NoLock(FileInfo* info);

    //--

    static const uint32_t NUM_ID_BUCKETS = 65536;
    ResourceInfo* m_resourceHashBuckets[NUM_ID_BUCKETS];

    //--

    struct StringHashEntry
    {
        uint64_t hash;
        StringView data;
        StringHashEntry* next = nullptr;
    };

    LinearAllocator m_stringPool;
    StructurePool<StringHashEntry> m_stringHashPool;

    static const uint32_t NUM_STRING_BUCKETS = 65536;
    StringHashEntry* m_stringHashBuckets[NUM_STRING_BUCKETS];

    StringView mapString_NoLock(StringView txt);

    //--

    SpinLock m_bufferedFileEventsLock;
    Array<DirectoryWatcherEvent> m_bufferedFileEvents;

    void gatherEvents_NoLock(StringView prefixPath, Array<Event>& outEvents);
    void applyEvent_NoLock(const Event& evt, IDepotProblemReporter& err, bool& outReportToHighLevel);

    void applyEvent_FileChanged_NoLock(StringView localPath, IDepotProblemReporter& err, bool& outReportToHighLevel);
    void applyEvent_FileAdded_NoLock(StringView localPath, IDepotProblemReporter& err, bool& outReportToHighLevel);
    void applyEvent_FileRemoved_NoLock(StringView localPath, IDepotProblemReporter& err, bool& outReportToHighLevel);
    void applyEvent_DirAdded_NoLock(StringView localPath, IDepotProblemReporter& err, bool& outReportToHighLevel);
    void applyEvent_DirRemoved_NoLock(StringView localPath, IDepotProblemReporter& err, bool& outReportToHighLevel);

    virtual void handleEvent(const DirectoryWatcherEvent& evt) override;
};

END_BOOMER_NAMESPACE()
