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

        AbsolutePathBuilder::AbsolutePathBuilder()
        {}

        AbsolutePathBuilder::AbsolutePathBuilder(const AbsolutePath& path)
        {
            reset(path);
        }

        AbsolutePathBuilder::AbsolutePathBuilder(const UTF16StringBuf& relaxedPath)
        {
            reset(relaxedPath.view());
        }

        AbsolutePathBuilder::AbsolutePathBuilder(StringView<wchar_t> relaxedPath)
        {
            reset(relaxedPath);
        }

        AbsolutePathBuilder& AbsolutePathBuilder::clear()
        {
            m_drive.clear();
            m_directories.clear();
            m_fileName.clear();
            m_extensions.clear();
            return *this;
        }

        AbsolutePathBuilder& AbsolutePathBuilder::reset(const AbsolutePath& path)
        {
            return reset(path.view());
        }

        namespace helper
        {
        
            INLINE static bool IsPathSeparator(wchar_t ch)
            {
                return AbsolutePath::IsPathSeparator(ch);
            }

            INLINE static bool IsExtensionSeparator(wchar_t ch)
            {
                return ch == '.';
            }

            INLINE static bool EatPathPart(const wchar_t*& str, UTF16StringBuf& outResult)
            {
                if (!*str)
                    return false;

                // skip over redundant path separators
                while (*str)
                {
                    if (!IsPathSeparator(*str))
                        break;

                    ++str;
                }

                // eat the directory name that ends with separator
                const wchar_t* start = str;
                while (*str)
                {
                    if (IsPathSeparator(*str))
                    {
                        auto len = (uint32_t)(str - start);
                        outResult = UTF16StringBuf(start, len);

                        str += 1; // skip over the separator
                        return true;
                    }

                    ++str;
                }

                // no more directories
                str = start;
                return false;
            }

            INLINE static bool EatFileNamePart(const wchar_t*& str, UTF16StringBuf& outResult)
            {
                ASSERT(*str);

                const wchar_t* start = str;
                while (*str)
                {
                    if (IsExtensionSeparator(*str))
                        break; // the dot is eaten by the extension parser

                    ++str;
                }

                // no file name
                if (start == str)
                    return false;

                // get the file name
                auto len = (uint32_t)(str - start);
                outResult = UTF16StringBuf(start, len);

                return true;
            }
            
            INLINE static bool EatFileExtensionPart(const wchar_t*& str, UTF16StringBuf& outResult)
            {
                ASSERT(*str);
                ASSERT(IsExtensionSeparator(*str));

                str += 1; // skip over the dot

                const wchar_t* start = str;
                while (*str)
                {
                    if (IsExtensionSeparator(*str))
                        break; // the dot is eaten by the extension parser

                    ++str;
                }

                // no extension name
                if (start == str)
                    return false;

                // get the file name
                auto len = (uint32_t)(str - start);
                outResult = UTF16StringBuf(start, len);
                return true;
            }

        } // helper

        AbsolutePathBuilder& AbsolutePathBuilder::reset(StringView<wchar_t> view)
        {
            clear();

            const wchar_t* str = view.data(); // TODO: fix
            UTF16StringBuf part;

            // parse the drive path
            if (!helper::IsPathSeparator(*str))
                if (helper::EatPathPart(str, part))
                    drive(part);

            // parse the directories and paths
            while (helper::EatPathPart(str, part))
                pushDirectory(part);

            // if we are not done we expect the file name
            if (*str)
            {
                // parse the file name, may be empty
                if (helper::EatFileNamePart(str, part))
                    fileName(part);

                // parse the valid extension
                while (*str)
                    if (helper::EatFileExtensionPart(str, part))
                        addExtension(part);
            }

            // done
            return *this;
        }

        //--

        AbsolutePathBuilder& AbsolutePathBuilder::removeFileNameAndExtension()
        {
            m_fileName.clear();
            m_extensions.clear();
            return *this;
        }

        AbsolutePathBuilder& AbsolutePathBuilder::fileName(StringView<wchar_t> fileName)
        {
            m_fileName = fileName;
            return *this;
        }

        AbsolutePathBuilder& AbsolutePathBuilder::appendFileName(StringView<wchar_t> partialName)
        {
            m_fileName += partialName;
            return *this;
        }

        AbsolutePathBuilder& AbsolutePathBuilder::removeExtensions()
        {
            m_extensions.clear();
            return *this;
        }

        AbsolutePathBuilder& AbsolutePathBuilder::removeExtension(uint32_t index /*= 0*/)
        {
            if (index < m_extensions.size())
                m_extensions.erase(index);

            return *this;
        }

        AbsolutePathBuilder& AbsolutePathBuilder::extension(StringView<wchar_t> extension)
        {
            m_extensions.clear();
            m_extensions.emplaceBack(extension);
            return *this;
        }

        AbsolutePathBuilder& AbsolutePathBuilder::addExtension(StringView<wchar_t> extension)
        {
            m_extensions.emplaceBack(extension);
            return *this;
        }

        AbsolutePathBuilder& AbsolutePathBuilder::removeDirectories()
        {
            m_directories.clear();
            m_drive.clear();
            return *this;
        }

        AbsolutePathBuilder& AbsolutePathBuilder::pushDirectory(StringView<wchar_t> directoryName)
        {
            if (directoryName == L"." || directoryName.empty())
                return *this;

            if (directoryName == L"..")
                return popDirectory();

            DEBUG_CHECK_EX(!directoryName.empty(), "Directory name cannot be empty");
            m_directories.emplaceBack(directoryName);
            return *this;
        }

        AbsolutePathBuilder& AbsolutePathBuilder::popDirectory()
        {
            DEBUG_CHECK_EX(!m_directories.empty(), "Cannot pop directory on empty path");
            if (!m_directories.empty())
                m_directories.popBack();
            return *this;
        }

        AbsolutePathBuilder& AbsolutePathBuilder::pushDirectories(StringView<wchar_t> relativeDirectoryPath)
        {
            auto cur  = relativeDirectoryPath.data(); // TODO!

            UTF16StringBuf part;
            while (helper::EatPathPart(cur, part))
                pushDirectory(part);

            return *this;
        }

        AbsolutePathBuilder& AbsolutePathBuilder::drive(StringView<wchar_t> driverPart)
        {
            m_drive = driverPart;
            return *this;
        }

        //---

        AbsolutePath AbsolutePathBuilder::toAbsolutePath(bool includeFilePart /*= true*/) const
        {
            UTF16StringBuf buf;

            const wchar_t pathSeparator[] =
            {
                AbsolutePath::SYSTEM_PATH_SEPARATOR, 0
            };

            buf.append(m_drive);
            buf.append(pathSeparator);

            for (auto& dirName : m_directories)
            { 
                buf.append(dirName);
                buf.append(pathSeparator);
            }

            if (includeFilePart)
            {
                buf.append(m_fileName);

                for (auto& ext : m_extensions)
                {
                    buf.append(L".");
                    buf.append(ext);
                }
            }

            return AbsolutePath(buf);
        }

    } // io
} // base