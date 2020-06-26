/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system\impl #]
* [#platform: winapi #]
***/

#include "build.h"
#include "ioFileIteratorWindows.h"


namespace base
{
    namespace io
    {
        namespace prv
        {

            WinFileIterator::WinFileIterator(const wchar_t* pathWithPattern, bool allowFiles, bool allowDirs)
                : m_allowDirs(allowDirs)
                , m_allowFiles(allowFiles)
            {
                //wchar_t searchPattern[1024];
                // Create the pattern
                //memcpy(searchPattern, path.data(), path.length() * sizeof(wchar_t)); // TODO: fix
				//wcscat_s(searchPattern, ARRAY_COUNT(searchPattern), pattern);

                // Format the search path
                m_findHandle = FindFirstFileW(pathWithPattern, &m_findData);

                // Get first valid entry
                while (!validateEntry())
                    if (!nextEntry())
                        break;
            }

            WinFileIterator::~WinFileIterator()
            {
                // Close search handle
                if (m_findHandle != INVALID_HANDLE_VALUE)
                {
                    FindClose(m_findHandle);
                    m_findHandle = INVALID_HANDLE_VALUE;
                }
            }

            const wchar_t* WinFileIterator::fileName() const
            {
                if (m_findHandle == INVALID_HANDLE_VALUE)
                    return nullptr;

                return m_findData.cFileName;
            }

            /*AbsolutePath WinFileIterator::filePath() const
            {
                if (m_findHandle == INVALID_HANDLE_VALUE)
                    return AbsolutePath();

                AbsolutePath ret(m_searchPath);
                return ret.appendFile(UTF16StringBuf(m_findData.cFileName));
            }*/

            bool WinFileIterator::validateEntry() const
            {
                if (m_findHandle == INVALID_HANDLE_VALUE)
                    return false;

                if (0 == wcscmp(m_findData.cFileName, L"."))
                    return false;

                if (0 == wcscmp(m_findData.cFileName, L".."))
                    return false;

                // Skip filtered
                bool isDirectory = 0 != (m_findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
                if ((isDirectory && !m_allowDirs) || (!isDirectory && !m_allowFiles))
                    return false;

                return true;
            }

            bool WinFileIterator::nextEntry()
            {
                if (m_findHandle == INVALID_HANDLE_VALUE)
                    return false;

                if (!FindNextFile(m_findHandle, &m_findData))
                {
                    FindClose(m_findHandle);
                    m_findHandle = INVALID_HANDLE_VALUE;
                    return false;
                }

                return true;
            }

            void WinFileIterator::operator++(int)
            {
                while (nextEntry())
                    if (validateEntry())
                        break;
            }

            void WinFileIterator::operator++()
            {
                while (nextEntry())
                    if (validateEntry())
                        break;
            }

            WinFileIterator::operator bool() const
            {
                return m_findHandle != INVALID_HANDLE_VALUE;
            }

        } // prv
    } // io
} // base