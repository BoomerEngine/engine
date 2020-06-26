/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

#include "base/containers/include/stringBuf.h"
#include "base/containers/include/inplaceArray.h"

namespace base
{
    namespace io
    {
        class AbsolutePath;

        /// helper class that can slice the absolute path into pieces
        /// NOTE: slow as fuck, use in backend only
        class BASE_IO_API AbsolutePathBuilder
        {
        public:
            AbsolutePathBuilder();
            AbsolutePathBuilder(const AbsolutePath& path);
            AbsolutePathBuilder(const UTF16StringBuf& relaxedPath); // may contain ../../ -> they are collapsed
            AbsolutePathBuilder(StringView<wchar_t> relaxedPath); // may contain ../../ -> they are collapsed

            /// get the drive name (C:\, D:\ or / under linux )
            INLINE const UTF16StringBuf& drive() const { return m_drive; }

            /// get number of directories
            INLINE uint32_t numDirectories() const { return m_directories.size(); }

            /// get directory name
            INLINE const UTF16StringBuf& directory(uint32_t index) const { return m_directories[index]; }

            /// get number of extensions
            INLINE uint32_t numExtensions() const { return m_extensions.size(); }

            /// get file extensions 
            INLINE const UTF16StringBuf& extension(uint32_t index) const { return m_extensions[index]; }

            // get main extensions (last one)
            INLINE UTF16StringBuf mainExtension() const { return m_extensions.empty() ? UTF16StringBuf() : m_extensions.back(); }

            // get file name
            INLINE const UTF16StringBuf& fileName() const { return m_fileName; }

            //--

            // clear path
            AbsolutePathBuilder& clear();

            // set new content
            AbsolutePathBuilder& reset(const AbsolutePath& path);

            // set new content
            AbsolutePathBuilder& reset(StringView<wchar_t> relaxedPath);


            //--

            // remove the file name and extension, good for converting file paths to directory paths
            AbsolutePathBuilder& removeFileNameAndExtension();

            // set new file name
            AbsolutePathBuilder& fileName(StringView<wchar_t> fileName);

            // append part to file name
            AbsolutePathBuilder& appendFileName(StringView<wchar_t> partialName);

            //--

            // remove all file extensions
            AbsolutePathBuilder& removeExtensions();

            // remove file extension, by index
            AbsolutePathBuilder& removeExtension(uint32_t index = 0);

            // set file main extension (the last one)
            // NOTE: empty extension is not valid and will assert (non fatal)
            AbsolutePathBuilder& extension(StringView<wchar_t> extension);

            // add file extensions (at the end)
            AbsolutePathBuilder& addExtension(StringView<wchar_t> extension);

            //--
            
            // remove all directories
            AbsolutePathBuilder& removeDirectories();

            // push new directory, empty directory names are ignored, pushing .. equals popping
            AbsolutePathBuilder& pushDirectory(StringView<wchar_t> directoryName);

            // pop last directory, poping when path has no directories will assert (non fatal)
            AbsolutePathBuilder& popDirectory();

            // append relative directory path
            AbsolutePathBuilder& pushDirectories(StringView<wchar_t> relativeDirectoryPath);

            //--

            // set drive part
            AbsolutePathBuilder& drive(StringView<wchar_t> driverPart);

            //--

            // convert back to absolute path, optionally the file part can be skipped
            AbsolutePath toAbsolutePath(bool includeFilePart = true) const;

            //--

        private:
            static const uint32_t MAX_DIRS = 32;
            static const uint32_t MAX_EXTENSIONS = 10;

            UTF16StringBuf m_drive;
            InplaceArray<UTF16StringBuf, MAX_DIRS > m_directories;
            UTF16StringBuf m_fileName;
            InplaceArray<UTF16StringBuf, MAX_EXTENSIONS> m_extensions;
        };

    } // io
} // base