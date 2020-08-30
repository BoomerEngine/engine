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

        ReadFileHandlePtr OpenForReading(AbsolutePathView absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().openForReading(absoluteFilePath);
        }

        WriteFileHandlePtr OpenForWriting(AbsolutePathView absoluteFilePath, FileWriteMode mode /*= FileWriteMode::StagedWrite*/)
        {
            return NativeHandlerClass::GetInstance().openForWriting(absoluteFilePath, mode);
        }

        AsyncFileHandlePtr OpenForAsyncReading(AbsolutePathView absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().openForAsyncReading(absoluteFilePath);
        }

        Buffer OpenMemoryMappedForReading(AbsolutePathView absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().openMemoryMappedForReading(absoluteFilePath);
        }

        Buffer LoadFileToBuffer(AbsolutePathView absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().loadIntoMemoryForReading(absoluteFilePath);
        }
        
        bool FileSize(AbsolutePathView absoluteFilePath, uint64_t& outFileSize)
        {
            return NativeHandlerClass::GetInstance().fileSize(absoluteFilePath, outFileSize);
        }

        bool FileTimeStamp(AbsolutePathView absoluteFilePath, class TimeStamp& outTimeStamp, uint64_t* outFileSize /*= nullptr*/)
        {
            return NativeHandlerClass::GetInstance().fileTimeStamp(absoluteFilePath, outTimeStamp, outFileSize);
        }

        bool CreatePath(AbsolutePathView absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().createPath(absoluteFilePath);
        }

        bool CopyFile(AbsolutePathView srcAbsolutePath, AbsolutePathView destAbsolutePath)
        {
            return NativeHandlerClass::GetInstance().copyFile(srcAbsolutePath, destAbsolutePath);
        }

        bool MoveFile(AbsolutePathView srcAbsolutePath, AbsolutePathView destAbsolutePath)
        {
            return NativeHandlerClass::GetInstance().moveFile(srcAbsolutePath, destAbsolutePath);
        }

        bool DeleteDir(AbsolutePathView absoluteDirPath)
        {
            return NativeHandlerClass::GetInstance().deleteDir(absoluteDirPath);
        }

        bool DeleteFile(AbsolutePathView absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().deleteFile(absoluteFilePath);
        }

        bool FileExists(AbsolutePathView absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().fileExists(absoluteFilePath);
        }

        bool TouchFile(AbsolutePathView absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().touchFile(absoluteFilePath);
        }

        bool IsFileReadOnly(AbsolutePathView absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().isFileReadOnly(absoluteFilePath);
        }

        bool ReadOnlyFlag(AbsolutePathView absoluteFilePath, bool flag)
        {
            return NativeHandlerClass::GetInstance().readOnlyFlag(absoluteFilePath, flag);
        }

        bool FindFiles(AbsolutePathView absoluteFilePath, StringView<wchar_t> searchPattern, const std::function<bool(AbsolutePathView fullPath, StringView<wchar_t> fileName)>& enumFunc, bool recurse)
        {
            return NativeHandlerClass::GetInstance().findFiles(absoluteFilePath, searchPattern, enumFunc, recurse);
        }

        bool FindSubDirs(AbsolutePathView absoluteFilePath, const std::function<bool(StringView<wchar_t> name)>& enumFunc)
        {
            return NativeHandlerClass::GetInstance().findSubDirs(absoluteFilePath, enumFunc);
        }

        bool FindLocalFiles(AbsolutePathView absoluteFilePath, StringView<wchar_t> searchPattern, const std::function<bool(StringView<wchar_t> name)>& enumFunc)
        {
            return NativeHandlerClass::GetInstance().findLocalFiles(absoluteFilePath, searchPattern, enumFunc);
        }

        void FindFiles(AbsolutePathView absoluteFilePath, StringView<wchar_t> searchPattern, Array<AbsolutePath>& outAbsoluteFiles, bool recurse)
        {
            NativeHandlerClass::GetInstance().findFiles(absoluteFilePath, searchPattern, [&outAbsoluteFiles](AbsolutePathView fullPath, base::StringView<wchar_t> name)
                {
                    outAbsoluteFiles.emplaceBack(AbsolutePath::Build(fullPath));
                    return false;
                }, recurse);
        }

        void FindSubDirs(AbsolutePathView absoluteFilePath, Array< UTF16StringBuf >& outDirectoryNames)
        {
            NativeHandlerClass::GetInstance().findSubDirs(absoluteFilePath, [&outDirectoryNames](StringView<wchar_t> name)
                {
                    outDirectoryNames.emplaceBack(name);
                    return false;
                });
        }

        void FindLocalFiles(AbsolutePathView absoluteFilePath, StringView<wchar_t> searchPattern, Array< UTF16StringBuf >& outFileNames)
        {
            NativeHandlerClass::GetInstance().findSubDirs(absoluteFilePath, [&outFileNames](StringView<wchar_t> name)
                {
                    outFileNames.emplaceBack(name);
                    return false;
                });
        }

        const AbsolutePath& SystemPath(PathCategory category)
        {
            static AbsolutePath systemPaths[(int)PathCategory::MAX];

            auto& pathRef = systemPaths[(int)category];
            if (pathRef.empty())
                pathRef = NativeHandlerClass::GetInstance().systemPath(category);

            return pathRef;
        }

        DirectoryWatcherPtr CreateDirectoryWatcher(AbsolutePathView path)
        {
            return NativeHandlerClass::GetInstance().createDirectoryWatcher(path);
        }

        void ShowFileExplorer(AbsolutePathView path)
        {
            NativeHandlerClass::GetInstance().showFileExplorer(path);
        }

        bool ShowFileOpenDialog(uint64_t nativeWindowHandle, bool allowMultiple, const Array<FileFormat>& formats, base::Array<AbsolutePath>& outPaths, OpenSavePersistentData& persistentData)
        {
            return NativeHandlerClass::GetInstance().showFileOpenDialog(nativeWindowHandle, allowMultiple, formats, outPaths, persistentData);
        }

        bool ShowFileSaveDialog(uint64_t nativeWindowHandle, const UTF16StringBuf& currentFileName, const Array<FileFormat>& formats, AbsolutePath& outPath, OpenSavePersistentData& persistentData)
        {
            return NativeHandlerClass::GetInstance().showFileSaveDialog(nativeWindowHandle, currentFileName, formats, outPath, persistentData);
        }

        //--

    } // io
} // base

