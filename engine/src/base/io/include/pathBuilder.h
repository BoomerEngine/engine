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
        /// helper class that can slice the absolute path into pieces
        /// NOTE: slow as fuck, use in backend only
        class BASE_IO_API PathBuilder
        {
        public:
            PathBuilder();
            PathBuilder(StringView path); // may contain ../../ -> they are collapsed

            /// get the drive name (C:\, D:\ or / under linux )
            INLINE const StringBuf& drive() const { return m_drive; }

            /// get number of directories
            INLINE uint32_t numDirectories() const { return m_directories.size(); }

            /// get directory name
            INLINE const StringBuf& directory(uint32_t index) const { return m_directories[index]; }

            /// get number of extensions
            INLINE uint32_t numExtensions() const { return m_extensions.size(); }

            /// get file extensions 
            INLINE const StringBuf& specificExtension(uint32_t index) const { return m_extensions[index]; }

            // get main extensions (last one)
            INLINE const StringBuf& mainExtension() const { return m_extensions.empty() ? StringBuf::EMPTY() : m_extensions.back(); }

            // get file name
            INLINE const StringBuf& fileName() const { return m_fileName; }

            //--

            // clear path
            PathBuilder& clear();

            // set new content
            PathBuilder& reset(StringView path);


            //--

            // remove the file name and extension, good for converting file paths to directory paths
            PathBuilder& removeFileNameAndExtension();

            // set new file name
            PathBuilder& fileName(StringView fileName);

            // append part to file name
            PathBuilder& appendFileName(StringView partialName);

            //--

            // remove all file extensions
            PathBuilder& removeExtensions();

            // remove file extension, by index
            PathBuilder& removeExtension(uint32_t index = 0);

            // set file main extension (the last one)
            // NOTE: empty extension is not valid and will assert (non fatal)
            PathBuilder& extension(StringView extension);

            // add file extensions (at the end)
            PathBuilder& addExtension(StringView extension);

            //--
            
            // remove all directories
            PathBuilder& removeDirectories();

            // push new directory, empty directory names are ignored, pushing .. equals popping
            PathBuilder& pushDirectory(StringView directoryName);

            // pop last directory, poping when path has no directories will assert (non fatal)
            PathBuilder& popDirectory();

            // append relative directory path
            PathBuilder& pushDirectories(StringView relativeDirectoryPath);

            //--

            // set drive part
            PathBuilder& drive(StringView driverPart);

            //--

            // convert back to absolute path, optionally the file part can be skipped
            StringBuf toString(bool includeFilePart = true) const;

            // get as unicode string
            Array<wchar_t> toUTF16Buffer() const;

            //--

        private:
            static const uint32_t MAX_DIRS = 32;
            static const uint32_t MAX_EXTENSIONS = 10;

            StringBuf m_drive;
            InplaceArray<StringBuf, MAX_DIRS> m_directories;
            StringBuf m_fileName;
            InplaceArray<StringBuf, MAX_EXTENSIONS> m_extensions;
        };

    } // io
} // base