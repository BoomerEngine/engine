/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

#include "base/containers/include/stringBuf.h"

namespace base
{
    namespace io
    {
        /// a specialized string for paths
        /// the path is ALWAYS absolute (starts with a drive)
        class BASE_IO_API AbsolutePath
        {
        public:
            static const wchar_t WINDOWS_PATH_SEPARATOR = '\\';
            static const wchar_t UNIX_PATH_SEPARATOR = '/';

#if defined(PLATFORM_WINDOWS)
            static const wchar_t SYSTEM_PATH_SEPARATOR = WINDOWS_PATH_SEPARATOR;
#else
            static const wchar_t SYSTEM_PATH_SEPARATOR = UNIX_PATH_SEPARATOR;
#endif

            AbsolutePath();
            AbsolutePath(const AbsolutePath& other);
            AbsolutePath(AbsolutePath&& other);

            // build path from given string, NOTE: may fail and then an empty absolute path is returned
            // only full absolute paths are supported
            // Windows file path: X:\test\crap\test.txt
            // Windows dir path: X:\test\crap\
            // Unix file path: /test/crap/test.txt
            // Unix dir path: /test/crap/
            static AbsolutePath Build(StringView<wchar_t> path);
            static AbsolutePath Build(const UTF16StringBuf& path);
            static AbsolutePath BuildAsDir(StringView<wchar_t> path);
            static AbsolutePath BuildAsDir(const UTF16StringBuf& path);

            // add a file part to the absolute path, returns new path
            AbsolutePath addFile(const UTF16StringBuf& filePath) const;
            AbsolutePath addFile(StringView<char> filePath) const;
            AbsolutePath addFile(StringView<wchar_t> filePath) const;

            // add a directory part to the absolute path, returns new path
            // NOTE: fails is called on a file path
            AbsolutePath addDir(const UTF16StringBuf& dirPath) const;
            AbsolutePath addDir(StringView<char> dirPath) const;
            AbsolutePath addDir(StringView<wchar_t> dirPath) const;

            // add a extension at the end
            // NOTE: fails is called on a file path
            AbsolutePath addExtension(const UTF16StringBuf& fileExtension) const;
            AbsolutePath addExtension(StringView<wchar_t> fileExtension) const;
            AbsolutePath addExtension(StringView<char> fileExtension) const;

            // change an extension of the file
            // NOTE: fails is called on a file path
            AbsolutePath changeExtension(const UTF16StringBuf& fileExtension) const;
            AbsolutePath changeExtension(StringView<wchar_t> fileExtension) const;
            AbsolutePath changeExtension(StringView<char> fileExtension) const;

            // add a file part to this absolute path
            AbsolutePath& appendFile(const UTF16StringBuf& filePath);
            AbsolutePath& appendFile(StringView<wchar_t> filePath);
            AbsolutePath& appendFile(StringView<char> filePath);

            // add a directory part to this absolute path
            // NOTE: fails is called on a file path
            AbsolutePath& appendDir(const UTF16StringBuf& dirPath);
            AbsolutePath& appendDir(StringView<wchar_t> dirPath);
            AbsolutePath& appendDir(StringView<char> dirPath);

            // append extensions
            AbsolutePath& appendExtension(const UTF16StringBuf& extensionName);
            AbsolutePath& appendExtension(StringView<wchar_t> extensionName);
            AbsolutePath& appendExtension(StringView<char> extensionName);

            // set from another path
            AbsolutePath& operator=(const AbsolutePath& other);

            // set from another path
            AbsolutePath& operator=(AbsolutePath&& other);

            // get path relative to given absolute base path
            UTF16StringBuf relativeTo(const AbsolutePath& rootPath) const;

            // get the file name with extension(s)
            StringView<wchar_t> fileNameWithExtensions() const;

            // get the file name without extensions
            StringView<wchar_t> fileNameNoExtensions() const;

            // get the file extensions (without the first dot)
            StringView<wchar_t> fileExtensions() const;

            // get the file last extension
            StringView<wchar_t> fileMainExtension() const;

            // get name of last directory in the path
            StringView<wchar_t> lastDirectoryName() const;

            // get the system drive name (ie. "C" for C:\) NOTE: Windows only
            StringView<wchar_t> driveName() const;

            // get the path without the file name
            AbsolutePath basePath() const;

            // get the path to parent directory (ie. for crap\data\test.txt returns crap\)
            AbsolutePath parentPath() const;

            //---

            // check if given path is based in rooted in given base path
            bool isBasedIn(const AbsolutePath& basePath) const;

            //---

            INLINE bool operator==(const AbsolutePath& other) const
            {
                return m_buf == other.m_buf;
            }

            INLINE bool operator!=(const AbsolutePath& other) const
            {
                return m_buf != other.m_buf;
            }

            INLINE bool operator<(const AbsolutePath& other) const
            {
                return m_buf < other.m_buf;
            }

            INLINE bool endsWithPathSeparator() const
            {
                return !m_buf.empty() && IsPathSeparator(m_buf.c_str()[m_buf.length() - 1]);
            }

            INLINE bool empty() const
            {
                return m_buf.empty();
            }

            INLINE operator bool() const
            {
                return !m_buf.empty();
            }

            INLINE const wchar_t* c_str() const
            {
                return m_buf.c_str();
            }

            INLINE const StringView<wchar_t> view() const
            {
                return m_buf.view();
            }

            INLINE operator StringView<wchar_t>() const
            {
                return m_buf.view();
            }

            INLINE StringBuf ansi_str() const
            {
                return m_buf.ansi_str();
            }

            INLINE static bool IsPathSeparator(wchar_t ch)
            {
                return (ch == WINDOWS_PATH_SEPARATOR) || (ch == UNIX_PATH_SEPARATOR);
            }

            INLINE const UTF16StringBuf& toString() const
            {
                return m_buf;
            }

            //---

            static bool IsValidAbsolutePath(StringView<wchar_t> str);

            //---

            INLINE static uint32_t CalcHash(const AbsolutePath& path)
            {
                return StringView<wchar_t>::CalcHash(path.m_buf.view());
            }


            INLINE static uint32_t CalcHash(StringView<wchar_t> path)
            {
                return StringView<wchar_t>::CalcHash(path);
            }

        private:
            // make sure the path ends with a path separator
            AbsolutePath& appendPathSeparator();

            // normalize path - remove all .. and . directories, returns normalized path
            // NOTE: normalization may fail if there are not enough directories
            void normalize();

            AbsolutePath(const UTF16StringBuf& buf);
            AbsolutePath(UTF16StringBuf&& buf);

            UTF16StringBuf m_buf;

            friend class AbsolutePathBuilder;
        };

    } // io
} // base
