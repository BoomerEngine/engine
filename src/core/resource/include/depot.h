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

    /// get mount path for given depot, can be empty if depot was not mounted (especially project)
    StringBuf queryDepotAbsolutePath(DepotType type) const;

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
    ReadFileHandlePtr createFileReader(StringView depotPath, TimeStamp* outTimestamp=nullptr) const;

    /// create a writer for the file's content
    /// NOTE: fails if the file system was not writable
    WriteFileHandlePtr createFileWriter(StringView depotPath) const;

    /// create an ASYNC reader for the file's content
    /// NOTE: creating a reader may take some time
    AsyncFileHandlePtr createFileAsyncReader(StringView depotPath, TimeStamp* outTimestamp = nullptr) const;

    //--

    /// get child directories at given path
    /// NOTE: virtual directories for the mounted file systems are reported as well
    void enumDirectoriesAtPath(StringView depotPath, const std::function<void(StringView) >& enumFunc) const;

    /// get files at given path (should be absolute depot DIRECTORY path)
    void enumFilesAtPath(StringView depotPath, const std::function<void(StringView)>& enumFunc) const;

    //--

    // find file in depot
    bool findFile(StringView depotPath, StringView fileName, uint32_t maxDepth, StringBuf& outFoundFileDepotPath) const;

    //--

    // load file content to buffer
    bool loadFileToBuffer(StringView depotPath, Buffer& outContent, TimeStamp* timestamp = nullptr) const;

    // load file content to string
    bool loadFileToString(StringView depotPath, StringBuf& outContent, TimeStamp* timestamp = nullptr) const;

    // load file to XML
    bool loadFileToXML(StringView depotPath, xml::DocumentPtr& outContent, TimeStamp* timestamp = nullptr) const;

    // load content directly (via simple XML serialization)
    bool loadFileToXMLObject(StringView depotPath, ObjectPtr& outObject, ClassType expectedClass = nullptr, TimeStamp* timestamp = nullptr) const;

    // load content directly (via simple XML serialization)
    template< typename T >
    INLINE bool loadFileToXMLObject(StringView depotPath, RefPtr<T>& outObject, TimeStamp* timestamp = nullptr) const
    {
        return loadFileToXMLObject(depotPath, *(ObjectPtr*) &outObject, T::GetStaticClass(), timestamp);
    }

    // load a resource directly (uncached)
    bool loadFileToResource(StringView depotPath, ResourcePtr& outResource, ResourceClass expectedClass = nullptr, bool loadImports = true, TimeStamp* timestamp = nullptr) const;

    // load a resource directly (uncached)
    template< typename T >
    INLINE bool loadFileToResource(StringView depotPath, RefPtr<T>& outObject, bool loadImports = true, TimeStamp* timestamp = nullptr) const
    {
        return loadFileToResource(depotPath, *(ResourcePtr*)&outObject, T::GetStaticClass(), loadImports, timestamp);
    }

    //--

    // NOTE: all saving is done via temporary file: 
    //  - first a temp file is created and content is written there
    //  - then the file is closed, opened again and verified
    //  - then old file is renamed to .bak
    //  - then temp file is moved into new place (atomically)
    //  - only after file was moved the original .bak file is deleted

    // save buffer to file in the depot
    bool saveFileFromBuffer(StringView depotPath, Buffer content, TimeStamp* manualTimestamp = nullptr) const;

    // save string (assuming UTF8) to a file in the depot
    bool saveFileFromString(StringView depotPath, StringView content, TimeStamp* manualTimestamp = nullptr) const;

    // save XML to a file in the depot
    bool saveFileFromXML(StringView depotPath, const xml::IDocument* doc, TimeStamp* manualTimestamp = nullptr) const;

    // save object to an XML file (usually used for xmeta)
    bool saveFileFromXMLObject(StringView depotPath, const IObject* data, TimeStamp* manualTimestamp = nullptr) const;

    // save resource to a depot file
    bool saveFileFromResource(StringView depotPath, const IResource* data, TimeStamp* manualTimestamp = nullptr) const;

    //--

    // find load path for given resource ID
    bool resolvePathForID(const ResourceID& id, StringBuf& outLoadPath) const;

    // create or retrieve a resource ID for given depot path
    bool resolveIDForPath(StringView depotPath, ResourceID& outID) const;

    //--

private:
    struct DepotInfo
    {
        StringBuf absolutePath; // path on disk, ends with "/"
        StringBuf prefix; // "/engine/"
        DepotStructure* structure = nullptr;
    };

    static const uint32_t MAX_DEPOTS = 2;

    DepotInfo m_depots[MAX_DEPOTS];

    //--

    virtual app::ServiceInitializationResult onInitializeService(const app::CommandLine& cmdLine) override final;
    virtual void onShutdownService() override final;
    virtual void onSyncUpdate() override final;
};

//---

END_BOOMER_NAMESPACE()
