/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#pragma once

#include "base/io/include/crcCache.h"

namespace base
{
    namespace res
    {

        //--

        /// "LOCAL COMPUTER" file system - basically shows all dirs/drives
        class BASE_RESOURCE_COMPILER_API SourceAssetFileSystem_LocalComputer : public ISourceAssetFileSystem
        {
        public:
            SourceAssetFileSystem_LocalComputer();
            virtual ~SourceAssetFileSystem_LocalComputer();

            // ISourceAssetFileSystem
            virtual void update() override;
            virtual bool fileExists(StringView<char> fileSystemPath, uint64_t* outCRC = nullptr) const override;
            virtual Buffer loadFileContent(StringView<char> fileSystemPath, uint64_t& outCRC) const override;
            virtual bool enumDirectoriesAtPath(StringView<char> fileSystemPath, const std::function<bool(StringView<char>)>& enumFunc) const override;
            virtual bool enumFilesAtPath(StringView<char> fileSystemPath, const std::function<bool(StringView<char>)>& enumFunc) const override;
            virtual bool translateAbsolutePath(io::AbsolutePathView absolutePath, StringBuf& outFileSystemPath) const override;

        private:
            io::CRCCache m_crcCache; // cache of file CRCs (persistent)
            io::AbsolutePath m_crcCachePath; // path to CRC cache
            

            bool convertToAbsolutePath(StringView<char> fileSystemPath, io::AbsolutePath& outAbsolutePath) const;
        };

        //--

    } // res
} // base