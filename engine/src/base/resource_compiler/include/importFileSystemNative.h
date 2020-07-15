/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#pragma once

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
            virtual bool fileExists(StringView<char> fileSystemPath) const override;
            virtual Buffer loadFileContent(StringView<char> fileSystemPath, ImportFileFingerprint& outFingerprint) const override;
            virtual bool enumDirectoriesAtPath(StringView<char> fileSystemPath, const std::function<bool(StringView<char>)>& enumFunc) const override;
            virtual bool enumFilesAtPath(StringView<char> fileSystemPath, const std::function<bool(StringView<char>)>& enumFunc) const override;
            virtual bool translateAbsolutePath(io::AbsolutePathView absolutePath, StringBuf& outFileSystemPath) const override;
            virtual bool resolveContextPath(StringView<char> fileSystemPath, StringBuf& outContextPath) const override;
            virtual CAN_YIELD SourceAssetStatus checkFileStatus(StringView<char> fileSystemPath, uint64_t lastKnownTimestamp, const ImportFileFingerprint& lastKnownFingerprint, IProgressTracker* progress) const override;

        private:
            bool convertToAbsolutePath(StringView<char> fileSystemPath, io::AbsolutePath& outAbsolutePath) const;
        };

        //--

    } // res
} // base