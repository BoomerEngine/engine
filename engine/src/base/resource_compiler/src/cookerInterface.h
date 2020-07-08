/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: cooking #]
***/

#pragma once

#include <base/resource/include/resourceMountPoint.h>
#include "base/io/include/absolutePath.h"
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
            CookerInterface(const depot::DepotStructure& depot, IResourceLoader* dependencyLoader, const ResourcePath& referenceFilePath, const ResourceMountPoint& referenceMountingPoint, bool finalCooker, IProgressTracker* externalProgressTracker = nullptr);
            virtual ~CookerInterface();

            virtual bool checkCancelation() const override final;
            virtual void reportProgress(uint64_t currentCount, uint64_t totalCount, StringView<char> text) override final;

            virtual const ResourcePath& queryResourcePath() const override final;
            virtual const ResourceMountPoint& queryResourceMountPoint() const override final;
            virtual const StringBuf& queryResourceContextName() const override final;
            virtual bool queryResolvedPath(StringView<char> relativePath, StringView<char> contextFileSystemPath, bool isLocal, StringBuf& outFileSystemPath) override final;
            virtual bool queryContextName(StringView<char> fileSystemPath, StringBuf& contextName) override final;
            virtual bool queryFileInfo(StringView<char> fileSystemPath, uint64_t* outCRC, uint64_t* outSize, io::TimeStamp* outTimeStamp, bool makeDependency = true) override final;

            virtual bool discoverResolvedPaths(StringView<char> relativeDirPath, bool recurse, StringView<char> extension, Array<StringBuf>& outFileSystemPaths) override final;

            virtual io::FileHandlePtr createReader(StringView<char> fileSystemPath) override final;
            virtual Buffer loadToBuffer(StringView<char> fileSystemPath) override final;
            virtual bool loadToString(StringView<char> fileSystemPath, StringBuf& outContent) override final;

            virtual ResourceHandle loadManifestFile(StringView<char> outputPartName, ClassType expectedManifestClass) override final;
            virtual ResourceHandle loadDependencyResource(const ResourceKey& key) override final;

            virtual bool findFile(StringView<char> contextPath, StringView<char> inputPath, StringBuf& outFileSystemPath, uint32_t maxScanDepth = 2) override final;

            virtual bool finalCooker() const override final;

            //--

            typedef Array<SourceDependency> TReportedDependencies;
            INLINE const TReportedDependencies& generatedDependencies() const { return m_dependencies; }

        private:
            ResourcePath m_referencePath;
            ResourceMountPoint m_referenceMountingPoint;
            StringBuf m_referenceContextName;
            StringBuf m_referencePathBase;

            const depot::DepotStructure& m_depot;
            IResourceLoader* m_loader;

            bool m_finalCooker = false;

            TReportedDependencies m_dependencies;

            IProgressTracker* m_externalProgressTracker = nullptr;

            ResourceHandle loadUncachedFile(StringView<char> fileSystemPath, ClassType expectedClassType);

            bool touchFile(StringView<char> systemPath);

            void enumFiles(StringView<char> systemPath, bool recurse, StringView<char> extension, Array<StringBuf>& outFileSystemPaths, uint64_t& outFileNamesHash, io::TimeStamp& outNewestTimeStamp);
        };

    } // depot
} // base
