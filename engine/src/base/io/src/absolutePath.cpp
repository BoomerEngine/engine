/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#include "build.h"
#include "absolutePath.h"
#include "absolutePathBuilder.h"

namespace base
{
    namespace io
    {

        AbsolutePath::AbsolutePath()
        {}

        AbsolutePath::AbsolutePath(const AbsolutePath& other)
            : m_buf(other.m_buf)
        {}

        AbsolutePath::AbsolutePath(AbsolutePath&& other)
        {
            m_buf = std::move(other.m_buf);
        }

        AbsolutePath::AbsolutePath(const UTF16StringBuf& buf)
            : m_buf(buf)
        {}

        AbsolutePath::AbsolutePath(UTF16StringBuf&& buf)
            : m_buf(std::move(buf))
        {}

        AbsolutePath AbsolutePath::Build(const UTF16StringBuf& path)
        {
            return AbsolutePath::Build(path.view());
        }

        AbsolutePath AbsolutePath::BuildAsDir(const UTF16StringBuf& path)
        {
            return AbsolutePath::BuildAsDir(path.view());
        }

        AbsolutePath AbsolutePath::BuildAsDir(StringView<wchar_t> path)
        {
            if (path.empty())
                return AbsolutePath();

            /*if (!IsValidAbsolutePath(path))
                return AbsolutePath();*/

            UTF16StringBuf pathBuffer(path);

            auto ch  = (wchar_t*)pathBuffer.c_str();
            while (*ch)
            {
                if (IsPathSeparator(*ch))
                    *ch = SYSTEM_PATH_SEPARATOR;

                ++ch;
            }

            wchar_t dirEnd[2] = {SYSTEM_PATH_SEPARATOR, 0};
            if (!pathBuffer.endsWith(dirEnd))
                pathBuffer += dirEnd;

            //AbsolutePathBuilder builder(pathBuffer);
            //return builder.toAbsolutePath();
            return AbsolutePath(std::move(pathBuffer));
        }

        AbsolutePath AbsolutePath::Build(StringView<wchar_t> path)
        {
            if (path.empty())
                return AbsolutePath();

            /*if (!IsValidAbsolutePath(path))
                return AbsolutePath();*/

            UTF16StringBuf pathBuffer(path);

            auto ch  = (wchar_t*)pathBuffer.c_str();
            while (*ch)
            {
                if (IsPathSeparator(*ch))
                    *ch = SYSTEM_PATH_SEPARATOR;

                ++ch;
            }
            
            //AbsolutePathBuilder builder(pathBuffer);
            //return builder.toAbsolutePath();
            return AbsolutePath(std::move(pathBuffer));
        }

        //--

        AbsolutePath AbsolutePath::addFile(const UTF16StringBuf& filePath) const
        {
            AbsolutePath copy(*this);
            return copy.appendFile(filePath);
        }

        AbsolutePath AbsolutePath::addFile(StringView<char> filePath) const
        {
            AbsolutePath copy(*this);
            return copy.appendFile(filePath);
        }

        AbsolutePath AbsolutePath::addFile(StringView<wchar_t> filePath) const
        {
            AbsolutePath copy(*this);
            return copy.appendFile(filePath);
        }

        //--

        AbsolutePath AbsolutePath::addDir(const UTF16StringBuf& dirPath) const
        {
            AbsolutePath copy(*this);
            return copy.appendDir(dirPath);
        }

        AbsolutePath AbsolutePath::addDir(StringView<char> dirPath) const
        {
            AbsolutePath copy(*this);
            return copy.appendDir(dirPath);
        }

        AbsolutePath AbsolutePath::addDir(StringView<wchar_t> dirPath) const
        {
            AbsolutePath copy(*this);
            return copy.appendDir(dirPath);
        }

        //--

        AbsolutePath AbsolutePath::addExtension(const UTF16StringBuf& fileExtension) const
        {
            AbsolutePath copy(*this);
            return copy.appendExtension(fileExtension);
        }

        AbsolutePath AbsolutePath::addExtension(StringView<wchar_t> fileExtension) const
        {
            AbsolutePath copy(*this);
            return copy.appendExtension(fileExtension);
        }

        AbsolutePath AbsolutePath::addExtension(StringView<char> fileExtension) const
        {
            AbsolutePath copy(*this);
            return copy.appendExtension(fileExtension);
        }

        //--

        AbsolutePath AbsolutePath::changeExtension(const UTF16StringBuf& fileExtension) const
        {
            AbsolutePathBuilder builder(*this);
            builder.extension(fileExtension);
            return builder.toAbsolutePath();
        }

        AbsolutePath AbsolutePath::changeExtension(StringView<wchar_t> fileExtension) const
        {
            AbsolutePathBuilder builder(*this);
            builder.extension(fileExtension);
            return builder.toAbsolutePath();
        }

        AbsolutePath AbsolutePath::changeExtension(StringView<char> fileExtension) const
        {
            AbsolutePathBuilder builder(*this);
            builder.extension(fileExtension);
            return builder.toAbsolutePath();
        }

        //--

        AbsolutePath& AbsolutePath::appendFile(const UTF16StringBuf& filePath)
        {
            return appendFile(filePath.view());
        }

        AbsolutePath& AbsolutePath::appendFile(StringView<wchar_t> filePath)
        {
            DEBUG_CHECK_EX(!m_buf.empty() && endsWithPathSeparator(), "Unable to append directories to empty path or a file path");

            if (!filePath.empty())
            {
                m_buf += filePath;
                normalize();
            }

            return *this;
        }

        AbsolutePath& AbsolutePath::appendFile(StringView<char> filePath)
        {
            DEBUG_CHECK_EX(!m_buf.empty() && endsWithPathSeparator(), "Unable to append directories to empty path or a file path");

            if (!filePath.empty())
            {
                m_buf += filePath;
                normalize();
            }

            return *this;
        }

        //--

        AbsolutePath& AbsolutePath::appendDir(const UTF16StringBuf& dirPath)
        {
            return appendDir(dirPath.view());
        }

        AbsolutePath& AbsolutePath::appendDir(StringView<wchar_t> dirPath)
        {
            DEBUG_CHECK_EX(!m_buf.empty() && endsWithPathSeparator(), "Unable to append directories to empty path or a file path");

            if (!dirPath.empty())
            {
                m_buf += dirPath;
                appendPathSeparator();
                normalize();
            }

            return *this;
        }

        AbsolutePath& AbsolutePath::appendDir(StringView<char> dirPath)
        {
            DEBUG_CHECK_EX(!m_buf.empty() && endsWithPathSeparator(), "Unable to append directories to empty path or a file path");

            if (!dirPath.empty())
            {
                m_buf += dirPath;
                appendPathSeparator();
                normalize();
            }

            return *this;
        }

        //--

        AbsolutePath& AbsolutePath::appendExtension(const UTF16StringBuf& extensionName)
        {
            return appendExtension(extensionName.view());
        }

        AbsolutePath& AbsolutePath::appendExtension(StringView<wchar_t> extensionName)
        {
            DEBUG_CHECK_EX(!m_buf.empty() && !endsWithPathSeparator(), "Unble to append extension to something that is not a file name");
            if (!extensionName.beginsWith(L"."))
                m_buf += L".";
            m_buf += extensionName;
            return *this;
        }

        AbsolutePath& AbsolutePath::appendExtension(StringView<char> extensionName)
        {
            DEBUG_CHECK_EX(!m_buf.empty() && !endsWithPathSeparator(), "Unble to append extension to something that is not a file name");
            if (!extensionName.beginsWith("."))
                m_buf += L".";
            m_buf += extensionName;
            return *this;
        }

        //--

        AbsolutePath& AbsolutePath::appendPathSeparator()
        {
            if (!m_buf.empty() && !endsWithPathSeparator())
            {
                const wchar_t txt[] = { SYSTEM_PATH_SEPARATOR, 0 };
                m_buf.append(txt);
            }

            return *this;
        }

        void AbsolutePath::normalize()
        {
            AbsolutePathBuilder builder(*this);
            m_buf = builder.toAbsolutePath().m_buf;
        }

        bool AbsolutePath::isBasedIn(const AbsolutePath& basePath) const
        {
            // special case for empty paths
            if (basePath.empty())
                return empty();

            // normal empty path is not based in anything
            if (empty())
                return false;

            // if the base path is not a directory it cannot be used as a base
            if (!basePath.endsWithPathSeparator())
                return false;

            // check if the string matches
            return m_buf.beginsWith(basePath.m_buf);
        }

        UTF16StringBuf AbsolutePath::relativeTo(const AbsolutePath& rootPath) const
        {
            // if we are not directly based we MAY be a more complex relative path
            if (!m_buf.beginsWith(rootPath.m_buf))
            {
                // we will be a relative path
                const wchar_t pathSeparatorStr[4] = {'.', '.', SYSTEM_PATH_SEPARATOR, 0};
                UTF16StringBuf ret(pathSeparatorStr);

                // try the relative paths
                auto basePath = rootPath.parentPath();
                while (!basePath.empty())
                {
                    // ok ?
                    if (m_buf.beginsWith(basePath))
                    {
                        auto restOfPath = StringView<wchar_t>(m_buf.c_str() + basePath.m_buf.length(), m_buf.c_str() + m_buf.length());
                        ret += restOfPath;
                        return ret;
                    }

                    // go to parent dir
                    basePath = basePath.parentPath();
                    ret += pathSeparatorStr;
                }

                return UTF16StringBuf();
            }

            return UTF16StringBuf(m_buf.c_str() + rootPath.m_buf.length());
        }

        StringView<wchar_t> AbsolutePath::driveName() const
        {
            if (empty())
                return StringView<wchar_t>();

            // get the base path
            const wchar_t txt[] = { SYSTEM_PATH_SEPARATOR, 0 };
            auto strippedPath = m_buf.view().beforeLast(txt);
            ASSERT(!strippedPath.empty());

            // get the last directory name
            return strippedPath.beforeFirst(txt);
        }

        StringView<wchar_t> AbsolutePath::lastDirectoryName() const
        {
            if (empty())
                return StringView<wchar_t>();

            // get the base path
            const wchar_t txt[] = { SYSTEM_PATH_SEPARATOR, 0 };
            auto strippedPath = m_buf.view().beforeLast(txt);
            ASSERT(!strippedPath.empty());

            // get the last directory name
            return strippedPath.afterLast(txt);
        }

        StringView<wchar_t> AbsolutePath::fileNameWithExtensions() const
        {
            if (empty() || endsWithPathSeparator())
                return StringView<wchar_t>();

            const wchar_t txt[] = { SYSTEM_PATH_SEPARATOR, 0 };
            return m_buf.view().afterLast(txt);
        }

        StringView<wchar_t> AbsolutePath::fileNameNoExtensions() const
        {
            auto fileNameWithExtensions = this->fileNameWithExtensions();
            if (fileNameWithExtensions.empty())
                return StringView<wchar_t>();

            return fileNameWithExtensions.beforeFirst(L".");
        }

        StringView<wchar_t> AbsolutePath::fileExtensions() const
        {
            auto fileNameWithExtensions = this->fileNameWithExtensions();
            if (fileNameWithExtensions.empty())
                return StringView<wchar_t>();

            return fileNameWithExtensions.afterFirst(L".");
        }

        StringView<wchar_t> AbsolutePath::fileMainExtension() const
        {
            auto fileNameWithExtensions = this->fileNameWithExtensions();
            if (fileNameWithExtensions.empty())
                return StringView<wchar_t>();

            return fileNameWithExtensions.afterLast(L".");
        }        

        AbsolutePath AbsolutePath::basePath() const
        {
            // path case
            if (empty() || endsWithPathSeparator())
                return *this;

            // get the path
            const wchar_t txt[] = { SYSTEM_PATH_SEPARATOR, 0 };
            auto strippedPath = m_buf.stringBeforeLast(txt);
            ASSERT(!strippedPath.empty());

            // append the path separator
            strippedPath.append(txt);
            return strippedPath;
        }

        AbsolutePath AbsolutePath::parentPath() const
        {
            // path case
            if (empty())
                return AbsolutePath();

            // get the base path
            const wchar_t txt[] = { SYSTEM_PATH_SEPARATOR, 0 };
            auto strippedPath = m_buf.stringBeforeLast(txt);
            if (strippedPath.empty())
                return AbsolutePath();

            // get parent path
            auto parentPath = strippedPath.stringBeforeLast(txt);
            parentPath.append(txt);
            return parentPath;
        }

        AbsolutePath& AbsolutePath::operator=(const AbsolutePath& other)
        {
            m_buf = other.m_buf;
            return *this;
        }

        AbsolutePath& AbsolutePath::operator=(AbsolutePath&& other)
        {
            m_buf = std::move(other.m_buf);
            return *this;
        }

        //---

        namespace helper
        {
            static bool EatDriveWindows(const wchar_t*& str)
            {
                if ((str[0] >= 'A' && str[0] <= 'Z') || (str[0] >= 'a' && str[0] <= 'z'))
                {
                    if (str[1] == ':')
                    {
                        if (str[2] == AbsolutePath::WINDOWS_PATH_SEPARATOR)
                        {
                            str += 3;
                            return true;
                        }
                    }
                }

                return false;
            }

            static bool EatDriveLinux(const wchar_t*& str)
            {
                if (AbsolutePath::IsPathSeparator(str[0]))
                {
                    str += 1;
                    return true;
                }

                return false;
            }

            // is this a valid folder directory name ?
            static bool IsFolderChar(wchar_t ch)
            {
                if (ch < 32)
                    return false;

                if (AbsolutePath::IsPathSeparator(ch))
                    return false;

                return true;
            }

            // must end with path separator
            static bool EatFolder(const wchar_t*& str)
            {
                const wchar_t* start = str;
                while (*str)
                {
                    auto ch = *str++;

                    if (AbsolutePath::IsPathSeparator(ch))
                        return true;
                }

                // we haven't ended with a path separator
                str = start;
                return false;
            }

            // we must have file name
            static bool EatFileName(const wchar_t*& str)
            {
                while (*str)
                {
                    auto ch = *str++;
                    if (ch == '.')
                    {
                        str -= 1;
                        return true;
                    }
                }

                // we haven't ended with a dot
                return false;
            }

            // we must have file name
            static bool EatFileExtension(const wchar_t*& str)
            {
                if (*str != '.')
                    return false;

                str += 1; // skip the intro dot

                const wchar_t* start = str;
                while (*str)
                {
                    auto ch = *str++;
                    if (ch == '.')
                    {
                        str -= 1;
                        return (str - start) > 1; // empty extensions are not supported
                    }
                }

                return (str - start) >= 1; // empty extensions are not supported
            }

        } // helper

        bool AbsolutePath::IsValidAbsolutePath(StringView<wchar_t> strView)
        {
            if (!strView.empty())
                return true;

            // eat the drive
            const auto* str = strView.data(); // TODO: FIX
            if (IsPathSeparator(str[0]))
            {
                if (!helper::EatDriveLinux(str))
                    return false;
            }
            else if (str[1] == ':')
            {
                if (!helper::EatDriveWindows(str))
                    return false;
            }
            else
            {
                return false;
            }

            // eat the directories
            while (helper::EatFolder(str))
            {}

            // more 
            if (*str)
            {
                // eat file name
                helper::EatFileName(str);

                // eat extension
                while (*str)
                {
                    if (!helper::EatFileExtension(str))
                        return false;
                }
            }

            // valid path
            return true;
        }

        //---

    } // io
} // base

