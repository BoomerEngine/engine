/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: depot\filesystem #]
***/

#pragma once

#include "base/io/include/timestamp.h"
#include "base/object/include/object.h"

namespace base
{
    namespace depot
    {
        //--

        /// Abstraction over the raw file system used by the raw loader to cook resources
        class BASE_RESOURCE_COMPILER_API IFileSystem : public base::NoCopy
        {
        public:
            virtual ~IFileSystem();

            /// is this a physical file system 
            virtual bool isPhysical() const = 0;

            /// is this file system writable ?
            virtual bool isWritable() const = 0;

            /// can this file system handle given raw file, if not will usually try the one above, this allows for fallback placement of manifest files outside read only sub-file systems
            virtual bool ownsFile(StringView<char> rawFilePath) const = 0;

            /// get a printable context name for the given file
            virtual bool contextName(StringView<char> rawFilePath, StringBuf& outContextName) const = 0;

            /// get the file timestamp
            virtual bool timestamp(StringView<char> rawFilePath, io::TimeStamp& outTimestamp) const = 0;

            /// get the absolute path to the file content, can be used to write new content
            /// NOTE: is present only for files physically on disk, not in archives or over the network
            virtual bool absolutePath(StringView<char> rawFilePath, StringBuf& outAbsolutePath) const = 0;

            /// get child directories at given path
            virtual bool enumDirectoriesAtPath(StringView<char> rawDirectoryPath, const std::function<bool(StringView<char>)>& enumFunc) const = 0;

            /// get files at given path
            virtual bool enumFilesAtPath(StringView<char> rawDirectoryPath, const std::function<bool(StringView<char>)>& enumFunc) const = 0;

            /// create a reader for the file's content
            /// NOTE: creating a reader may take some time
            virtual io::ReadFileHandlePtr createReader(StringView<char> rawFilePath) const = 0;

            /// create an ASYNC reader for the file's content
            /// NOTE: creating a reader may take some time
            virtual io::AsyncFileHandlePtr createAsyncReader(StringView<char> rawFilePath) const = 0;

            /// create a writer for new content of file
            /// NOTE: can be denied if the file system or file is not writable or for any other reason
            /// NOTE: this will block any read request to the same file
            /// NOTE: this will update the timestamp and CRC of the file
            virtual io::WriteFileHandlePtr createWriter(StringView<char> rawFilePath) const = 0;

            //--

            /// enable live tracking of operations on the file system
            virtual void enableFileSystemObservers() = 0;

            //--
        };

        ///--

        // a package's dependency, responsible for creating a file system
        class BASE_RESOURCE_COMPILER_API IFileSystemProvider : public IObject
        {
            RTTI_DECLARE_VIRTUAL_CLASS(IFileSystemProvider, IObject);

        public:
            virtual ~IFileSystemProvider();

            // create the file system representing the dependency
            // NOTE: this is allowed to take lots of time but should be cached if possible
            virtual UniquePtr<IFileSystem> createFileSystem(DepotStructure* owner) const = 0;
        };

        ///--

        // a simple file system notifier that gets informed about changes in the files
        class BASE_RESOURCE_COMPILER_API IFileSystemNotifier : public base::NoCopy
        {
        public:
            virtual ~IFileSystemNotifier();

        };

        //--

    } // depot
} // base