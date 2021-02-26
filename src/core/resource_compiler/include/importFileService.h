/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(res)

//--

/// service that hosts all source asset file systems that can be used to import resources
/// NOTE: this is global/shared service
class CORE_RESOURCE_COMPILER_API ImportFileService : public app::ILocalService
{
    RTTI_DECLARE_VIRTUAL_CLASS(ImportFileService, app::ILocalService);

public:
    ImportFileService();
    virtual ~ImportFileService();

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

    // load content of a file, returns the CRC of the data as well
    Buffer loadFileContent(StringView assetImportPath, io::TimeStamp& outTimestamp, ImportFileFingerprint& outCRC) const;

    /// get child directories at given path
    bool enumDirectoriesAtPath(StringView assetImportPath, const std::function<bool(StringView)>& enumFunc) const;

    /// get files at given path
    bool enumFilesAtPath(StringView assetImportPath, const std::function<bool(StringView)>& enumFunc) const;

    /// enumerate file system "roots" (ie. LOCAL, HL2, etc)
    bool enumRoots(const std::function<bool(StringView)>& enumFunc) const;

    //--

    /// validate source file 
    /// NOTE: this may take long time, run on fiber
    CAN_YIELD SourceAssetStatus checkFileStatus(StringView assetImportPath, const io::TimeStamp& lastKnownTimestamp, const ImportFileFingerprint& lastKnownCRC, IProgressTracker* progress = nullptr) const;

    //--

    /// compile a base resource import configuration for asset of given type imported from given source folder
    ResourceConfigurationPtr compileBaseResourceConfiguration(StringView assetImportPath, SpecificClassType<ResourceConfiguration> configClass) const;

    //--

protected:
    virtual app::ServiceInitializationResult onInitializeService(const app::CommandLine& cmdLine) override final;
    virtual void onShutdownService() override final;
    virtual void onSyncUpdate() override final;

    struct FileSystemInfo
    {
        StringBuf prefix; // LOCAL:
        SourceAssetFileSystemPtr fileSystem;
    };

    Array<FileSystemInfo> m_fileSystems;

    void createFileSystems();
    void destroyFileSystems();

    const ISourceAssetFileSystem* resolveFileSystem(StringView assetImportPath, StringView& outFileSystemPath) const;

    NativeTimePoint m_nextSystemUpdate;
};

//--

END_BOOMER_NAMESPACE_EX(res)
