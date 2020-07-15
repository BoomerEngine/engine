/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system #]
***/

#include "build.h"
#include "utils.h"
#include "ioFileHandle.h"
#include "ioMemoryPool.h"
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

        System::System()
        {
            // create the native bind point
            m_handler = MemNew(NativeHandlerClass);

            // create the memory pool
            m_memoryPool.create();

            // started
            TRACE_SPAM("File system started");
        }

        ReadFileHandlePtr System::openForReading(AbsolutePathView absoluteFilePath)
        {
            return m_handler->openForReading(absoluteFilePath);
        }

        WriteFileHandlePtr System::openForWriting(AbsolutePathView absoluteFilePath, FileWriteMode mode /*= FileWriteMode::StagedWrite*/)
        {
            return m_handler->openForWriting(absoluteFilePath, mode);
        }

        AsyncFileHandlePtr System::openForAsyncReading(AbsolutePathView absoluteFilePath)
        {
            return m_handler->openForAsyncReading(absoluteFilePath);
        }

        Buffer System::loadIntoMemoryForReading(AbsolutePathView absoluteFilePath)
        {
            return m_handler->loadIntoMemoryForReading(absoluteFilePath);
        }
        
        Buffer System::openMemoryMappedForReading(AbsolutePathView absoluteFilePath)
        {
            return m_handler->openMemoryMappedForReading(absoluteFilePath);
        }

        bool System::fileSize(AbsolutePathView absoluteFilePath, uint64_t& outFileSize)
        {
            return m_handler->fileSize(absoluteFilePath, outFileSize);
        }

        bool System::fileTimeStamp(AbsolutePathView absoluteFilePath, class TimeStamp& outTimeStamp, uint64_t* outFileSize /*= nullptr*/)
        {
            return m_handler->fileTimeStamp(absoluteFilePath, outTimeStamp, outFileSize);
        }

        bool System::createPath(AbsolutePathView absoluteFilePath)
        {
            return m_handler->createPath(absoluteFilePath);
        }

        bool System::copyFile(AbsolutePathView srcAbsolutePath, AbsolutePathView destAbsolutePath)
        {
            return m_handler->copyFile(srcAbsolutePath, destAbsolutePath);
        }

        bool System::moveFile(AbsolutePathView srcAbsolutePath, AbsolutePathView destAbsolutePath)
        {
            return m_handler->moveFile(srcAbsolutePath, destAbsolutePath);
        }

        bool System::deleteDir(AbsolutePathView absoluteDirPath)
        {
            return m_handler->deleteDir(absoluteDirPath);
        }

        bool System::deleteFile(AbsolutePathView absoluteFilePath)
        {
            return m_handler->deleteFile(absoluteFilePath);
        }

        bool System::fileExists(AbsolutePathView absoluteFilePath)
        {
            return m_handler->fileExists(absoluteFilePath);
        }

        bool System::touchFile(AbsolutePathView absoluteFilePath)
        {
            return m_handler->touchFile(absoluteFilePath);
        }

        bool System::isFileReadOnly(AbsolutePathView absoluteFilePath)
        {
            return m_handler->isFileReadOnly(absoluteFilePath);
        }

        bool System::readOnlyFlag(AbsolutePathView absoluteFilePath, bool flag)
        {
            return m_handler->readOnlyFlag(absoluteFilePath, flag);
        }

        bool System::findFiles(AbsolutePathView absoluteFilePath, StringView<wchar_t> searchPattern, const std::function<bool(AbsolutePathView fullPath, StringView<wchar_t> fileName)>& enumFunc, bool recurse)
        {
            return m_handler->findFiles(absoluteFilePath, searchPattern, enumFunc, recurse);
        }

        bool System::findSubDirs(AbsolutePathView absoluteFilePath, const std::function<bool(StringView<wchar_t> name)>& enumFunc)
        {
            return m_handler->findSubDirs(absoluteFilePath, enumFunc);
        }

        bool System::findLocalFiles(AbsolutePathView absoluteFilePath, StringView<wchar_t> searchPattern, const std::function<bool(StringView<wchar_t> name)>& enumFunc)
        {
            return m_handler->findLocalFiles(absoluteFilePath, searchPattern, enumFunc);
        }

        void System::findFiles(AbsolutePathView absoluteFilePath, StringView<wchar_t> searchPattern, Array<AbsolutePath>& outAbsoluteFiles, bool recurse)
        {
            m_handler->findFiles(absoluteFilePath, searchPattern, [&outAbsoluteFiles](AbsolutePathView fullPath, base::StringView<wchar_t> name)
                {
                    outAbsoluteFiles.emplaceBack(AbsolutePath::Build(fullPath));
                    return false;
                }, recurse);
        }

        void System::findSubDirs(AbsolutePathView absoluteFilePath, Array< UTF16StringBuf >& outDirectoryNames)
        {
            m_handler->findSubDirs(absoluteFilePath, [&outDirectoryNames](StringView<wchar_t> name)
                {
                    outDirectoryNames.emplaceBack(name);
                    return false;
                });
        }

        void System::findLocalFiles(AbsolutePathView absoluteFilePath, StringView<wchar_t> searchPattern, Array< UTF16StringBuf >& outFileNames)
        {
            m_handler->findSubDirs(absoluteFilePath, [&outFileNames](StringView<wchar_t> name)
                {
                    outFileNames.emplaceBack(name);
                    return false;
                });
        }

        const AbsolutePath& System::systemPath(PathCategory category)
        {
            auto& entry = m_paths[(uint8_t)category];
            if (entry.empty())
                entry = m_handler->systemPath(category);

            return entry;
        }

        DirectoryWatcherPtr System::createDirectoryWatcher(AbsolutePathView path)
        {
            return m_handler->createDirectoryWatcher(path);
        }

        void System::showFileExplorer(AbsolutePathView path)
        {
            m_handler->showFileExplorer(path);
        }

        bool System::showFileOpenDialog(uint64_t nativeWindowHandle, bool allowMultiple, const Array<FileFormat>& formats, base::Array<AbsolutePath>& outPaths, OpenSavePersistentData& persistentData)
        {
            return m_handler->showFileOpenDialog(nativeWindowHandle, allowMultiple, formats, outPaths, persistentData);
        }

        bool System::showFileSaveDialog(uint64_t nativeWindowHandle, const UTF16StringBuf& currentFileName, const Array<FileFormat>& formats, AbsolutePath& outPath, OpenSavePersistentData& persistentData)
        {
            return m_handler->showFileSaveDialog(nativeWindowHandle, currentFileName, formats, outPath, persistentData);
        }

        void System::deinit()
        {
            MemDelete(m_handler);
            m_memoryPool.reset();

            for (uint32_t i=0; i<ARRAY_COUNT(m_paths); ++i)
                m_paths[i] = AbsolutePath();
        }

    } // io
} // base

