/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: service #]
***/

#include "build.h"
#include "depotService.h"
#include "base/io/include/ioFileHandle.h"
#include "base/app/include/commandline.h"

BEGIN_BOOMER_NAMESPACE(base)

///---

RTTI_BEGIN_TYPE_CLASS(DepotService);
RTTI_END_TYPE();

///---

DepotService::DepotService()
{}

bool DepotService::queryFileAbsolutePath(StringView depotPath, StringBuf& outAbsolutePath) const
{
    DEBUG_CHECK_RETURN_EX_V(ValidateDepotPath(depotPath, DepotPathClass::AnyAbsolutePath), "Invalid path", false);

    if (m_engineDepotPath && depotPath.beginsWith("/engine/"))
    {
        outAbsolutePath = TempString("{}{}", m_engineDepotPath, depotPath.subString(8));
        return true;
    }

    if (m_projectDepotPath && depotPath.beginsWith("/project/"))
    {
        outAbsolutePath = TempString("{}{}", m_projectDepotPath, depotPath.subString(9));
        return true;
    }

    return false;
}

bool DepotService::queryFileDepotPath(StringView absolutePath, StringBuf& outDeptotPath) const
{
    if (m_engineDepotPath && absolutePath.beginsWith(m_engineDepotPath))
    {
        outDeptotPath = base::TempString("/engine/{}", absolutePath.afterFirst(m_engineDepotPath));
        return true;
    }

    if (m_projectDepotPath && absolutePath.beginsWith(m_projectDepotPath))
    {
        outDeptotPath = base::TempString("/project/{}", absolutePath.afterFirst(m_projectDepotPath));
        return true;
    }

    return false;
}

bool DepotService::queryFileTimestamp(StringView depotPath, io::TimeStamp& outTimestamp) const
{
    base::StringBuf absolutePath;
    if (!queryFileAbsolutePath(depotPath, absolutePath))
        return false;

    return io::FileTimeStamp(absolutePath, outTimestamp);
}

io::ReadFileHandlePtr DepotService::createFileReader(StringView depotPath) const
{
    base::StringBuf absolutePath;
    if (!queryFileAbsolutePath(depotPath, absolutePath))
        return nullptr;

    return io::OpenForReading(absolutePath);
}

io::WriteFileHandlePtr DepotService::createFileWriter(StringView depotPath) const
{
    base::StringBuf absolutePath;
    if (!queryFileAbsolutePath(depotPath, absolutePath))
        return nullptr;

    return io::OpenForWriting(absolutePath);
}

io::AsyncFileHandlePtr DepotService::createFileAsyncReader(StringView depotPath) const
{
    base::StringBuf absolutePath;
    if (!queryFileAbsolutePath(depotPath, absolutePath))
        return nullptr;

    return io::OpenForAsyncReading(absolutePath);
}

bool DepotService::enumDirectoriesAtPath(StringView rawDirectoryPath, const std::function<bool(const DirectoryInfo& info) >& enumFunc) const
{
    DEBUG_CHECK_RETURN_EX_V(ValidateDepotPath(rawDirectoryPath, DepotPathClass::AbsoluteDirectoryPath), "Invalid directory path", false);

    if (rawDirectoryPath == "/")
    {
        if (m_engineDepotPath)
        {
            DirectoryInfo info;
            info.fileSystemRoot = true;
            info.name = "engine";
            if (enumFunc(info))
                return true;
        }

        if (m_projectDepotPath)
        {
            DirectoryInfo info;
            info.fileSystemRoot = true;
            info.name = "project";
            if (enumFunc(info))
                return true;
        }
    }
    else
    {
        base::StringBuf absolutePath;
        if (queryFileAbsolutePath(rawDirectoryPath, absolutePath))
        {
            return io::FindSubDirs(absolutePath, [&enumFunc](StringView name)
                {
                    DirectoryInfo info;
                    info.fileSystemRoot = true;
                    info.name = name;
                    return enumFunc(info);
                });
        }                
    }

    return false;
}

bool DepotService::enumFilesAtPath(StringView rawDirectoryPath, const std::function<bool(const FileInfo& info)>& enumFunc) const
{
    DEBUG_CHECK_RETURN_EX_V(ValidateDepotPath(rawDirectoryPath, DepotPathClass::AbsoluteDirectoryPath), "Invalid directory path", false);

    if (rawDirectoryPath != "/")
    {
        base::StringBuf absolutePath;
        if (queryFileAbsolutePath(rawDirectoryPath, absolutePath))
        {
            return io::FindLocalFiles(absolutePath, "*.*", [&enumFunc](StringView name)
                {
                    FileInfo info;
                    info.name = name;
                    return enumFunc(info);
                });
        }
    }

    return false;
}

//--

struct DepotPathAppendBuffer
{
    static const uint32_t MAX_LENGTH = 512;

    char m_buffer[MAX_LENGTH + 1]; // NOTE: not terminated with zero while building, only at the end
    uint32_t m_pos = 0;

    DepotPathAppendBuffer()
    {}

    ALWAYS_INLINE bool empty() const
    {
        return m_pos == 0;
    }

    ALWAYS_INLINE StringView view() const
    {
        return StringView(m_buffer, m_pos);
    }

    bool append(StringView text, uint32_t& outPos)
    {
        DEBUG_CHECK_RETURN_V(m_pos + text.length() <= MAX_LENGTH, false); // should not happen in healthy system

        outPos = m_pos;

        memcpy(m_buffer + m_pos, text.data(), text.length());
        m_pos += text.length();
        return true;
    }

    void revert(uint32_t pos)
    {
        m_pos = pos;
    }
};

#if 0
bool DepotStructure::findFileInternal(DepotPathAppendBuffer& path, StringView fileName, uint32_t maxDepth, StringBuf& outFoundFileDepotPath) const
{
    if (maxDepth > 0)
    {
        if (auto fileSystemListAtDir = m_fileSystemsAtDirectory.find(path.view()))
        {
            for (auto& fsEntry : *fileSystemListAtDir)
            {
                uint32_t prevPos = 0;
                if (path.append(TempString("{}/", fsEntry.name), prevPos))
                {
                    if (fsEntry.fileSystem)
                    {
                        ASSERT(path.view().beginsWith(fsEntry.fileSystem->mountPoint().view()));
                        if (findFileInternal(fsEntry.fileSystem, path, fileName, maxDepth - 1, outFoundFileDepotPath))
                            return true;
                    }
                    else
                    {
                        if (findFileInternal(path, fileName, maxDepth - 1, outFoundFileDepotPath))
                            return true;
                    }

                    path.revert(prevPos);
                }
            }
        }
    }

    return false;
}

bool DepotStructure::findFileInternal(const DepotFileSystem* fs, DepotPathAppendBuffer& path, StringView matchFileName, uint32_t maxDepth, StringBuf& outFoundFileDepotPath) const
{
    const auto mountPoint = fs->mountPoint().view();
    const auto relativePath = path.view().subString(mountPoint.length());

    if (fs->fileSystem().enumFilesAtPath(relativePath, [&outFoundFileDepotPath, &path, matchFileName](StringView name)
        {
            if (name.caseCmp(matchFileName) == 0)
            {
                outFoundFileDepotPath = TempString("{}{}", path.view(), name);
                return true;
            }

            return false;
        }))
        return true;

        if (maxDepth > 0)
        {
            maxDepth -= 1;

            if (fs->fileSystem().enumDirectoriesAtPath(relativePath, [this, fs, &outFoundFileDepotPath, &path, matchFileName, maxDepth](StringView name)
                {
                    uint32_t pos = 0;
                    if (path.append(TempString("{}/", name), pos))
                    {
                        if (findFileInternal(fs, path, matchFileName, maxDepth, outFoundFileDepotPath))
                            return true;

                        path.revert(pos);
                    }

                    return false;
                }))
                return true;
        }

        return false;
}
#endif

bool DepotService::findFile(StringView depotPath, StringView fileName, uint32_t maxDepth, StringBuf& outFoundFileDepotPath) const
{
    DEBUG_CHECK_RETURN_EX_V(ValidateDepotPath(depotPath, DepotPathClass::AbsoluteDirectoryPath), "Invalid directory path", false);

    return false;
}

//--

app::ServiceInitializationResult DepotService::onInitializeService(const app::CommandLine& cmdLine)
{
    const auto engineDir = io::SystemPath(io::PathCategory::EngineDir);
    m_engineDepotPath = TempString("{}data/depot/", engineDir);
    TRACE_INFO("Engine depot directory: '{}'", m_engineDepotPath);

    const auto& projectDir = cmdLine.singleValue("projectDir");
    if (!projectDir.empty())
    {
        if (io::FileExists(TempString("{}project.xml", projectDir)))
        {
            m_projectDepotPath = TempString("{}data/depot/", projectDir);
            TRACE_INFO("Project depot directory: '{}'", m_projectDepotPath);
        }
    }

    m_engineObserver = io::CreateDirectoryWatcher(m_engineDepotPath);
    m_engineObserver->attachListener(this);

    return app::ServiceInitializationResult::Finished;
}

void DepotService::onShutdownService()
{

}

void DepotService::onSyncUpdate()
{

}

//--

void DepotService::handleEvent(const io::DirectoryWatcherEvent& evt)
{
    StringBuf depotPath;
    if (queryFileDepotPath(evt.path, depotPath))
    {
        switch (evt.type)
        {

        case io::DirectoryWatcherEventType::FileAdded:
            TRACE_INFO("Depot file was reported as added", depotPath);
            DispatchGlobalEvent(m_eventKey, EVENT_DEPOT_FILE_ADDED, depotPath);
            break;

        case io::DirectoryWatcherEventType::FileRemoved:
            TRACE_INFO("Depot file was reported as removed", depotPath);
            DispatchGlobalEvent(m_eventKey, EVENT_DEPOT_FILE_REMOVED, depotPath);
            break;

        case io::DirectoryWatcherEventType::DirectoryAdded:
            TRACE_INFO("Depot directory '{}' was reported as added", depotPath);
            DispatchGlobalEvent(m_eventKey, EVENT_DEPOT_DIRECTORY_ADDED, depotPath);
            break;

        case io::DirectoryWatcherEventType::DirectoryRemoved:
            TRACE_INFO("Depot directory '{}' was reported as removed", depotPath);
            DispatchGlobalEvent(m_eventKey, EVENT_DEPOT_DIRECTORY_REMOVED, depotPath);
            break;

        case io::DirectoryWatcherEventType::FileContentChanged:
            TRACE_INFO("Depot file '{}' was reported as changed", depotPath);
            DispatchGlobalEvent(m_eventKey, EVENT_DEPOT_FILE_CHANGED, depotPath);
            break;

        case io::DirectoryWatcherEventType::FileMetadataChanged:
            break;
        }
    }
}

//--

bool DepotService::loadFileToBuffer(StringView depotPath, Buffer& outContent, io::TimeStamp* timestamp) const
{
    if (auto file = createFileReader(depotPath))
    {
        auto size = file->size();
        if (auto data = Buffer::Create(POOL_TEMP, size, 16))
        {
            if (size == file->readSync(data.data(), size))
            {
                outContent = data;
                return true;
            }
        }
    }

    return false;
}

bool DepotService::loadFileToString(StringView depotPath, StringBuf& outContent, io::TimeStamp* timestamp) const
{
    if (auto file = createFileReader(depotPath))
    {
        auto size = file->size();

        auto str = StringBuf(size);
        if (size == file->readSync((void*)str.c_str(), size))
        {
            outContent = str;
            return true;
        }
    }

    return false;
}

END_BOOMER_NAMESPACE(base)
