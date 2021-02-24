/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#include "build.h"
#include "importFileSystem.h"
#include "importFileSystemNative.h"
#include "base/io/include/ioSystem.h"
#include "base/io/include/timestamp.h"
#include "importFileFingerprintService.h"
#include "importFileFingerprint.h"

BEGIN_BOOMER_NAMESPACE(base::res)

//--

SourceAssetFileSystem_LocalComputer::SourceAssetFileSystem_LocalComputer()
{
}

SourceAssetFileSystem_LocalComputer::~SourceAssetFileSystem_LocalComputer()
{}

void SourceAssetFileSystem_LocalComputer::update()
{

}

bool SourceAssetFileSystem_LocalComputer::resolveContextPath(StringView fileSystemPath, StringBuf& outContextPath) const
{
    StringBuf path;
    if (!convertToAbsolutePath(fileSystemPath, path))
        return false;

    outContextPath = StringBuf(path.c_str());
    return true;
}

bool SourceAssetFileSystem_LocalComputer::fileExists(StringView fileSystemPath) const
{
    StringBuf path;
    if (!convertToAbsolutePath(fileSystemPath, path))
        return false;

    return base::io::FileExists(path);
}

SourceAssetStatus SourceAssetFileSystem_LocalComputer::checkFileStatus(StringView fileSystemPath, const io::TimeStamp& lastKnownTimestamp, const ImportFileFingerprint& lastKnownFingerprint, IProgressTracker* progress) const
{
    StringBuf path;
    if (!convertToAbsolutePath(fileSystemPath, path))
        return SourceAssetStatus::Missing;

    io::TimeStamp timestamp;
    if (!base::io::FileTimeStamp(path, timestamp))
        return SourceAssetStatus::Missing;

    // if timestamp is given then check and use it to save CRC check
    if (!lastKnownTimestamp.empty())
        if (lastKnownTimestamp == timestamp)
            return SourceAssetStatus::UpToDate;

    // compute the fingerprint using the fingerprint service
    ImportFileFingerprint currentFingerprint;
    const auto ret = base::GetService<ImportFileFingerprintService>()->calculateFingerprint(path, true, progress, currentFingerprint);
    if (ret == FingerpintCalculationStatus::ErrorOutOfMemory || ret == FingerpintCalculationStatus::ErrorInvalidRead)
        return SourceAssetStatus::ReadFailure;
    else if (ret == FingerpintCalculationStatus::ErrorNoFile)
        return SourceAssetStatus::Missing;
    else if (ret == FingerpintCalculationStatus::Canceled)
        return SourceAssetStatus::Canceled;
    else if (ret != FingerpintCalculationStatus::OK)
        return SourceAssetStatus::ReadFailure;

    // valid 
    if (lastKnownFingerprint == currentFingerprint)
        return SourceAssetStatus::UpToDate;
    else
        return SourceAssetStatus::ContentChanged;
}

Buffer SourceAssetFileSystem_LocalComputer::loadFileContent(StringView fileSystemPath, io::TimeStamp& outTimestamp, ImportFileFingerprint& outFingerprint) const
{
    StringBuf path;
    if (!convertToAbsolutePath(fileSystemPath, path))
        return Buffer();

    io::TimeStamp timestamp;
    if (!base::io::FileTimeStamp(path, timestamp))
        return Buffer();

    const auto ret = base::io::LoadFileToBuffer(path);
    if (ret)
    {
        CalculateMemoryFingerprint(ret.data(), ret.size(), nullptr, outFingerprint);
        outTimestamp = timestamp;
    }

    return ret;
}

bool SourceAssetFileSystem_LocalComputer::enumDirectoriesAtPath(StringView fileSystemPath, const std::function<bool(StringView)>& enumFunc) const
{
    StringBuf path;
    if (!convertToAbsolutePath(fileSystemPath, path))
        return false;

    return base::io::FindSubDirs(path, [enumFunc](StringView view)
        {
            if (!view.beginsWith("."))
            {
                StringBuf utf8Name(view);
                return enumFunc(utf8Name);
            }

            return false;
        });
}

bool SourceAssetFileSystem_LocalComputer::enumFilesAtPath(StringView fileSystemPath, const std::function<bool(StringView)>& enumFunc) const
{
    StringBuf path;
    if (!convertToAbsolutePath(fileSystemPath, path))
        return false;

    return base::io::FindLocalFiles(path, "*.*", [enumFunc](StringView view)
        {
            if (!view.beginsWith("."))
            {
                StringBuf utf8Name(view);
                return enumFunc(utf8Name);
            }

            return false;
        });
}

bool SourceAssetFileSystem_LocalComputer::translateAbsolutePath(StringView absolutePath, StringBuf& outFileSystemPath) const
{
    // non-file path
    if (absolutePath.empty() || absolutePath.endsWith("/") || absolutePath.endsWith("\\"))
        return false;

    // TODO: LINUX/MacOS

    // copy path to local buffer
    StringBuf localPath(absolutePath);

    // change all path separators to the sane ones
    localPath.replaceChar('\\', '/');

    uint32_t letter = 0;
    StringParser parser(localPath);
    if (!parser.parseChar(letter))
        return false;

    // make lower case
    if (letter >= 'A' && letter <= 'Z')
        letter = (letter - 'A') + 'a';

    // we are only interested in drive letters, no network shares
    if (letter < 'a' || letter > 'z')
        return false;

    // follow path spec
    if (!parser.parseKeyword(":/"))
        return false;

    StringBuilder retPath;
    retPath.append("/");
    retPath.appendch((char)letter);
    retPath.append("/");
    retPath.append(parser.currentView());

    // done
    outFileSystemPath = retPath.toString();
    return true;
}

//--

bool SourceAssetFileSystem_LocalComputer::convertToAbsolutePath(StringView fileSystemPath, StringBuf& outAbsolutePath) const
{
    StringParser parser(fileSystemPath);

    // parse the drive letter
    if (!parser.parseKeyword("/"))
        return false;

    // TODO: LINUX/MacOS

    // parse the drive letter
    uint32_t letter = 0;
    if (!parser.parseChar(letter))
        return false;

    // make lower case
    if (letter >= 'A' && letter <= 'Z')
        letter = (letter - 'A') + 'a';

    // we are only interested in drive letters
    if (letter < 'a' || letter > 'z')
        return false;

    // assemble the UTF-16 path
    StringBuilder pathString;

    // drive path
    char drivePath[3] = { letter, ':', 0 };
    pathString.append(drivePath);

    // rest of the path (automatic conversion from UTF-8 to UTF-16 will occur)
    pathString.append(parser.currentView());

    // fixup
    {
        auto* ch = pathString.c_str();
        while (*ch)
        {
            if (*ch == io::WRONG_SYSTEM_PATH_SEPARATOR)
                *ch = io::SYSTEM_PATH_SEPARATOR;
            ++ch;
        }
    }

    // export
    outAbsolutePath = pathString.toString();
    return true;
}

//--

END_BOOMER_NAMESPACE(base::res)
