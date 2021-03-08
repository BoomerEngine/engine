/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: service #]
***/

#include "build.h"
#include "depot.h"
#include "depotStructure.h"
#include "fileLoader.h"
#include "fileSaver.h"

#include "core/io/include/timestamp.h"
#include "core/io/include/fileHandle.h"
#include "core/app/include/commandline.h"
#include "core/containers/include/path.h"
#include "core/xml/include/xmlUtils.h"
#include "core/xml/include/xmlDocument.h"
#include "core/xml/include/xmlWrappers.h"

BEGIN_BOOMER_NAMESPACE()

///---

RTTI_BEGIN_TYPE_CLASS(DepotService);
RTTI_END_TYPE();

///---

static const StringView ENGINE_PREFIX = "/engine/";
static const StringView PROJECT_PREFIX = "/engine/";

DepotService::DepotService()
{}

StringBuf DepotService::queryDepotAbsolutePath(DepotType type) const
{
    const auto index = (int)type;
    if (index >= 0 && index < MAX_DEPOTS)
        return m_depots[index].absolutePath;

    return "";
}

bool DepotService::queryFileAbsolutePath(StringView depotPath, StringBuf& outAbsolutePath) const
{
    DEBUG_CHECK_RETURN_EX_V(ValidateDepotFilePath(depotPath), "Invalid path", false);

    for (const auto& depot : m_depots)
    {
        if (depot.prefix && depotPath.beginsWith(depot.prefix))
        {
            const auto remainingPath = depotPath.subString(depot.prefix.length());
            outAbsolutePath = TempString("{}{}", depot.absolutePath, remainingPath);
            return true;
        }
    }

    return false;
}

bool DepotService::queryFileDepotPath(StringView absolutePath, StringBuf& outDeptotPath) const
{
    for (const auto& depot : m_depots)
    {
        if (depot.prefix && absolutePath.beginsWith(depot.absolutePath))
        {
            const auto remainingPath = absolutePath.afterFirst(depot.absolutePath);
            outDeptotPath = TempString("{}{}", depot.prefix, remainingPath);
            return true;
        }
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

ReadFileHandlePtr DepotService::createFileReader(StringView depotPath, TimeStamp* outTimestamp) const
{
    StringBuf absolutePath;
    if (!queryFileAbsolutePath(depotPath, absolutePath))
        return nullptr;

    return OpenForReading(absolutePath, outTimestamp);
}

WriteFileHandlePtr DepotService::createFileWriter(StringView depotPath) const
{
    StringBuf absolutePath;
    if (!queryFileAbsolutePath(depotPath, absolutePath))
        return nullptr;

    return OpenForWriting(absolutePath);
}

AsyncFileHandlePtr DepotService::createFileAsyncReader(StringView depotPath, TimeStamp* outTimestamp) const
{
    StringBuf absolutePath;
    if (!queryFileAbsolutePath(depotPath, absolutePath))
        return nullptr;

    return OpenForAsyncReading(absolutePath, outTimestamp);
}

void DepotService::enumDirectoriesAtPath(StringView depotPath, const std::function<void(StringView) >& enumFunc) const
{
    DEBUG_CHECK_RETURN_EX(ValidateDepotDirPath(depotPath), "Invalid directory path");

    if (depotPath == "/")
    {
        for (const auto& depot : m_depots)
        {
            if (depot.prefix)
            {
                const auto dirName = depot.prefix.subString(1, depot.prefix.length() - 2);
                enumFunc(dirName);
            }
        }
    }
    else
    {
        for (const auto& depot : m_depots)
        {
            if (depot.prefix && depotPath.beginsWith(depot.prefix))
            {
                const auto remainingPath = depotPath.subString(depot.prefix.length());
                depot.structure->enumDirectories(remainingPath, enumFunc);
            }
        }
    }
}

void DepotService::enumFilesAtPath(StringView depotPath, const std::function<void(StringView)>& enumFunc) const
{
    DEBUG_CHECK_RETURN_EX(ValidateDepotDirPath(depotPath), "Invalid directory path");

    for (const auto& depot : m_depots)
    {
        if (depot.prefix && depotPath.beginsWith(depot.prefix))
        {
            const auto remainingPath = depotPath.subString(depot.prefix.length());
            depot.structure->enumFiles(remainingPath, enumFunc);
        }
    }
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

    DepotErrorCapture err;

    {
        auto& depot = m_depots[(int)DepotType::Engine];
        depot.absolutePath = TempString("{}data/depot/", engineDir);
        TRACE_INFO("Engine depot directory: '{}'", depot.absolutePath);

        depot.prefix = "/engine/";
        depot.structure = new DepotStructure(depot.absolutePath);
        depot.structure->scan(err);
    }

    auto projectDir = cmdLine.singleValue("projectDir");
    if (!projectDir.empty())
    {
        auto& depot = m_depots[(int)DepotType::Project];

        depot.absolutePath = TempString("{}data/depot/", projectDir);
        //depot.absolutePath = "Z:\\projects\\w3\\r4data\\";
        TRACE_INFO("Project depot directory: '{}'", depot.absolutePath);

        depot.prefix = "/project/";
        depot.structure = new DepotStructure(depot.absolutePath);
        depot.structure->scan(err);
    }

    return app::ServiceInitializationResult::Finished;
}

void DepotService::onShutdownService()
{
    for (auto& depot : m_depots)
    {
        delete depot.structure;
        depot.structure = nullptr;
        depot.absolutePath = StringBuf();
        depot.prefix = StringBuf();
    }    
}

void DepotService::onSyncUpdate()
{
    // TODO: process updates
}

//--

bool DepotService::loadFileToBuffer(StringView depotPath, Buffer& outContent, TimeStamp* timestamp) const
{
    DEBUG_CHECK_RETURN_EX_V(depotPath, "Invalid path", false);
    DEBUG_CHECK_RETURN_EX_V(ValidateDepotFilePath(depotPath), "Invalid path", false);

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
    DEBUG_CHECK_RETURN_EX_V(depotPath, "Invalid path", false);
    DEBUG_CHECK_RETURN_EX_V(ValidateDepotFilePath(depotPath), "Invalid path", false);

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

bool DepotService::loadFileToXML(StringView depotPath, xml::DocumentPtr& outContent, TimeStamp* timestamp /*= nullptr*/) const
{
    DEBUG_CHECK_RETURN_EX_V(depotPath, "Invalid path", false);
    DEBUG_CHECK_RETURN_EX_V(ValidateDepotFilePath(depotPath), "Invalid path", false);

    Buffer data;
    if (!loadFileToBuffer(depotPath, data, timestamp))
        return false;

    if (auto content = xml::LoadDocument(xml::ILoadingReporter::GetDefault(), data))
    {
        outContent = content;
        return true;
    }

    return false;
}

bool DepotService::loadFileToXMLObject(StringView depotPath, ObjectPtr& outObject, ClassType expectedClass /*= nullptr*/, TimeStamp* timestamp /*= nullptr*/) const
{
    DEBUG_CHECK_RETURN_EX_V(depotPath, "Invalid path", false);
    DEBUG_CHECK_RETURN_EX_V(ValidateDepotFilePath(depotPath), "Invalid path", false);

    xml::DocumentPtr doc;
    if (!loadFileToXML(depotPath, doc, timestamp))
        return false;

    if (const auto obj = LoadObjectFromXML(doc, expectedClass.cast<IObject>()))
    {
        outObject = obj;
        return true;
    }

    return false;
}

bool DepotService::loadFileToResource(StringView depotPath, ResourcePtr& outResource, ResourceClass expectedClass /*= nullptr*/, bool loadImports /*=true*/, TimeStamp* outTimestamp /*= nullptr*/) const
{
    DEBUG_CHECK_RETURN_EX_V(depotPath, "Invalid path", false);
    DEBUG_CHECK_RETURN_EX_V(ValidateDepotFilePath(depotPath), "Invalid path", false);

    TimeStamp timestamp;
    if (const auto file = this->createFileAsyncReader(depotPath, &timestamp))
    {
        FileLoadingContext context;
        context.loadImports = loadImports;
        context.knownMainFileSize = 0; 
        context.resourceLoadPath = StringBuf(depotPath);
        context.expectedRootClass = expectedClass ? expectedClass : IResource::GetStaticClass();

        FileLoadingResult result;
        if (LoadFile(file, context, result))
        {
            if (result.roots.size() == 1)
            {
                auto root = rtti_cast<IResource>(result.roots[0]);

                if (expectedClass && !root->is(expectedClass))
                {
                    TRACE_WARNING("Loaded file '{}' is '{}' not expected '{}'", depotPath, root->cls(), expectedClass);
                    root = nullptr;
                }

                if (root)
                {
                    outResource = root;

                    if (outTimestamp)
                        *outTimestamp = timestamp;

                    return true;
                }
            }
        }
    }

    return false;
}

//--

bool DepotService::saveFileFromBuffer(StringView depotPath, Buffer content, TimeStamp* manualTimestamp /*= nullptr*/) const
{
    DEBUG_CHECK_RETURN_EX_V(ValidateDepotFilePath(depotPath), "Invalid path", false);

    auto file = createFileWriter(depotPath);
    DEBUG_CHECK_RETURN_EX_V(file, "Unable to create file writer", false);

    const auto numWritten = file->writeSync(content.data(), content.size());
    if (numWritten != content.size())
    {
        TRACE_ERROR("Failed to write data to '{}', saved {}, expected {}", depotPath, numWritten, content.size());
        file->discardContent();
        return false;
    }

    return true;
}

bool DepotService::saveFileFromString(StringView depotPath, StringView content, TimeStamp* manualTimestamp /*= nullptr*/) const
{
    DEBUG_CHECK_RETURN_EX_V(ValidateDepotFilePath(depotPath), "Invalid path", false);

    // TODO: encoding ?

    auto file = createFileWriter(depotPath);
    DEBUG_CHECK_RETURN_EX_V(file, "Unable to create file writer", false);

    const auto numWritten = file->writeSync(content.data(), content.length());
    if (numWritten != content.length())
    {
        TRACE_ERROR("Failed to write data to '{}', saved {}, expected {}", depotPath, numWritten, content.length());
        file->discardContent();
        return false;
    }

    return true;
}

bool DepotService::saveFileFromXML(StringView depotPath, const xml::IDocument* doc, TimeStamp* manualTimestamp /*= nullptr*/) const
{
    DEBUG_CHECK_RETURN_EX_V(ValidateDepotFilePath(depotPath), "Invalid path", false);
    DEBUG_CHECK_RETURN_EX_V(doc, "Nothing to save", false);

    // open the writer
    auto file = createFileWriter(depotPath);
    DEBUG_CHECK_RETURN_EX_V(doc, "Failed to create file writer", false);

    if (!xml::SaveDocument(doc, file))
    {
        file->discardContent();
        return false;
    }

    return true;
}

bool DepotService::saveFileFromXMLObject(StringView depotPath, const IObject* data, TimeStamp* manualTimestamp /*= nullptr*/) const
{
    DEBUG_CHECK_RETURN_EX_V(ValidateDepotFilePath(depotPath), "Invalid path", false);

    auto ret = xml::CreateDocument("object");
    DEBUG_CHECK_RETURN_EX_V(ret, "Unable to create XML document", false);

    if (data)
    {
        auto rootNode = xml::Node(ret);
        rootNode.writeAttribute("class", data->cls().name().view());
        data->writeXML(rootNode);
    }

    return saveFileFromXML(depotPath, ret, manualTimestamp);
}

bool DepotService::saveFileFromResource(StringView depotPath, const IResource* data, TimeStamp* manualTimestamp /*= nullptr*/) const
{
    DEBUG_CHECK_RETURN_EX_V(ValidateDepotFilePath(depotPath), "Invalid path", false);
    DEBUG_CHECK_RETURN_EX_V(data, "Nothing to save", false);

    auto file = createFileWriter(depotPath);
    DEBUG_CHECK_RETURN_EX_V(file, "Unable to create file writer", false);

    FileSavingContext context;
    context.format = FileSaveFormat::ProtectedBinaryStream;
    context.rootObjects.pushBack(data);

    FileSavingResult result;
    if (!SaveFile(file, context, result))
    {
        file->discardContent();
        return false;
    }

    return true;
}

//--

bool DepotService::resolvePathForID(const ResourceID& id, StringBuf& outLoadPath) const
{
    for (const auto& depot : m_depots)
    {
        StringBuf localPath;
        if (depot.structure && depot.structure->resolvePathForID(id, localPath))
        {
            outLoadPath = TempString("{}{}", depot.prefix, localPath);
            return true;
        }
    }

    TRACE_WARNING("No depot path resolved for ID '{}'", id);
    return false;
}

bool DepotService::resolveIDForPath(StringView depotPath, ResourceID& outID) const
{
    for (const auto& depot : m_depots)
    {
        if (depot.prefix && depotPath.beginsWith(depot.prefix))
        {
            const auto remainingPath = depotPath.subString(depot.prefix.length());
            if (depot.structure->resolveIDForPath(remainingPath, outID))
                return true;
        }
    }

    return false;
}

//--

END_BOOMER_NAMESPACE()

