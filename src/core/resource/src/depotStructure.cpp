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
#include "metadata.h"
#include "core/containers/include/path.h"
#include "core/containers/include/queue.h"

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

template< typename T >
static void LinkListBucket(T*& list, T* cur)
{
    DEBUG_CHECK_RETURN_EX(cur->bucketPrev == nullptr, "Already linked");
    DEBUG_CHECK_RETURN_EX(cur->bucketNext == nullptr, "Already linked");

    cur->bucketPrev = &list;
    cur->bucketNext = list;
    if (list)
        list->bucketPrev = &cur->bucketNext;
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

DepotStructure::FileInfo* DepotStructure::DirectoryInfo::findFile(StringView name) const
{
    auto* cur = fileList;
    while (cur)
    {
        if (0 == cur->name.caseCmp(name))
            return cur;

        cur = cur->next;
    }

    return nullptr;
}

DepotStructure::DirectoryInfo* DepotStructure::DirectoryInfo::findDir(StringView name) const
{
    auto* cur = dirList;
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
    {
        auto lock = CreateLock(m_globalLock);
        scanDirContent_NoLock(m_rootDir, err);
    }

    TRACE_INFO("Finished scanning in {}, found {} files, {} dirs and {} resources",
        timer, m_filePool.size(), m_dirPool.size(), m_resourcePool.size());
}

void DepotStructure::exploreDirectories_NoLock(DirectoryInfo* dir, Array<DirectoryInfo*>& outNewDirs)
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

void DepotStructure::scanDirContent_NoLock(DirectoryInfo* dir, IDepotProblemReporter& err)
{
    DEBUG_CHECK_RETURN_EX(dir->numDirs == 0, "Scan dir called on non-empty directory");
    DEBUG_CHECK_RETURN_EX(dir->numFiles == 0, "Scan dir called on non-empty directory");

    Array<DirectoryInfo*> collectedDirs;
    collectedDirs.reserve(1024);

    // collect ALL child directories
    {
        ScopeTimer timer;
        exploreDirectories_NoLock(dir, collectedDirs);
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
            scanDirFiles_NoLock(dir);
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
                scanFileResources_NoLock(file, err);

        TRACE_WARNING("Found {} resources in {}", m_resourcePool.size() - resCount, timer);
    }
}

void DepotStructure::scanDirFiles_NoLock(DirectoryInfo* dir)
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

bool DepotStructure::scanFileResourcesInternal_NoLock(StringView localPath, Array<ResourceID>& outIDs) const
{
    auto fileHandle = OpenForReading(TempString("{}{}", m_rootPath, localPath));
    DEBUG_CHECK_RETURN_EX_V(fileHandle, "Unable to open file for reading", false);
    
    // file is way to big
    const auto fileSizeLimit = 100 << 10;
    const auto fileSize = fileHandle->size();
    DEBUG_CHECK_RETURN_EX_V(fileSize < fileSizeLimit, "Metadata file is to large", false);

    // load the header
    char buf[32];
    DEBUG_CHECK_RETURN_EX_V(fileHandle->readSync(buf, sizeof(buf)) == sizeof(buf), "Unable to read file header", false);

    // check if it's XML
    const auto header = "<?xml version=";
    DEBUG_CHECK_RETURN_EX_V(memcmp(buf, header, sizeof(header)) == 0, "Invalid metadata header (not an XML)", false);

    // create buffer
    auto tempData = Buffer::Create(POOL_TEMP, fileSize);
    DEBUG_CHECK_RETURN_EX_V(tempData, "Unable to allocate memory", false);

    // load content
    fileHandle->pos(0);
    DEBUG_CHECK_RETURN_EX_V(fileSize == fileHandle->readSync(tempData.data(), fileSize), "Unable to read file content", false);

    // parse as XML
    const auto doc = xml::LoadDocument(xml::ILoadingReporter::GetDefault(), tempData);
    DEBUG_CHECK_RETURN_EX_V(doc, "Metadata's XML parsing error", false);

    /*
    * <object class="ResourceMetadata">
       <ids>
        <element>{D898CDAA-0F0C-4B55-BC92-870DA3C345B9}</element>
       </ids>
    */

    auto idsID = doc->nodeFirstChild(doc->root(), "ids");
    DEBUG_CHECK_RETURN_EX_V(idsID, "Missing 'ids' tag in XML", false);

    auto elementID = doc->nodeFirstChild(idsID, "element");
    DEBUG_CHECK_RETURN_EX_V(elementID, "Missing 'element' tag in XML", false);

    while (elementID)
    {
        auto elementValue = doc->nodeValue(elementID);
        DEBUG_CHECK_EX(elementValue, "Missing ID value in 'element' tag in XML");

        if (elementValue)
        {
            ResourceID id;
            ResourceID::Parse(elementValue, id);
            DEBUG_CHECK_EX(id, "Unable to parse ID");

            if (id)
                outIDs.pushBack(id);
        }

        elementID = doc->nodeSibling(elementID, "element");
    }    

    return true;
}

void DepotStructure::unlinkFileResources_NoLock(FileInfo* info)
{
    auto* res = info->resList;
    while (res)
    {
        auto* next = res->next;
        TRACE_INFO("Depot unlinking ID '{}' from file '{}'", res->id, info->name);
        res->unlink();
        res = next;
    }

    DEBUG_CHECK(info->resList == nullptr);
    info->resList = nullptr;
}

DepotStructure::ResourceInfo* DepotStructure::findOrCreateResourceInfo_NoLock(ResourceID id)
{
    auto lock = CreateLock(m_globalLock); // TODO: RW lock

    const auto hash = GUID::CalcHash(id.guid());
    const auto bucketIndex = hash % NUM_ID_BUCKETS;

    // find existing
    auto* bucket = m_resourceHashBuckets[bucketIndex];
    while (bucket)
    {
        if (bucket->id == id)
            return bucket;

        bucket = bucket->bucketNext;
    }

    // create new
    bucket = m_resourcePool.create();
    bucket->id = id;

    // link to hash map
    LinkListBucket(m_resourceHashBuckets[bucketIndex], bucket);
    return bucket;
}

void DepotStructure::scanFileResources_NoLock(FileInfo* file, IDepotProblemReporter& err)
{
    // unlink any resource we got reported from this file
    unlinkFileResources_NoLock(file);

    // load metadata
    if (file->name.endsWith(ResourceMetadata::FILE_EXTENSION))
    {
        InplaceArray<ResourceID, 10> ids;
        if (!scanFileResourcesInternal_NoLock(TempString("{}{}", file->dir->localPath, file->name), ids))
        {
            err.reportInvalidFile(TempString("{}{}{}", m_rootPath, file->dir->localPath, file->name));
        }
        else
        {
            for (const auto& id : ids)
            {
                auto* res = findOrCreateResourceInfo_NoLock(id);
                DEBUG_CHECK_EX(res, "Failed to create resource entry");

                if (res)
                {
                    if (res->file)
                    {
                        const auto thisFile = TempString("{}{}", file->dir->localPath, file->name);
                        const auto otherFile = TempString("{}{}", res->file->dir->localPath, res->file->name);
                        err.reportDuplicatedID(id, thisFile, otherFile);
                    }
                    else
                    {
                        TRACE_INFO("Depot linking ID '{}' to file '{}' in '{}'", res->id, file->name, file->dir->localPath);
                        file->link(res);
                    }
                }
            }
        }
    }
}

bool DepotStructure::resolvePathForID(const ResourceID& id, StringBuf& outRelativePath) const
{
    auto lock = CreateLock(m_globalLock); // TODO: RW lock

    const auto hash = GUID::CalcHash(id.guid());
    const auto bucketIndex = hash % NUM_ID_BUCKETS;

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
    return const_cast<DepotStructure*>(this)->findDir_NoLock(path);
}

DepotStructure::DirectoryInfo* DepotStructure::findDir_NoLock(StringView path)
{
    InplaceArray<StringView, 20> parts;
    path.slice("/", false, parts);

    auto* dir = m_rootDir;
    for (const auto part : parts)
    {
        if (part == ".")
            continue;

        if (part.data()[0] == '~' || part.data()[0] == '.')
            return nullptr;

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
            outDirPath = path.leftPart(pos + 1); // include the separator
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
    return const_cast<DepotStructure*>(this)->findFile_NoLock(path);
}

static bool ShouldSkipFile(StringView fileName)
{
    const auto ext = fileName.afterFirst(".");
    if (!ext || ext == "bak" || ext == "tmp")
        return true;

    if (fileName.data()[0] == '.' || fileName.data()[0] == '~')
        return true;

    return false;
}

static bool ShouldSkipPath(StringView path)
{
    InplaceArray<StringView, 10> parts;
    path.slice("/", false, parts);

    for (const auto part : parts)
    {
        if (part.data()[0] == '.' || part.data()[0] == '~')
            return true;
    }

    return false;
}

DepotStructure::FileInfo* DepotStructure::createFileInDirectory_NoLock(DirectoryInfo* dir, StringView name, IDepotProblemReporter& err)
{
    DEBUG_CHECK_RETURN_EX_V(dir, "Invalid directory", nullptr);
    DEBUG_CHECK_RETURN_EX_V(name, "Invalid file name", nullptr);
    DEBUG_CHECK_RETURN_EX_V(!ShouldSkipFile(name), "Non-trackced file name", nullptr);
    DEBUG_CHECK_RETURN_EX_V(name.extensions(), "File name with no extensions", nullptr);
    DEBUG_CHECK_RETURN_EX_V(!dir->findFile(name), "File already exists in the directory", nullptr);

    auto* file = m_filePool.create();
    file->name = mapString_NoLock(name);
    dir->link(file);

    scanFileResources_NoLock(file, err);

    return file;
}

DepotStructure::DirectoryInfo* DepotStructure::createDirectoryInDirectory_NoLock(DirectoryInfo* dir, StringView name, IDepotProblemReporter& err)
{
    DEBUG_CHECK_RETURN_EX_V(dir, "Invalid directory", nullptr);
    DEBUG_CHECK_RETURN_EX_V(name, "Invalid directory name", nullptr);
    DEBUG_CHECK_RETURN_EX_V(!ShouldSkipPath(name), "Non-trackced directory name", nullptr);
    DEBUG_CHECK_RETURN_EX_V(!dir->findDir(name), "Directory already exists in the directory", nullptr);

    auto* childDir = m_dirPool.create();
    childDir->name = mapString_NoLock(name);
    childDir->localPath = mapString_NoLock(TempString("{}{}/", dir->localPath, name));
    dir->link(childDir);

    scanDirContent_NoLock(childDir, err);

    return childDir;
}

void DepotStructure::removeFile_NoLock(FileInfo* file)
{
    DEBUG_CHECK_RETURN_EX(file, "Invalid file");
    DEBUG_CHECK_RETURN_EX(file->dir, "File not in directory");

    unlinkFileResources_NoLock(file);

    file->unlink();

    m_filePool.free(file);
}

void DepotStructure::removeDir_NoLock(DirectoryInfo* dir, uint32_t& outNumRemovedDirs, uint32_t& outNumRemovedFiles)
{
    // remove all child directories
    {
        auto* cur = dir->dirList;
        while (cur)
        {
            auto* next = cur->next;
            removeDir_NoLock(cur, outNumRemovedDirs, outNumRemovedFiles);
            cur = next;
        }
    }

    // remove all files
    {
        auto* cur = dir->fileList;
        while (cur)
        {
            auto* next = cur->next;
            removeFile_NoLock(cur);
            outNumRemovedFiles += 1;
            cur = next;
        }
    }

    dir->unlink();
    outNumRemovedDirs += 1;

    m_dirPool.free(dir);
}

DepotStructure::FileInfo* DepotStructure::findFile_NoLock(StringView path)
{
    if (path.empty())
        return nullptr;

    StringView dirPath, fileName;
    SplitPathIntoParts(path, dirPath, fileName);

    auto* dir = findDir_NoLock(dirPath);
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

bool DepotStructure::enumFilesRecrusive(StringView path, int maxDepth, const std::function<bool(StringView, StringView)>& func) const
{
    auto lock = CreateLock(m_globalLock); // TODO: RW lock

    if (const auto* dir = findDir_NoLock(path))
    {
        Queue<std::pair<const DirectoryInfo*, int>, InplaceArray<std::pair<const DirectoryInfo*, int>, 128>> dirStack;
        dirStack.push(std::make_pair(dir, maxDepth));

        while (!dirStack.empty())
        {
            auto dir = dirStack.top().first;
            auto depth = dirStack.top().second;
            dirStack.pop();

            // check files
            {
                const auto* cur = dir->fileList;
                while (cur)
                {
                    if (func(dir->localPath, cur->name))
                        return true;
                    cur = cur->next;
                }
            }

            // check dirs
            if (depth > 0)
            {
                const auto* cur = dir->dirList;
                while (cur)
                {
                    dirStack.push(std::make_pair(cur, depth - 1));
                    cur = cur->next;
                }
            }
        }
    }

    return false;
}

//--

void DepotStructure::gatherEvents_NoLock(StringView prefixPath, Array<Event>& outEvents)
{
    m_bufferedFileEventsLock.acquire();
    auto events = std::move(m_bufferedFileEvents);
    m_bufferedFileEventsLock.release();

    for (const auto& evt : events)
    {
        // FILE event
        if (evt.type == DirectoryWatcherEventType::FileAdded
            || evt.type == DirectoryWatcherEventType::FileRemoved
            || evt.type == DirectoryWatcherEventType::FileContentChanged)
        {
            auto localPath = ConformDepotFilePath(evt.path.subString(m_rootPath.length()));
            auto depotPath = StringBuf(TempString("{}{}", prefixPath, localPath));

            if (ShouldSkipFile(evt.path.view().fileName()))
                continue;

            if (evt.type == DirectoryWatcherEventType::FileAdded)
            {
                TRACE_INFO("Depot file added: '{}'", depotPath);
                auto& evt = outEvents.emplaceBack();
                evt.depotPath = depotPath;
                evt.localPath = localPath;
                evt.type = EventType::FileAdded;
            }
            else if (evt.type == DirectoryWatcherEventType::FileRemoved)
            {
                TRACE_INFO("Depot file removed: '{}'", depotPath);
                auto& evt = outEvents.emplaceBack();
                evt.depotPath = depotPath;
                evt.localPath = localPath;
                evt.type = EventType::FileRemoved;
            }
            else if (evt.type == DirectoryWatcherEventType::FileContentChanged)
            {
                if (depotPath.endsWith(ResourceMetadata::FILE_EXTENSION))
                {
                    TRACE_INFO("Depot file changed: '{}'", depotPath);
                    auto& evt = outEvents.emplaceBack();
                    evt.depotPath = depotPath;
                    evt.localPath = localPath;
                    evt.type = EventType::FileChanged;
                }
            }
        }

        // DIRECTORY event
        else if (evt.type == DirectoryWatcherEventType::DirectoryAdded ||
            evt.type == DirectoryWatcherEventType::DirectoryRemoved)
        {
            auto localPath = ConformDepotDirectoryPath(evt.path.subString(m_rootPath.length()));
            auto depotPath = StringBuf(TempString("{}{}", prefixPath, localPath));

            const auto dirName = localPath.view().directoryName();
            if (dirName.beginsWith("~") || dirName.beginsWith("."))
                continue;

            if (evt.type == DirectoryWatcherEventType::DirectoryAdded)
            {
                TRACE_INFO("Depot directory added: '{}'", depotPath);
                auto& evt = outEvents.emplaceBack();
                evt.depotPath = depotPath;
                evt.localPath = localPath;
                evt.type = EventType::DirectoryAdded;
            }
            else if (evt.type == DirectoryWatcherEventType::DirectoryRemoved)
            {
                TRACE_INFO("Depot directory removed: '{}'", depotPath);
                auto& evt = outEvents.emplaceBack();
                evt.depotPath = depotPath;
                evt.localPath = localPath;
                evt.type = EventType::DirectoryRemoved;
            }
        }
    }
}

void DepotStructure::applyEvent_FileChanged_NoLock(StringView localPath, IDepotProblemReporter& err, bool& outReportToHighLevel)
{
    auto* file = findFile_NoLock(localPath);
    DEBUG_CHECK_RETURN_EX(file, TempString("Change to non-tracked file '{}'", localPath));

    if (file->name.endsWith(ResourceMetadata::FILE_EXTENSION))
    {
        scanFileResources_NoLock(file, err);
    }
    else
    {
        // TODO: reload ?
    }
}

void DepotStructure::applyEvent_FileAdded_NoLock(StringView localPath, IDepotProblemReporter& err, bool& outReportToHighLevel)
{
    auto dirPath = localPath.baseDirectory();

    auto fileName = localPath.fileName();
    DEBUG_CHECK_RETURN_EX(!ShouldSkipFile(fileName), TempString("Trying to track non-tracked file '{}'", localPath));

    auto* dir = findDir_NoLock(dirPath);
    DEBUG_CHECK_RETURN_EX(dir, TempString("Adding file to a non to non-tracked directory '{}'", dir->localPath));

    auto* file = dir->findFile(fileName);
    if (!file)
    {
        createFileInDirectory_NoLock(dir, fileName, err);
        outReportToHighLevel = true;
    }
    else
    {
        TRACE_WARNING("File '{}' already exists in the parent directory", localPath);
    }
}

void DepotStructure::applyEvent_FileRemoved_NoLock(StringView localPath, IDepotProblemReporter& err, bool& outReportToHighLevel)
{
    if (auto* file = findFile_NoLock(localPath))
    {
        removeFile_NoLock(file);
        outReportToHighLevel = true;
    }
}

void DepotStructure::applyEvent_DirAdded_NoLock(StringView localPath, IDepotProblemReporter& err, bool& outReportToHighLevel)
{
    DEBUG_CHECK_RETURN_EX(!ShouldSkipPath(localPath), TempString("Trying to track non-tracked directory '{}'", localPath));

    auto parentDirPath = localPath.parentDirectory();
    DEBUG_CHECK_RETURN_EX(parentDirPath, "Invalid base path");

    auto* parentDir = findDir_NoLock(parentDirPath);
    DEBUG_CHECK_RETURN_EX(parentDir, TempString("Adding child directory to a non to non-tracked directory '{}'", localPath));

    auto directoryName = localPath.directoryName();

    auto* dir = parentDir->findDir(directoryName);
    if (!dir)
    {
        createDirectoryInDirectory_NoLock(parentDir, directoryName, err);
        outReportToHighLevel = true;
    }
    else
    {
        TRACE_WARNING("Directory '{}' already exists in the parent directory", localPath);
    }
}

void DepotStructure::applyEvent_DirRemoved_NoLock(StringView localPath, IDepotProblemReporter& err, bool& outReportToHighLevel)
{
    if (auto* dir = findDir_NoLock(localPath))
    {
        uint32_t numRemovedFiles = 0;
        uint32_t numRemovedDirs = 0;
        removeDir_NoLock(dir, numRemovedDirs, numRemovedFiles);
        TRACE_INFO("Unlinked directory '{}', {} total files and {} dirs were unlinked", localPath, numRemovedFiles, numRemovedDirs);

        outReportToHighLevel = true;
    }
}

void DepotStructure::applyEvent_NoLock(const Event& evt, IDepotProblemReporter& err, bool& outReportToHighLevel)
{
    switch (evt.type)
    {
        case EventType::FileChanged:
            applyEvent_FileChanged_NoLock(evt.localPath, err, outReportToHighLevel);
            break;

        case EventType::FileAdded:
            applyEvent_FileAdded_NoLock(evt.localPath, err, outReportToHighLevel);
            break;

        case EventType::FileRemoved:
            applyEvent_FileRemoved_NoLock(evt.localPath, err, outReportToHighLevel);
            break;

        case EventType::DirectoryAdded:
            applyEvent_DirAdded_NoLock(evt.localPath, err, outReportToHighLevel);
            break;

        case EventType::DirectoryRemoved:
            applyEvent_DirRemoved_NoLock(evt.localPath, err, outReportToHighLevel);
            break;
    }
}

void DepotStructure::update(StringView prefixPath, Array<Event>& outEvents, IDepotProblemReporter& err)
{
    // process low-level file system events
    InplaceArray<Event, 10> events;
    gatherEvents_NoLock(prefixPath, events);

    // apply the low-level file system events to our in-memory depot structure
    // this will filter some events
    {
        auto lock = CreateLock(m_globalLock);

        for (const auto& evt : events)
        {
            bool reportToHighLevel = false;
            applyEvent_NoLock(evt, err, reportToHighLevel);

            if (reportToHighLevel)
                outEvents.pushBack(evt);
        }
    }
}

void DepotStructure::handleEvent(const DirectoryWatcherEvent& evt)
{
    auto lock = CreateLock(m_bufferedFileEventsLock);
    m_bufferedFileEvents.pushBack(evt);
}

//--

END_BOOMER_NAMESPACE()

