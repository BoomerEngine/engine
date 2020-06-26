/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system\impl #]
* [#platform: winapi #]
***/

#pragma once

#include "absolutePath.h"
#include "base/containers/include/stringBuf.h"

#include <Windows.h>

namespace base
{
    namespace io
    {
        namespace prv
        {

            /// File iterator for enumerating directory structure
            class WinFileIterator
            {
            public:
                //! Are directories allowed ?
                INLINE bool areDirectoriesAllowed() const { return m_allowDirs; }

                //! Are files allowed ?
                INLINE bool areFilesAllowed() const { return m_allowFiles; }

                //---

                WinFileIterator(const wchar_t* pathWithPattern, bool allowFiles, bool allowDirs);
                ~WinFileIterator();

                //! Iterate to next
                void operator++(int);

                //! Iterate to next
                void operator++();

                //! Is the current file valid ?
                operator bool() const;

                //! Get current file name ( name and extension only )
                const wchar_t* fileName() const;

            private:
                //! Do we have a valid entry ?
                bool validateEntry() const;

                //! Skip until valid file is found
                bool nextEntry();

                WIN32_FIND_DATA     m_findData;
                HANDLE              m_findHandle;
                bool                m_allowDirs;
                bool                m_allowFiles;
            };

        } // prv
    } // io
} // base