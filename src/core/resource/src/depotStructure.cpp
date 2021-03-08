/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#include "build.h"
#include "depotStructure.h"

#include "core/io/include/directoryWatcher.h"
#include "core/io/include/fileHandle.h"
#include "core/xml/include/xmlUtils.h"
#include "core/xml/include/xmlDocument.h"

BEGIN_BOOMER_NAMESPACE()

//--

template< typename T >
static void UnlinkList(T* cur, T**& prev, T*& next)
{
    DEBUG_CHECK_RETURN_EX(prev != nullptr, "Not linked");

    if (next)
        next->prev = prev;
    if (prev)
        *prev = next;
    next = nullptr;
    prev = nullptr;
}

template< typename T >
static void LinkList(T*& list, T* cur)
{
    DEBUG_CHECK_RETURN_EX(cur->prev == nullptr, "Already linked");
    DEBUG_CHECK_RETURN_EX(cur->next == nullptr, "Already linked");

    cur->prev = &list;
    cur->next = list;
    if (list)
        list->prev = &cur->next;
    list = cur;
}

void DepotStructure::ResourceInfo::unlink()
{
    DEBUG_CHECK_RETURN_EX(file != nullptr, "Not linked");
    file = nullptr;

    UnlinkList(this, prev, next);
    UnlinkList(this, bucketPrev, bucketNext);
}

//--

void DepotStructure::FileInfo::link(ResourceInfo* res)
{
    DEBUG_CHECK_RETURN_EX(res != nullptr, "Invalid resource");
    DEBUG_CHECK_RETURN_EX(res->file == nullptr, "Already linked");

    res->file = this;
    LinkList(resList, res);
}

void DepotStructure::FileInfo::unlink()
{
    DEBUG_CHECK_RETURN_EX(dir != nullptr, "Not linked");

    ASSERT(dir->numFiles > 0);
    dir->numFiles -= 1;

    dir = nullptr;

    UnlinkList(this, prev, next);
}

//--

void DepotStructure::DirectoryInfo::link(DirectoryInfo* dir)
{
    DEBUG_CHECK_RETURN_EX(dir != nullptr, "Invalid resource");
    DEBUG_CHECK_RETURN_EX(dir->parent == nullptr, "Already linked");

    dir->parent = this;
    numDirs += 1;
    LinkList(dirList, dir);
}

void DepotStructure::DirectoryInfo::link(FileInfo* file)
{
    DEBUG_CHECK_RETURN_EX(file != nullptr, "Invalid resource");
    DEBUG_CHECK_RETURN_EX(file->dir == nullptr, "Already linked");

    file->dir = this;
    numFiles += 1;
    LinkList(fileList, file);
}

const DepotStructure::FileInfo* DepotStructure::DirectoryInfo::findFile(StringView name) const
{
    const auto* cur = fileList;
    while (cur)
    {
        if (0 == cur->name.caseCmp(name))
            return cur;

        cur = cur->next;
    }

    return nullptr;
}

const DepotStructure::DirectoryInfo* DepotStructure::DirectoryInfo::findDir(StringView name) const
{
    const auto* cur = dirList;
    while (cur)
    {
        if (0 == cur->name.caseCmp(name))
            return cur;

        cur = cur->next;
    }

    return nullptr;
}

void DepotStructure::DirectoryInfo::unlink()
{
    DEBUG_CHECK_RETURN_EX(parent != nullptr, "Not linked");

    ASSERT(parent->numDirs > 0);
    parent->numDirs -= 1;

    parent = nullptr;

    UnlinkList(this, prev, next);
}

//--

IDepotProblemReporter::~IDepotProblemReporter()
{}

//--

DepotStructure::DepotStructure(StringView depotRootPath, bool observerDynamicChanges)
    : m_rootPath(depotRootPath)
    , m_filePool(POOL_MANAGED_DEPOT, 4096)
    , m_dirPool(POOL_MANAGED_DEPOT, 4096)
    , m_resourcePool(POOL_MANAGED_DEPOT, 4096)
    , m_stringHashPool(POOL_MANAGED_DEPOT, 4096)
{
    memzero(m_stringHashBuckets, sizeof(m_stringHashBuckets));
    memzero(m_resourceHashBuckets, sizeof(m_resourceHashBuckets));

    if (observerDynamicChanges)
        if (m_watcher = CreateDirectoryWatcher(depotRootPath))
            m_watcher->attachListener(this);
}

DepotStructure::~DepotStructure()
{
    if (m_watcher)
        m_watcher->dettachListener(this);
}

void DepotStructure::scan(IDepotProblemReporter& err)
{
    ScopeTimer timer;

    TRACE_INFO("Scaning depot at '{}'...", m_rootPath);

    // create root directory
    m_rootDir = m_dirPool.create();
    m_rootDir->localPath = "";
    m_rootDir->parent = nullptr;

    // scan content at root
    scanDirContent(m_rootDir, err);

    TRACE_INFO("Finished scanning in {}, found {} files, {} dirs and {} resources",
        timer, m_filePool.size(), m_dirPool.size(), m_resourcePool.size());
}

void DepotStructure::exploreDirectories(DirectoryInfo* dir, Array<DirectoryInfo*>& outNewDirs)
{
    InplaceArray<DirectoryInfo*, 256> stack;
    stack.pushBack(dir);

    while (!stack.empty())
    {
        auto* dir = stack.back();
        stack.popBack();

        FindSubDirs(TempString("{}{}", m_rootPath, dir->localPath), [this, dir, &outNewDirs, &stack](StringView name)
            {
                auto* childDir = m_dirPool.create();
                childDir->name = mapString_NoLock(name);
                childDir->localPath = mapString_NoLock(TempString("{}{}/", dir->localPath, name));

                dir->link(childDir);
                outNewDirs.pushBack(childDir);
                stack.pushBack(childDir);

                return false;
            });
    }    
}

void DepotStructure::scanDirContent(DirectoryInfo* dir, IDepotProblemReporter& err)
{
    Array<DirectoryInfo*> collectedDirs;
    collectedDirs.reserve(1024);

    // collect ALL child directories
    {
        ScopeTimer timer;
        exploreDirectories(dir, collectedDirs);
        TRACE_WARNING("Found {} dirs in {}", collectedDirs.size(), timer);
    }

    // scan each directory
    Array<FileInfo*> collectedFiles;
    {
        ScopeTimer timer;

        // TODO: fibers
        std::atomic<uint32_t> totalFiles = 0;
        for (auto* dir : collectedDirs)
        {
            scanDirFiles(dir);
            totalFiles += dir->numFiles;
        }

        // extract file list
        {
            collectedFiles.resize(totalFiles);

            auto* writeFile = collectedFiles.typedData();

            for (auto* dir : collectedDirs)
            {
                auto* file = dir->fileList;
                while (file)
                {
                    *writeFile++ = file;
                    file = file->next;
                }
            }
        }

        TRACE_WARNING("Found {} files in {}", totalFiles.load(), timer);
    }

    // load resource ID at each file
    {
        ScopeTimer timer;

        // TODO: fibers
        const auto resCount = m_resourcePool.size();
        for (auto* file : collectedFiles)
            if (file)
                scanFileResources(file, err);

        TRACE_WARNING("Found {} resources in {}", m_resourcePool.size() - resCount, timer);
    }
}

void DepotStructure::scanDirFiles(DirectoryInfo* dir)
{
    FindLocalFiles(TempString("{}{}", m_rootPath, dir->localPath), "*.*", [this, dir](StringView name)
        {
            m_filePoolLock.acquire();

            auto* childFile = m_filePool.create();
            childFile->name = mapString_NoLock(name);
            
            m_filePoolLock.release();

            dir->link(childFile);
            return false;
        });
}

bool DepotStructure::scanFileResourcesInternal(FileInfo* file, StringView path)
{
    auto fileHandle = OpenForReading(path);
    if (!fileHandle)
        return false;

    // file is way to big
    const auto fileSizeLimit = 100 << 10;
    const auto fileSize = fileHandle->size();
    if (fileSize > fileSizeLimit)
        return false;

    // load the header
    char buf[32];
    if (fileHandle->readSync(buf, sizeof(buf)) != sizeof(buf))
        return false;

    // check if it's XML
    const auto header = "<?xml version=";
    if (memcmp(buf, header, sizeof(header) != 0))
        return false;

    // create buffer
    auto tempData = Buffer::Create(POOL_TEMP, fileSize);
    if (!tempData)
        return false;

    // load content
    fileHandle->pos(0);
    if (fileSize != fileHandle->readSync(tempData.data(), fileSize))
        return false;

    // parse as XML
    const auto doc = xml::LoadDocument(xml::ILoadingReporter::GetDefault(), tempData);
    if (!doc)
        return false;

    // TODO: read IDs

    return true;
}

void DepotStructure::scanFileResources(FileInfo* file, IDepotProblemReporter& err)
{
    if (!file->name.endsWith(".xmeta"))
        return;

    if (!scanFileResourcesInternal(file, TempString("{}{}{}", m_rootPath, file->dir->localPath, file->name)))
    {
        err.reportInvalidFile(TempString("{}{}{}", m_rootPath, file->dir->localPath, file->name));
    }
}

bool DepotStructure::resolvePathForID(const ResourceID& id, StringBuf& outRelativePath) const
{
    auto lock = CreateLock(m_globalLock); // TODO: RW lock

    const auto hash = GUID::CalcHash(id.guid());
    const auto bucketIndex = hash & NUM_ID_BUCKETS;

    const auto* bucket = m_resourceHashBuckets[bucketIndex];
    while (bucket)
    {
        if (bucket->id == id)
        {
            outRelativePath = TempString("{}{}", bucket->file->dir->localPath, bucket->file->name);
            return true;
        }

        bucket = bucket->bucketNext;
    }

    return false;
}

const DepotStructure::DirectoryInfo* DepotStructure::findDir_NoLock(StringView path) const
{
    InplaceArray<StringView, 20> parts;
    path.slice("/", false, parts);

    const auto* dir = m_rootDir;
    for (const auto part : parts)
    {
        if (part == ".")
            continue;

        if (part == "..")
            dir = dir->parent;
        else
            dir = dir->findDir(part);

        if (!dir)
            break;
    }

    return dir;
}

static void SplitPathIntoParts(StringView path, StringView& outDirPath, StringView& outFileName)
{
    const auto pos = path.findLastChar('/');
    if (pos != INDEX_NONE)
    {
        if (pos > 0)
            outDirPath = path.leftPart(pos - 1);
        if (pos < path.length())
            outFileName = path.subString(pos + 1);
    }
    else
    {
        outFileName = path;
    }
}

const DepotStructure::FileInfo* DepotStructure::findFile_NoLock(StringView path) const
{
    if (path.empty())
        return nullptr;

    StringView dirPath, fileName;
    SplitPathIntoParts(path, dirPath, fileName);

    const auto* dir = findDir_NoLock(dirPath);
    if (!dir)
        return nullptr;

    return dir->findFile(fileName);
}

bool DepotStructure::resolveIDForPath(StringView path, ResourceID& outId) const
{
    auto lock = CreateLock(m_globalLock); // TODO: RW lock

    if (const auto* file = findFile_NoLock(path))
    {
        if (file->resList)
        {
            outId = file->resList->id;
            return true;
        }
    }

    return false;
}

StringView DepotStructure::mapString_NoLock(StringView txt)
{
    if (txt.empty())
        return "";

    const auto hash = txt.calcCRC64();
    const auto bucketIndex = hash % NUM_STRING_BUCKETS;

    // search in the hash map
    auto*& bucketList = m_stringHashBuckets[bucketIndex];
    auto* bucket = bucketList;
    while (bucket)
    {
        if (bucket->hash == hash && bucket->data == txt)
            return bucket->data;
        bucket = bucket->next;
    }

    // create new entry
    auto* entry = m_stringHashPool.create();
    entry->data = m_stringPool.strcpy(txt.data(), txt.length());
    entry->hash = hash;
    entry->next = bucketList;
    bucketList = entry;

    return entry->data;
}

void DepotStructure::enumDirectories(StringView path, const std::function<void(StringView)>& func) const
{
    auto lock = CreateLock(m_globalLock); // TODO: RW lock

    if (const auto* dir = findDir_NoLock(path))
    {
        const auto* cur = dir->dirList;
        while (cur)
        {
            func(cur->name);
            cur = cur->next;
        }
    }
}


void DepotStructure::enumFiles(StringView path, const std::function<void(StringView)>& func) const
{
    auto lock = CreateLock(m_globalLock); // TODO: RW lock

    if (const auto* dir = findDir_NoLock(path))
    {
        const auto* cur = dir->fileList;
        while (cur)
        {
            func(cur->name);
            cur = cur->next;
        }
    }
}

//--

void DepotStructure::handleEvent(const DirectoryWatcherEvent& evt)
{
    /*StringBuf depotPath;
    if (queryFileDepotPath(evt.path, depotPath))
    {
        switch (evt.type)
        {

        case DirectoryWatcherEventType::FileAdded:
            TRACE_INFO("Depot file was reported as added", depotPath);
            DispatchGlobalEvent(m_eventKey, EVENT_DEPOT_FILE_ADDED, depotPath);
            break;

        case DirectoryWatcherEventType::FileRemoved:
            TRACE_INFO("Depot file was reported as removed", depotPath);
            DispatchGlobalEvent(m_eventKey, EVENT_DEPOT_FILE_REMOVED, depotPath);
            break;

        case DirectoryWatcherEventType::DirectoryAdded:
            TRACE_INFO("Depot directory '{}' was reported as added", depotPath);
            DispatchGlobalEvent(m_eventKey, EVENT_DEPOT_DIRECTORY_ADDED, depotPath);
            break;

        case DirectoryWatcherEventType::DirectoryRemoved:
            TRACE_INFO("Depot directory '{}' was reported as removed", depotPath);
            DispatchGlobalEvent(m_eventKey, EVENT_DEPOT_DIRECTORY_REMOVED, depotPath);
            break;

        case DirectoryWatcherEventType::FileContentChanged:
            TRACE_INFO("Depot file '{}' was reported as changed", depotPath);
            DispatchGlobalEvent(m_eventKey, EVENT_DEPOT_FILE_CHANGED, depotPath);
            break;

        case DirectoryWatcherEventType::FileMetadataChanged:
            break;
        }
    }*/
}

//--

END_BOOMER_NAMESPACE()

