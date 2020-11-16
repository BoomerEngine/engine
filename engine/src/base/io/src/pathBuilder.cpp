/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#include "build.h"
#include "pathBuilder.h"

namespace base
{
    namespace io
    {

        PathBuilder::PathBuilder()
        {}

        PathBuilder::PathBuilder(StringView<char> path)
        {
            reset(path);
        }

        PathBuilder& PathBuilder::clear()
        {
            m_drive.clear();
            m_fileName.clear();
            m_directories.reset();
            m_extensions.reset();
            return *this;
        }

        namespace helper
        {
        
            INLINE static bool IsPathSeparator(char ch)
            {
                return ch == '/' || ch == '\\';
            }

            INLINE static bool IsExtensionSeparator(char ch)
            {
                return ch == '.';
            }

            INLINE static bool EatPathPart(const char*& str, StringBuf& outResult)
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
                const char* start = str;
                while (*str)
                {
                    if (IsPathSeparator(*str))
                    {
                        auto len = (uint32_t)(str - start);
                        outResult = StringBuf(start, len);

                        str += 1; // skip over the separator
                        return true;
                    }

                    ++str;
                }

                // no more directories
                str = start;
                return false;
            }

            INLINE static bool EatFileNamePart(const char*& str, StringBuf& outResult)
            {
                ASSERT(*str);

                const char* start = str;
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
                outResult = StringBuf(start, len);

                return true;
            }
            
            INLINE static bool EatFileExtensionPart(const char*& str, StringBuf& outResult)
            {
                ASSERT(*str);
                ASSERT(IsExtensionSeparator(*str));

                str += 1; // skip over the dot

                const char* start = str;
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
                outResult = StringBuf(start, len);
                return true;
            }

        } // helper

        PathBuilder& PathBuilder::reset(StringView<char> view)
        {
            clear();

            const char* str = view.data(); // TODO: fix
            StringBuf part;

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

        PathBuilder& PathBuilder::removeFileNameAndExtension()
        {
            m_fileName.clear();
            m_extensions.clear();
            return *this;
        }

        PathBuilder& PathBuilder::fileName(StringView<char> fileName)
        {
            m_fileName = StringBuf(fileName);
            return *this;
        }

        PathBuilder& PathBuilder::appendFileName(StringView<char> partialName)
        {
            m_fileName = TempString("{}{}", m_fileName, partialName);
            return *this;
        }

        PathBuilder& PathBuilder::removeExtensions()
        {
            m_extensions.clear();
            return *this;
        }

        PathBuilder& PathBuilder::removeExtension(uint32_t index /*= 0*/)
        {
            if (index < m_extensions.size())
                m_extensions.erase(index);

            return *this;
        }

        PathBuilder& PathBuilder::extension(StringView<char> extension)
        {
            m_extensions.clear();
            m_extensions.emplaceBack(extension);
            return *this;
        }

        PathBuilder& PathBuilder::addExtension(StringView<char> extension)
        {
            m_extensions.emplaceBack(extension);
            return *this;
        }

        PathBuilder& PathBuilder::removeDirectories()
        {
            m_directories.clear();
            m_drive.clear();
            return *this;
        }

        PathBuilder& PathBuilder::pushDirectory(StringView<char> directoryName)
        {
            if (directoryName == "." || directoryName.empty())
                return *this;

            if (directoryName == "..")
                return popDirectory();

            DEBUG_CHECK_EX(!directoryName.empty(), "Directory name cannot be empty");
            m_directories.emplaceBack(directoryName);
            return *this;
        }

        PathBuilder& PathBuilder::popDirectory()
        {
            DEBUG_CHECK_EX(!m_directories.empty(), "Cannot pop directory on empty path");
            if (!m_directories.empty())
                m_directories.popBack();
            return *this;
        }

        PathBuilder& PathBuilder::pushDirectories(StringView<char> relativeDirectoryPath)
        {
            auto cur  = relativeDirectoryPath.data(); // TODO!

            StringBuf part;
            while (helper::EatPathPart(cur, part))
                pushDirectory(part);

            return *this;
        }

        PathBuilder& PathBuilder::drive(StringView<char> driverPart)
        {
            m_drive = StringBuf(driverPart);
            return *this;
        }

        //---

        StringBuf PathBuilder::toString(bool includeFilePart /*= true*/) const
        {
            StringBuilder buf;

            const char pathSeparator[] =
            {
                SYSTEM_PATH_SEPARATOR, 0
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

            return buf.toString();
        }

    } // io
} // base