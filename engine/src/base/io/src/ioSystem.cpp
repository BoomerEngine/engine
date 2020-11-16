/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system #]
***/

#include "build.h"
#include "ioFileHandle.h"
#include "ioSystem.h"
#include "ioDirectoryWatcher.h"
#include "base/containers/include/utf8StringFunctions.h"

#ifdef PLATFORM_WINDOWS
    #include "ioSystemWindows.h"
    typedef base::io::prv::WinIOSystem NativeHandlerClass;
#elif defined(PLATFORM_POSIX)
    #include "ioSystemPOSIX.h"
    typedef base::io::prv::POSIXIOSystem NativeHandlerClass;
#else
    #error "Implement native IO system handler for this platform"
#endif

namespace base
{
    namespace io
    {
        //--

        ReadFileHandlePtr OpenForReading(StringView<char> absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().openForReading(absoluteFilePath);
        }

        WriteFileHandlePtr OpenForWriting(StringView<char> absoluteFilePath, FileWriteMode mode /*= FileWriteMode::StagedWrite*/)
        {
            return NativeHandlerClass::GetInstance().openForWriting(absoluteFilePath, mode);
        }

        AsyncFileHandlePtr OpenForAsyncReading(StringView<char> absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().openForAsyncReading(absoluteFilePath);
        }

        Buffer OpenMemoryMappedForReading(StringView<char> absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().openMemoryMappedForReading(absoluteFilePath);
        }

        Buffer LoadFileToBuffer(StringView<char> absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().loadIntoMemoryForReading(absoluteFilePath);
        }
        
        bool FileSize(StringView<char> absoluteFilePath, uint64_t& outFileSize)
        {
            return NativeHandlerClass::GetInstance().fileSize(absoluteFilePath, outFileSize);
        }

        bool FileTimeStamp(StringView<char> absoluteFilePath, class TimeStamp& outTimeStamp, uint64_t* outFileSize /*= nullptr*/)
        {
            return NativeHandlerClass::GetInstance().fileTimeStamp(absoluteFilePath, outTimeStamp, outFileSize);
        }

        bool CreatePath(StringView<char> absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().createPath(absoluteFilePath);
        }

        bool CopyFile(StringView<char> srcAbsolutePath, StringView<char> destAbsolutePath)
        {
            return NativeHandlerClass::GetInstance().copyFile(srcAbsolutePath, destAbsolutePath);
        }

        bool MoveFile(StringView<char> srcAbsolutePath, StringView<char> destAbsolutePath)
        {
            return NativeHandlerClass::GetInstance().moveFile(srcAbsolutePath, destAbsolutePath);
        }

        bool DeleteDir(StringView<char> absoluteDirPath)
        {
            return NativeHandlerClass::GetInstance().deleteDir(absoluteDirPath);
        }

        bool DeleteFile(StringView<char> absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().deleteFile(absoluteFilePath);
        }

        bool FileExists(StringView<char> absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().fileExists(absoluteFilePath);
        }

        bool TouchFile(StringView<char> absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().touchFile(absoluteFilePath);
        }

        bool IsFileReadOnly(StringView<char> absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().isFileReadOnly(absoluteFilePath);
        }

        bool ReadOnlyFlag(StringView<char> absoluteFilePath, bool flag)
        {
            return NativeHandlerClass::GetInstance().readOnlyFlag(absoluteFilePath, flag);
        }

        bool FindFiles(StringView<char> absoluteFilePath, StringView<char> searchPattern, const std::function<bool(StringView<char> fullPath, StringView<char> fileName)>& enumFunc, bool recurse)
        {
            return NativeHandlerClass::GetInstance().findFiles(absoluteFilePath, searchPattern, enumFunc, recurse);
        }

        bool FindSubDirs(StringView<char> absoluteFilePath, const std::function<bool(StringView<char> name)>& enumFunc)
        {
            return NativeHandlerClass::GetInstance().findSubDirs(absoluteFilePath, enumFunc);
        }

        bool FindLocalFiles(StringView<char> absoluteFilePath, StringView<char> searchPattern, const std::function<bool(StringView<char> name)>& enumFunc)
        {
            return NativeHandlerClass::GetInstance().findLocalFiles(absoluteFilePath, searchPattern, enumFunc);
        }

        void FindFiles(StringView<char> absoluteFilePath, StringView<char> searchPattern, Array<StringBuf>& outAbsoluteFiles, bool recurse)
        {
            NativeHandlerClass::GetInstance().findFiles(absoluteFilePath, searchPattern, [&outAbsoluteFiles](StringView<char> fullPath, base::StringView<char> name)
                {
                    outAbsoluteFiles.emplaceBack(StringBuf(fullPath));
                    return false;
                }, recurse);
        }

        void FindSubDirs(StringView<char> absoluteFilePath, Array<StringBuf>& outDirectoryNames)
        {
            NativeHandlerClass::GetInstance().findSubDirs(absoluteFilePath, [&outDirectoryNames](StringView<char> name)
                {
                    outDirectoryNames.emplaceBack(name);
                    return false;
                });
        }

        void FindLocalFiles(StringView<char> absoluteFilePath, StringView<char> searchPattern, Array<StringBuf>& outFileNames)
        {
            NativeHandlerClass::GetInstance().findSubDirs(absoluteFilePath, [&outFileNames](StringView<char> name)
                {
                    outFileNames.emplaceBack(name);
                    return false;
                });
        }

        const StringBuf& SystemPath(PathCategory category)
        {
            static StringBuf systemPaths[(int)PathCategory::MAX];

            auto& pathRef = systemPaths[(int)category];
            if (pathRef.empty())
            {
                StringBuilder txt;
                NativeHandlerClass::GetInstance().systemPath(category, txt);
                pathRef = txt.toString();
            }

            return pathRef;
        }

        DirectoryWatcherPtr CreateDirectoryWatcher(StringView<char> path)
        {
            return NativeHandlerClass::GetInstance().createDirectoryWatcher(path);
        }

        void ShowFileExplorer(StringView<char> path)
        {
            NativeHandlerClass::GetInstance().showFileExplorer(path);
        }

        bool ShowFileOpenDialog(uint64_t nativeWindowHandle, bool allowMultiple, const Array<FileFormat>& formats, base::Array<StringBuf>& outPaths, OpenSavePersistentData& persistentData)
        {
            return NativeHandlerClass::GetInstance().showFileOpenDialog(nativeWindowHandle, allowMultiple, formats, outPaths, persistentData);
        }

        bool ShowFileSaveDialog(uint64_t nativeWindowHandle, const StringBuf& currentFileName, const Array<FileFormat>& formats, StringBuf& outPath, OpenSavePersistentData& persistentData)
        {
            return NativeHandlerClass::GetInstance().showFileSaveDialog(nativeWindowHandle, currentFileName, formats, outPath, persistentData);
        }

        //--

    } // io
} // base

