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

namespace base
{
    namespace res
    {

        //--

        SourceAssetFileSystem_LocalComputer::SourceAssetFileSystem_LocalComputer()
        {
        }

        SourceAssetFileSystem_LocalComputer::~SourceAssetFileSystem_LocalComputer()
        {}

        void SourceAssetFileSystem_LocalComputer::update()
        {

        }

        bool SourceAssetFileSystem_LocalComputer::resolveContextPath(StringView<char> fileSystemPath, StringBuf& outContextPath) const
        {
            io::AbsolutePath path;
            if (!convertToAbsolutePath(fileSystemPath, path))
                return false;

            outContextPath = StringBuf(path.c_str());
            return true;
        }

        bool SourceAssetFileSystem_LocalComputer::fileExists(StringView<char> fileSystemPath) const
        {
            io::AbsolutePath path;
            if (!convertToAbsolutePath(fileSystemPath, path))
                return false;

            return base::io::FileExists(path);
        }

        SourceAssetStatus SourceAssetFileSystem_LocalComputer::checkFileStatus(StringView<char> fileSystemPath, const io::TimeStamp& lastKnownTimestamp, const ImportFileFingerprint& lastKnownFingerprint, IProgressTracker* progress) const
        {
            io::AbsolutePath path;
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

        Buffer SourceAssetFileSystem_LocalComputer::loadFileContent(StringView<char> fileSystemPath, io::TimeStamp& outTimestamp, ImportFileFingerprint& outFingerprint) const
        {
            io::AbsolutePath path;
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

        bool SourceAssetFileSystem_LocalComputer::enumDirectoriesAtPath(StringView<char> fileSystemPath, const std::function<bool(StringView<char>)>& enumFunc) const
        {
            io::AbsolutePath path;
            if (!convertToAbsolutePath(fileSystemPath, path))
                return false;

            return base::io::FindSubDirs(path, [enumFunc](io::AbsolutePathView view)
                {
                    if (view.beginsWith(L"."))
                        return false;

                    StringBuf utf8Name(view);
                    return enumFunc(utf8Name);
                });
        }

        bool SourceAssetFileSystem_LocalComputer::enumFilesAtPath(StringView<char> fileSystemPath, const std::function<bool(StringView<char>)>& enumFunc) const
        {
            io::AbsolutePath path;
            if (!convertToAbsolutePath(fileSystemPath, path))
                return false;

            return base::io::FindLocalFiles(path, L"*.*", [enumFunc](io::AbsolutePathView view)
                {
                    if (view.beginsWith(L"."))
                        return false;

                    StringBuf utf8Name(view);
                    return enumFunc(utf8Name);
                });
        }

        bool SourceAssetFileSystem_LocalComputer::translateAbsolutePath(io::AbsolutePathView absolutePath, StringBuf& outFileSystemPath) const
        {
            // non-file path
            if (absolutePath.empty() || absolutePath.endsWith(L"/") || absolutePath.endsWith(L"\\"))
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

        bool SourceAssetFileSystem_LocalComputer::convertToAbsolutePath(StringView<char> fileSystemPath, io::AbsolutePath& outAbsolutePath) const
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
            UTF16StringBuf pathString;
            pathString.reserve(fileSystemPath.length() + 4);

            // drive path
            wchar_t drivePath[3] = { (wchar_t)letter, ':', 0 };
            pathString += drivePath;

            // rest of the path (automatic conversion from UTF-8 to UTF-16 will occur)
            pathString += parser.currentView();

            // replace path separators
            for (auto& ch : pathString)
                if (ch == io::AbsolutePath::WRONG_SYSTEM_PATH_SEPARATOR)
                    ch = io::AbsolutePath::SYSTEM_PATH_SEPARATOR;

            // TODO: any more validation ?

            outAbsolutePath = io::AbsolutePath::Build(pathString);
            return true;
        }

        //--

    } // res
} // base
