/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: cooking #]
***/

#pragma once

#include "base/resource/include/resourceCookingInterface.h"
#include "base/resource/include/resourceMetadata.h"

namespace base
{
    namespace res
    {
        /// a simple local implementation of cooking interface
        class CookerInterface : public IResourceCookerInterface
        {
        public:
            CookerInterface(IResourceLoader* dependencyLoader, StringView referenceFilePath, bool finalCooker, IProgressTracker* externalProgressTracker = nullptr);
            virtual ~CookerInterface();

            virtual bool checkCancelation() const override final;
            virtual void reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text) override final;

            virtual const StringBuf& queryResourcePath() const override final;
            virtual const StringBuf& queryResourceContextName() const override final;
            virtual bool queryResolvedPath(StringView relativePath, StringView contextFileSystemPath, bool isLocal, StringBuf& outFileSystemPath) override final;
            virtual bool queryContextName(StringView fileSystemPath, StringBuf& contextName) override final;
            virtual bool queryFileExists(StringView fileSystemPath) const override final;

            virtual bool discoverResolvedPaths(StringView relativeDirPath, bool recurse, StringView extension, Array<StringBuf>& outFileSystemPaths) override final;

            virtual io::ReadFileHandlePtr createReader(StringView fileSystemPath) override final;
            virtual Buffer loadToBuffer(StringView fileSystemPath) override final;
            virtual bool loadToString(StringView fileSystemPath, StringBuf& outContent) override final;

            virtual ResourceHandle loadDependencyResource(const ResourceKey& key) override final;

            virtual bool findFile(StringView contextPath, StringView inputPath, StringBuf& outFileSystemPath, uint32_t maxScanDepth = 2) override final;

            virtual bool finalCooker() const override final;

            //--

            typedef Array<SourceDependency> TReportedDependencies;
            INLINE const TReportedDependencies& generatedDependencies() const { return m_dependencies; }

        private:
            StringBuf m_referencePath;
            StringBuf m_referenceContextName;

            DepotService* m_depot = nullptr;
            IResourceLoader* m_loader;

            bool m_finalCooker = false;

            TReportedDependencies m_dependencies;

            IProgressTracker* m_externalProgressTracker = nullptr;

            bool touchFile(StringView systemPath);

            void enumFiles(StringView systemPath, bool recurse, StringView extension, Array<StringBuf>& outFileSystemPaths, io::TimeStamp& outNewestTimeStamp);
        };

    } // depot
} // base
