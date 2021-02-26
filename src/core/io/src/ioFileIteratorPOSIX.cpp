/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system\posix #]
* [#platform: posix #]
***/

#include "build.h"
#include "ioFileIteratorPOSIX.h"

#include "core/containers/include/utf8StringFunctions.h"

#include <dirent.h>

namespace base
{
    namespace io
    {
        namespace prv
        {

            POSIXFileIterator::POSIXFileIterator(const StringBuf& path, const wchar_t* pattern, bool allowFiles, bool allowDirs)
                : m_allowDirs(allowDirs)
                , m_allowFiles(allowFiles)
                , m_searchPath(path)
                , m_searchPattern(pattern)
                , m_dir(nullptr)
                , m_entry(nullptr)
            {
                char filePath[512];
                utf8::FromUniChar(filePath, sizeof(filePath), path.c_str(), wcslen(path.c_str()));

                // Format the search path
                m_dir = opendir(filePath);
                m_entry = m_dir ? readdir((DIR*)m_dir) : nullptr;

                // Get first valid entry
                while (!validateEntry())
                    if (!nextEntry())
                        break;
            }

            POSIXFileIterator::~POSIXFileIterator()
            {
                // Close search handle
                if (m_dir != 0)
                {
                    closedir((DIR*)m_dir);
                    m_dir = 0;
                }
            }

            const char* POSIXFileIterator::fileName() const
            {
                if (m_entry == 0)
                    return nullptr;

                return ((struct dirent*)m_entry)->d_name;
            }

            StringBuf POSIXFileIterator::filePath() const
            {
                if (m_entry == 0)
                    return StringBuf();

                const auto* fileName = (const char*) ((struct dirent*)m_entry)->d_name;
                return TempString("{}{}", m_searchPath, fileName);
            }

            bool POSIXFileIterator::validateEntry() const
            {
                if (m_entry == 0)
                    return false;

                auto fileEntry  = ((struct dirent*)m_entry);
                auto fileName  = fileEntry->d_name;
                if (0 == strcmp(fileName, "."))
                    return false;

                if (0 == strcmp(fileName, ".."))
                    return false;

                // Skip filtered
                bool isDirectory = (fileEntry->d_type == DT_DIR);
                if ((isDirectory && !m_allowDirs) || (!isDirectory && !m_allowFiles))
                    return false;

                // check pattern
                if (m_searchPattern != "*.*" && m_searchPattern != "*.")
                {
                    if (!StringView(fileName).matchString(m_searchPattern.view()))
                        return false;
                }

                // entry can be used
                return true;
            }

            bool POSIXFileIterator::nextEntry()
            {
                if (m_dir == 0)
                    return false;

                m_entry = readdir((DIR*)m_dir);
                if (!m_entry)
                {
                    closedir((DIR*)m_dir);
                    m_dir = nullptr;
                }

                return true;
            }

            void POSIXFileIterator::operator++(int)
            {
                while (nextEntry())
                    if (validateEntry())
                        break;
            }

            void POSIXFileIterator::operator++()
            {
                while (nextEntry())
                    if (validateEntry())
                        break;
            }

            POSIXFileIterator::operator bool() const
            {
                return m_entry != 0;
            }

        } // prv
    } // io
} // base