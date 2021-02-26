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

/// "LOCAL COMPUTER" file system - basically shows all dirs/drives
class CORE_RESOURCE_COMPILER_API SourceAssetFileSystem_LocalComputer : public ISourceAssetFileSystem
{
public:
    SourceAssetFileSystem_LocalComputer();
    virtual ~SourceAssetFileSystem_LocalComputer();

    // ISourceAssetFileSystem
    virtual void update() override;
    virtual bool fileExists(StringView fileSystemPath) const override;
    virtual Buffer loadFileContent(StringView fileSystemPath, io::TimeStamp& outTimestamp, ImportFileFingerprint& outFingerprint) const override;
    virtual bool enumDirectoriesAtPath(StringView fileSystemPath, const std::function<bool(StringView)>& enumFunc) const override;
    virtual bool enumFilesAtPath(StringView fileSystemPath, const std::function<bool(StringView)>& enumFunc) const override;
    virtual bool translateAbsolutePath(StringView absolutePath, StringBuf& outFileSystemPath) const override;
    virtual bool resolveContextPath(StringView fileSystemPath, StringBuf& outContextPath) const override;
    virtual CAN_YIELD SourceAssetStatus checkFileStatus(StringView fileSystemPath, const io::TimeStamp& lastKnownTimestamp, const ImportFileFingerprint& lastKnownFingerprint, IProgressTracker* progress) const override;

private:
    bool convertToAbsolutePath(StringView fileSystemPath, StringBuf& outAbsolutePath) const;
};

//--

END_BOOMER_NAMESPACE_EX(res)
