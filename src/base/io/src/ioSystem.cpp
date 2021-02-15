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

        ReadFileHandlePtr OpenForReading(StringView absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().openForReading(absoluteFilePath);
        }

        WriteFileHandlePtr OpenForWriting(StringView absoluteFilePath, FileWriteMode mode /*= FileWriteMode::StagedWrite*/)
        {
            return NativeHandlerClass::GetInstance().openForWriting(absoluteFilePath, mode);
        }

        AsyncFileHandlePtr OpenForAsyncReading(StringView absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().openForAsyncReading(absoluteFilePath);
        }

        Buffer OpenMemoryMappedForReading(StringView absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().openMemoryMappedForReading(absoluteFilePath);
        }

        Buffer LoadFileToBuffer(StringView absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().loadIntoMemoryForReading(absoluteFilePath);
        }
        
        bool FileSize(StringView absoluteFilePath, uint64_t& outFileSize)
        {
            return NativeHandlerClass::GetInstance().fileSize(absoluteFilePath, outFileSize);
        }

        bool FileTimeStamp(StringView absoluteFilePath, class TimeStamp& outTimeStamp, uint64_t* outFileSize /*= nullptr*/)
        {
            return NativeHandlerClass::GetInstance().fileTimeStamp(absoluteFilePath, outTimeStamp, outFileSize);
        }

        bool CreatePath(StringView absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().createPath(absoluteFilePath);
        }

        bool CopyFile(StringView srcAbsolutePath, StringView destAbsolutePath)
        {
            return NativeHandlerClass::GetInstance().copyFile(srcAbsolutePath, destAbsolutePath);
        }

        bool MoveFile(StringView srcAbsolutePath, StringView destAbsolutePath)
        {
            return NativeHandlerClass::GetInstance().moveFile(srcAbsolutePath, destAbsolutePath);
        }

        bool DeleteDir(StringView absoluteDirPath)
        {
            return NativeHandlerClass::GetInstance().deleteDir(absoluteDirPath);
        }

        bool DeleteFile(StringView absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().deleteFile(absoluteFilePath);
        }

        bool FileExists(StringView absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().fileExists(absoluteFilePath);
        }

        bool TouchFile(StringView absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().touchFile(absoluteFilePath);
        }

        bool IsFileReadOnly(StringView absoluteFilePath)
        {
            return NativeHandlerClass::GetInstance().isFileReadOnly(absoluteFilePath);
        }

        bool ReadOnlyFlag(StringView absoluteFilePath, bool flag)
        {
            return NativeHandlerClass::GetInstance().readOnlyFlag(absoluteFilePath, flag);
        }

        bool FindFiles(StringView absoluteFilePath, StringView searchPattern, const std::function<bool(StringView fullPath, StringView fileName)>& enumFunc, bool recurse)
        {
            return NativeHandlerClass::GetInstance().findFiles(absoluteFilePath, searchPattern, enumFunc, recurse);
        }

        bool FindSubDirs(StringView absoluteFilePath, const std::function<bool(StringView name)>& enumFunc)
        {
            return NativeHandlerClass::GetInstance().findSubDirs(absoluteFilePath, enumFunc);
        }

        bool FindLocalFiles(StringView absoluteFilePath, StringView searchPattern, const std::function<bool(StringView name)>& enumFunc)
        {
            return NativeHandlerClass::GetInstance().findLocalFiles(absoluteFilePath, searchPattern, enumFunc);
        }

        void FindFiles(StringView absoluteFilePath, StringView searchPattern, Array<StringBuf>& outAbsoluteFiles, bool recurse)
        {
            NativeHandlerClass::GetInstance().findFiles(absoluteFilePath, searchPattern, [&outAbsoluteFiles](StringView fullPath, base::StringView name)
                {
                    outAbsoluteFiles.emplaceBack(StringBuf(fullPath));
                    return false;
                }, recurse);
        }

        void FindSubDirs(StringView absoluteFilePath, Array<StringBuf>& outDirectoryNames)
        {
            NativeHandlerClass::GetInstance().findSubDirs(absoluteFilePath, [&outDirectoryNames](StringView name)
                {
                    outDirectoryNames.emplaceBack(name);
                    return false;
                });
        }

        void FindLocalFiles(StringView absoluteFilePath, StringView searchPattern, Array<StringBuf>& outFileNames)
        {
            NativeHandlerClass::GetInstance().findSubDirs(absoluteFilePath, [&outFileNames](StringView name)
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

        DirectoryWatcherPtr CreateDirectoryWatcher(StringView path)
        {
            return NativeHandlerClass::GetInstance().createDirectoryWatcher(path);
        }

        void ShowFileExplorer(StringView path)
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

