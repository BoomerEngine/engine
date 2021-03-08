/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//--

struct SourceAssetCache;

/// service that hosts all source asset file systems that can be used to import resources
/// NOTE: this is global/shared service
class CORE_RESOURCE_COMPILER_API SourceAssetService : public app::ILocalService
{
    RTTI_DECLARE_VIRTUAL_CLASS(SourceAssetService, app::ILocalService);

public:
    SourceAssetService();
    virtual ~SourceAssetService();

    // NOTE: the asset import paths start with the FS mount point, e.g.:
    // "LOCAL:/z/assets/test.fbx"
    // "PROJECT:/buildings/houses/house1.fbx"
    // "HL2:/models/alyx/alyx.mdl"

    //--

    // check if file exists
    bool fileExists(StringView assetImportPath) const;

    // translate absolute path to a asset import path
    // NOTE: if multiple file systems cover the same path we choose the shortest representation
    // NOTE: returned path is in the "assetImportPath" format: "LOCAL:/z/assets/test.fbx", etc.
    bool translateAbsolutePath(StringView absolutePath, StringBuf& outFileSystemPath) const;

    // translate import path to a context path (mostly for printing errors)
    bool resolveContextPath(StringView assetImportPath, StringBuf& outContextPath) const;

    /// get child directories at given path
    bool enumDirectoriesAtPath(StringView assetImportPath, const std::function<bool(StringView)>& enumFunc) const;

    /// get files at given path
    bool enumFilesAtPath(StringView assetImportPath, const std::function<bool(StringView)>& enumFunc) const;

    /// enumerate file system "roots" (ie. LOCAL, HL2, etc)
    bool enumRoots(const std::function<bool(StringView)>& enumFunc) const;

    //--

    /// validate source file 
    /// NOTE: this may take long time, run on fiber
    CAN_YIELD SourceAssetStatus checkFileStatus(StringView assetImportPath, const TimeStamp& lastKnownTimestamp, const SourceAssetFingerprint& lastKnownCRC, IProgressTracker* progress = nullptr);

    //--

    /// compile a base resource import configuration for asset of given type imported from given source folder
    void collectImportConfiguration(ResourceConfiguration* config, StringView sourceAssetPath);

    //--

    // load raw content of a file, returns the CRC of the data as well
    // NOTE: this load is not buffered 
    Buffer loadRawContent(StringView assetImportPath, TimeStamp& outTimestamp, SourceAssetFingerprint& outFingerprint) const;

    /// load source asset and cache the result, better for large assets that are slow to load and may be loaded many times (ie. importing mesh from FBX and later materials from the same one)
    // NOTE: if asset is already loaded it will be returned without reloading (unless it changed on disk)
    SourceAssetPtr loadAssetCached(StringView assetImportPath, TimeStamp& outTimestamp, SourceAssetFingerprint& outFingerprint);

    /// load source asset without caching anything, does not block, better for small assets
    SourceAssetPtr loadAssetUncached(StringView assetImportPath, TimeStamp& outTimestamp, SourceAssetFingerprint& outFingerprint);

    //--

protected:
    virtual app::ServiceInitializationResult onInitializeService(const app::CommandLine& cmdLine) override final;
    virtual void onShutdownService() override final;
    virtual void onSyncUpdate() override final;

    //--

    struct FileSystemInfo
    {
        StringBuf prefix; // LOCAL:
        SourceAssetFileSystemPtr fileSystem;
    };

    Array<FileSystemInfo> m_fileSystems;

    void createFileSystems();
    void destroyFileSystems();

    const ISourceAssetFileSystem* resolveFileSystem(StringView assetImportPath, StringView& outFileSystemPath) const;

    //--

    Mutex m_cacheLock;
    SourceAssetCache* m_cache = nullptr;

    //--

    NativeTimePoint m_nextSystemUpdate;

    //---
};

//--

END_BOOMER_NAMESPACE()
