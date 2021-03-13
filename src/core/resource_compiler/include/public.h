/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "core_resource_compiler_glue.inl"

BEGIN_BOOMER_NAMESPACE()

//--

class IImportDepotLoader;
class IImportDepotSaver;

class IResourceImporter;
class IResourceImporterInterface;

struct ImportJobInfo;
struct ImportJobResult;

class ImportList;
typedef RefPtr<ImportList> ImportListPtr;

//--

class SourceAssetService;
class SourceAssetFingerprint;


class ISourceAsset;
typedef RefPtr<ISourceAsset> SourceAssetPtr;

class ISourceAssetFileSystem;
typedef RefPtr<ISourceAssetFileSystem> SourceAssetFileSystemPtr;

//--

/// import status of file
enum class ImportStatus : uint8_t
{
    Pending, // pending status, no job done yet
    Checking, // status is being checked
    Canceled, // processing was canceled
    Processing, // file is being imported
    Failed, // we finished processing resource but it failed
    FinishedUpTodate, // we finished processing resource and there was noting to import
    FinishedNewContent, // we finished processing resource and new content was saved
};

/// import flags
enum class ImportExtendedStatusBit : uint32_t
{
    Canceled,
    MetadataInvalid,
    NotImported,
    ImporterClassMissing,
    ImporterVersionChanged,
    ResourceClassMissing,
    ResourceVersionChanged,
    SourceFileMissing,
    SourceFileChanged,
    SourceFileNotReadable,
    ConfigurationChanged,
    UpToDate,
};

typedef BitFlags<ImportExtendedStatusBit> ImportExtendedStatusFlags;

/// result of asset file validation
enum class SourceAssetStatus : uint8_t
{
    Checking,
    ContentChanged,
    UpToDate,
    ReadFailure,
    Missing,
    Canceled,
};

//--

/// read only access to the current depot state
class CORE_RESOURCE_COMPILER_API IImportDepotLoader : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_IMPORT)

public:
    virtual ~IImportDepotLoader();

    // query resource ID of a depot file
    virtual ResourceID queryResourceID(StringView depotPath) const = 0;

    // load metadata of existing resource, should not be cached or reused
    virtual ResourceMetadataPtr loadExistingMetadata(StringView depotPath) const = 0;

    // load existing content of a resource, should not be cached or reused
    virtual ResourcePtr loadExistingResource(StringView depotPath) const = 0;

    // check if file exists
    virtual bool fileExists(StringView depotPath) const = 0;

    // find depot file
    virtual bool findFile(StringView depotPath, StringView fileName, uint32_t maxDepth, StringBuf& outFoundFileDepotPath, ResourceID* outFoundResourceID=nullptr) const = 0;

    //--

    // get loader for global depot
    static const IImportDepotLoader& GetGlobalDepotLoader();
};

//--

/// writable access to the depot, used to store results of import
class CORE_RESOURCE_COMPILER_API IImportDepotSaver : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_IMPORT)

public:
    virtual ~IImportDepotSaver();

    /// save metadata to the depot
    virtual bool saveMetadata(StringView depotPath, const ResourceMetadata* metadata) = 0;

    /// save resource content to the depot
    virtual bool saveResource(StringView depotPath, const IResource* data) = 0;

    //--

    // get saver for the global depot (direct, no save thread)
    static IImportDepotSaver& GetGlobalDepotSaver();
};

//--

/// notification interface for the queue
class CORE_RESOURCE_COMPILER_API IImportProgressTracker : public IProgressTracker
{
public:
    virtual ~IImportProgressTracker();

    virtual void jobAdded(const ImportJobInfo& info) {};
    virtual void jobStarted(StringView depotPath) {};
    virtual void jobFinished(StringView depotPath, ImportStatus status, double timeTaken) {};
    virtual void jobProgressUpdate(StringView depotPath, uint64_t currentCount, uint64_t totalCount, StringView text) {};

    std::atomic<uint32_t> numTotalJobs = 0;
    std::atomic<uint32_t> numImportedFiles = 0;
    std::atomic<uint32_t> numUpToDateFiles = 0;
    std::atomic<uint32_t> numFailedFiles = 0;
};

//--

// process import/reimport of a list of resources
//  - if "loader" is NULL the a default full depot access is used
//  - results are not saved anywhere and just passed in outResults
// NOTE: this should most likely be called from a thread :)
extern void CORE_RESOURCE_COMPILER_API ImportResources(IImportProgressTracker& progress, const Array<ImportJobInfo>& jobs, Array<ImportJobResult>& outResults, const IImportDepotLoader* loader = nullptr);

// process import/reimport of a list of resources, this should most likely be called from a thread
//  - if "loader" is NULL then a default full depot access is used
//  - is saver is NULL then a default full depot access is used
// NOTE: this should most likely be called from a thread :)
extern void CORE_RESOURCE_COMPILER_API ImportResources(IImportProgressTracker& progress, const Array<ImportJobInfo>& jobs, IImportDepotSaver* saver = nullptr, const IImportDepotLoader* loader = nullptr);

// reimport already loaded resource
// NOTE: nothing is written to disk, just a new resource data and metadata object is returned
// NOTE: this should most likely be called from a thread :)
extern bool CORE_RESOURCE_COMPILER_API ReimportResource(StringView depotPath, const IResource* existingResource, const ResourceMetadata* existingMetadata, ResourcePtr& outResource, ResourceMetadataPtr& outMetadata, IProgressTracker* localProgress = nullptr);

//--

END_BOOMER_NAMESPACE()
