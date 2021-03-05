/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: service #]
***/

#include "build.h"
#include "depot.h"

#include "core/io/include/fileHandle.h"
#include "core/app/include/commandline.h"
#include "core/containers/include/path.h"
#include "depotStructure.h"

BEGIN_BOOMER_NAMESPACE()

///---

RTTI_BEGIN_TYPE_CLASS(DepotService);
RTTI_END_TYPE();

///---

DepotService::DepotService()
{}

bool DepotService::queryFileAbsolutePath(StringView depotPath, StringBuf& outAbsolutePath) const
{
    DEBUG_CHECK_RETURN_EX_V(ValidateDepotFilePath(depotPath), "Invalid path", false);

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
        outDeptotPath = TempString("/engine/{}", absolutePath.afterFirst(m_engineDepotPath));
        return true;
    }

    if (m_projectDepotPath && absolutePath.beginsWith(m_projectDepotPath))
    {
        outDeptotPath = TempString("/project/{}", absolutePath.afterFirst(m_projectDepotPath));
        return true;
    }

    return false;
}

bool DepotService::queryFileTimestamp(StringView depotPath, TimeStamp& outTimestamp) const
{
    StringBuf absolutePath;
    if (!queryFileAbsolutePath(depotPath, absolutePath))
        return false;

    return FileTimeStamp(absolutePath, outTimestamp);
}

ReadFileHandlePtr DepotService::createFileReader(StringView depotPath) const
{
    StringBuf absolutePath;
    if (!queryFileAbsolutePath(depotPath, absolutePath))
        return nullptr;

    return OpenForReading(absolutePath);
}

WriteFileHandlePtr DepotService::createFileWriter(StringView depotPath) const
{
    StringBuf absolutePath;
    if (!queryFileAbsolutePath(depotPath, absolutePath))
        return nullptr;

    return OpenForWriting(absolutePath);
}

AsyncFileHandlePtr DepotService::createFileAsyncReader(StringView depotPath) const
{
    StringBuf absolutePath;
    if (!queryFileAbsolutePath(depotPath, absolutePath))
        return nullptr;

    return OpenForAsyncReading(absolutePath);
}

bool DepotService::enumDirectoriesAtPath(StringView rawDirectoryPath, const std::function<bool(const DirectoryInfo& info) >& enumFunc) const
{
    DEBUG_CHECK_RETURN_EX_V(ValidateDepotDirPath(rawDirectoryPath), "Invalid directory path", false);

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
        StringBuf absolutePath;
        if (queryFileAbsolutePath(rawDirectoryPath, absolutePath))
        {
            return FindSubDirs(absolutePath, [&enumFunc](StringView name)
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
    DEBUG_CHECK_RETURN_EX_V(ValidateDepotDirPath(rawDirectoryPath), "Invalid directory path", false);

    if (rawDirectoryPath != "/")
    {
        StringBuf absolutePath;
        if (queryFileAbsolutePath(rawDirectoryPath, absolutePath))
        {
            return FindLocalFiles(absolutePath, "*.*", [&enumFunc](StringView name)
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
    DEBUG_CHECK_RETURN_EX_V(ValidateDepotDirPath(depotPath), "Invalid directory path", false);

    return false;
}

//--

class DepotErrorCapture : public IDepotProblemReporter
{
public:
    virtual void reportInvalidFile(StringView path) override
    {
        TRACE_ERROR("Depot file '{}' is invalid");
    }

    virtual void reportDuplicatedID(ResourceID id, StringView path, StringView otherPath) override
    {
        TRACE_ERROR("Duplicated depot file ID found at '{}' and '{}'", id, path, otherPath);
    }
};

app::ServiceInitializationResult DepotService::onInitializeService(const app::CommandLine& cmdLine)
{
    const auto engineDir = SystemPath(PathCategory::EngineDir);
    m_engineDepotPath = TempString("{}data/depot/", engineDir);
    TRACE_INFO("Engine depot directory: '{}'", m_engineDepotPath);

    DepotErrorCapture err;

    m_engineDepotStructure = new DepotStructure(m_engineDepotPath);
    m_engineDepotStructure->scan(err);

    const auto& projectDir = cmdLine.singleValue("projectDir");
    if (!projectDir.empty())
    {
        if (FileExists(TempString("{}project.xml", projectDir)))
        {
            m_projectDepotPath = TempString("{}data/depot/", projectDir);
            TRACE_INFO("Project depot directory: '{}'", m_projectDepotPath);

            m_projectDepotStructure = new DepotStructure(m_projectDepotPath);
            m_projectDepotStructure->scan(err);
        }
    }

    {
        auto test = new DepotStructure("Z://projects//w3//r4data//");
        test->scan(err);
    }

    return app::ServiceInitializationResult::Finished;
}

void DepotService::onShutdownService()
{
    delete m_projectDepotStructure;
    m_projectDepotStructure = nullptr;

    delete m_engineDepotStructure;
    m_engineDepotStructure = nullptr;
}

void DepotService::onSyncUpdate()
{
    // TODO: process updates
}

//--

bool DepotService::loadFileToBuffer(StringView depotPath, Buffer& outContent, TimeStamp* timestamp) const
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

bool DepotService::loadFileToString(StringView depotPath, StringBuf& outContent, TimeStamp* timestamp) const
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

bool DepotService::resolvePathForID(const ResourceID& id, StringBuf& outLoadPath) const
{
    if (m_projectDepotStructure)
    {
        StringBuf path;
        if (m_projectDepotStructure->resolveID(id, path))
        {
            outLoadPath = TempString("{}{}", m_projectDepotPath, path);
            return true;
        }
    }

    if (m_engineDepotStructure)
    {
        StringBuf path;
        if (m_engineDepotStructure->resolveID(id, path))
        {
            outLoadPath = TempString("{}{}", m_engineDepotPath, path);
            return true;
        }
    }

    TRACE_WARNING("No depot path resolved for ID '{}'", id);
    return false;
}

bool DepotService::resolveIDForPath(StringView depotPath, ResourceID& outID) const
{
    return false;
}

END_BOOMER_NAMESPACE()

