/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#include "build.h"
#include "importFileSystem.h"
#include "importFileService.h"
#include "core/resource/include/resourceMetadata.h"

#include "core/io/include/io.h"
#include "importFileSystemNative.h"

BEGIN_BOOMER_NAMESPACE_EX(res)

//--

ConfigProperty<bool> cvAllowLocalPCImports("SourceAssets", "AllowLocalPCImports", true);
ConfigProperty<double> cvSystemTickTime("SourceAssets", "FileSystemTickInterval", 1.0);

//--

RTTI_BEGIN_TYPE_CLASS(ImportFileService);
RTTI_END_TYPE();

//--

ImportFileService::ImportFileService()
{}

ImportFileService::~ImportFileService()
{}

bool ImportFileService::fileExists(StringView assetImportPath) const
{
    StringView fileSystemPath;
    if (const auto* fs = resolveFileSystem(assetImportPath, fileSystemPath))
        return fs->fileExists(fileSystemPath);

    return false;
}

bool ImportFileService::translateAbsolutePath(StringView absolutePath, StringBuf& outFileSystemPath) const
{
    StringBuf bestShortPath;
    StringBuf bestPrefix;

    for (const auto& fs : m_fileSystems)
    {
        StringBuf fileSytemPath;
        if (fs.fileSystem->translateAbsolutePath(absolutePath, fileSytemPath))
        {
            if (bestShortPath.empty() || fileSytemPath.length() < bestShortPath.length())
            {
                bestShortPath = fileSytemPath;
                bestPrefix = fs.prefix;
            }
        }
    }

    if (bestShortPath.empty())
        return false;

    outFileSystemPath = TempString("{}{}", bestPrefix, bestShortPath);
    return true;
}

bool ImportFileService::resolveContextPath(StringView assetImportPath, StringBuf& outContextPath) const
{
    StringView fileSystemPath;
    if (const auto* fs = resolveFileSystem(assetImportPath, fileSystemPath))
        return fs->resolveContextPath(fileSystemPath, outContextPath);

    return Buffer();
}

Buffer ImportFileService::loadFileContent(StringView assetImportPath, TimeStamp& outTimestamp, ImportFileFingerprint& outFingerprint) const
{
    StringView fileSystemPath;
    if (const auto* fs = resolveFileSystem(assetImportPath, fileSystemPath))
        return fs->loadFileContent(fileSystemPath, outTimestamp, outFingerprint);

    return Buffer();
}

bool ImportFileService::enumDirectoriesAtPath(StringView assetImportPath, const std::function<bool(StringView)>& enumFunc) const
{
    StringView fileSystemPath;
    if (const auto* fs = resolveFileSystem(assetImportPath, fileSystemPath))
        return fs->enumDirectoriesAtPath(fileSystemPath, enumFunc);

    return false;
}

bool ImportFileService::enumFilesAtPath(StringView assetImportPath, const std::function<bool(StringView)>& enumFunc) const
{
    StringView fileSystemPath;
    if (const auto* fs = resolveFileSystem(assetImportPath, fileSystemPath))
        return fs->enumFilesAtPath(fileSystemPath, enumFunc);

    return false;
}

bool ImportFileService::enumRoots(const std::function<bool(StringView)>& enumFunc) const
{
    for (const auto& fs : m_fileSystems)
        if (enumFunc(fs.prefix))
            return true;
    return false;
}

//--

SourceAssetStatus ImportFileService::checkFileStatus(StringView assetImportPath, const TimeStamp& lastKnownTimestamp, const ImportFileFingerprint& lastKnownCRC, IProgressTracker* progress) const
{
    StringView fileSystemPath;
    if (const auto* fs = resolveFileSystem(assetImportPath, fileSystemPath))
        return fs->checkFileStatus(fileSystemPath, lastKnownTimestamp, lastKnownCRC, progress);

    return SourceAssetStatus::Missing;
}

//--

ResourceConfigurationPtr ImportFileService::compileBaseResourceConfiguration(StringView assetImportPath, SpecificClassType<ResourceConfiguration> configClass) const
{
    if (!configClass || configClass->isAbstract())
        return nullptr;
                
    // TODO: look for files on disk with custom configuration 

    return configClass.create();
}

//--

app::ServiceInitializationResult ImportFileService::onInitializeService(const app::CommandLine& cmdLine)
{
    createFileSystems();

    m_nextSystemUpdate = NativeTimePoint::Now() + cvSystemTickTime.get();

    return app::ServiceInitializationResult::Finished;
}

void ImportFileService::onShutdownService()
{
    destroyFileSystems();
}

void ImportFileService::onSyncUpdate()
{
    // TODO: we can do background CRC calculation here, background checks for changed assets etc

    // update the file systems from time to time
    if (m_nextSystemUpdate.reached())
    {
        m_nextSystemUpdate = NativeTimePoint::Now() + cvSystemTickTime.get();

        for (const auto& fs : m_fileSystems)
            fs.fileSystem->update();
    }
}

//--

void ImportFileService::createFileSystems()
{
    // create the "LOCAL" source
    if (cvAllowLocalPCImports.get())
    {
        auto& entry = m_fileSystems.emplaceBack();
        entry.fileSystem = RefNew<SourceAssetFileSystem_LocalComputer>();
        entry.prefix = StringBuf("LOCAL:");
    }
}

void ImportFileService::destroyFileSystems()
{
    m_fileSystems.clear();
}

//--

const ISourceAssetFileSystem* ImportFileService::resolveFileSystem(StringView assetImportPath, StringView& outFileSystemPath) const
{
    for (const auto& fs : m_fileSystems)
    {
        if (assetImportPath.beginsWith(fs.prefix))
        {
            outFileSystemPath = assetImportPath.subString(fs.prefix.length());
            return fs.fileSystem;
        }
    }
            
    return nullptr;
}

//--

END_BOOMER_NAMESPACE_EX(res)
