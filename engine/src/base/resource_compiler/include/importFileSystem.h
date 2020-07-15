/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#pragma once

namespace base
{
    namespace res
    {

        //--

        /// wrapper for loading data from various native sources
        class BASE_RESOURCE_COMPILER_API ISourceAssetFileSystem : public IReferencable
        {
        public:
            virtual ~ISourceAssetFileSystem() = 0;

            //--

            // check if file exists
            virtual bool fileExists(StringView<char> fileSystemPath) const = 0;

            // load content of a file, returns the fingerprint of loaded data as well (since it's in the memory any way)
            virtual Buffer loadFileContent(StringView<char> fileSystemPath, ImportFileFingerprint& outFingerprint) const = 0;

            /// get child directories at given path
            virtual bool enumDirectoriesAtPath(StringView<char> fileSystemPath, const std::function<bool(StringView<char>)>& enumFunc) const = 0;

            /// get files at given path
            virtual bool enumFilesAtPath(StringView<char> fileSystemPath, const std::function<bool(StringView<char>)>& enumFunc) const = 0;

            // does this file system covers given absolute path on disk ? if so translate the path to local path
            virtual bool translateAbsolutePath(io::AbsolutePathView absolutePath, StringBuf& outFileSystemPath) const = 0;

            // get a full context path (usually absolute path) for a given import path
            virtual bool resolveContextPath(StringView<char> fileSystemPath, StringBuf& outContextPath) const = 0;

            // check status of a file
            virtual CAN_YIELD SourceAssetStatus checkFileStatus(StringView<char> fileSystemPath, uint64_t lastKnownTimestamp, const ImportFileFingerprint& lastKnownFingerprint, IProgressTracker* progress) const = 0;

            //--

            /// called from time to time for internal maintenance (save caches, process IO events, etc)
            virtual void update() = 0;
        };

        //--

        /// global "factory" for special file systems
        /// NOTE: native file systems are created manually, without the factory
        class BASE_RESOURCE_COMPILER_API ISourceAssetFileSystemFactory : public IReferencable
        {
            RTTI_DECLARE_VIRTUAL_ROOT_CLASS(ISourceAssetFileSystemFactory);

        public:
            virtual ~ISourceAssetFileSystemFactory();

            // create the source asset file system, can fail if something is of
            virtual SourceAssetFileSystemPtr createFileSystem() const = 0;
        };

        //--

    } // res
} // base