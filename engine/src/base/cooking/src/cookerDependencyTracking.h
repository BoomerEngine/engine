/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: cooking #]
***/

#pragma once

#include "base/system/include/spinLock.h"
#include "base/system/include/mutex.h"
#include "base/resources/include/resourceMetadata.h"
#include "base/depot/include/depotStructure.h"
#include "base/containers/include/hashSet.h"

namespace base
{
    namespace cooker
    {

        class DependencyTracker;

        //--

        // helper class for tracking changes to the assets
        class DependencyTracker : public depot::IDepotObserver
        {
        public:
            DependencyTracker(depot::DepotStructure& depot);
            ~DependencyTracker();

            //--

            /// check if the resource file we want to load is up to date
            /// NOTE: this always rechecks the CRCs of the dependencies (if known)
            bool checkUpToDate(const res::ResourceKey& key) CAN_YIELD;

            /// query list of files that had their dependencies changed since last call to this function
            /// NOTE: calling this function "validates" the file so it's not reported again unless it's changed again
            void queryFilesForReloading(base::Array<res::ResourceKey>& outReloadList);

            //--

            /// notify that we have a new dependencies for a given resource
            void notifyDependenciesChanged(const res::ResourceKey& key, const Array<res::SourceDependency>& newDependencies);

            //--

        private:
            //--

            depot::DepotStructure& m_depot;

            //--

            struct TrackedDepotFile;
            struct TrackedCookedFile;

            struct TrackedDepotDir
            {
                TrackedDepotDir* parent = nullptr;
                StringBuf name;
                StringBuf depotPath;
                Array<TrackedDepotDir*> dirs;
                Array<TrackedDepotFile*> files;
            };

            struct TrackedDepotFile
            {
                TrackedDepotDir* parent = nullptr;
                StringBuf depotPath;
                StringBuf name;

                HashSet<TrackedCookedFile*> users;
                SpinLock usersLock;
            };

            typedef HashMap<StringBuf, TrackedDepotFile*> TSourceAssetsMap;
            typedef Array<TrackedDepotFile*> TSourceAssetsList;
            typedef Array<TrackedDepotDir*> TSourceAssetsDirList;
            TSourceAssetsMap m_sourceAssetsMap;
            TSourceAssetsList m_sourceAssets;
            TSourceAssetsDirList  m_sourceAssetDirs;
            TrackedDepotDir* m_sourceAssetRootDir;

            SpinLock m_sourceAssetsLock;

            TrackedDepotFile* fileEntry(StringView<char> filePath, bool createIfMissing);
            TrackedDepotDir* childDirectory_NoLock(TrackedDepotDir* parentDir, StringView<char>dirName, bool createIfMissing);
            TrackedDepotFile* fileEntry_NoLock(TrackedDepotDir* parentDir, StringView<char>fileName, bool createIfMissing);

            //---

            struct TrackedFileDependency
            {
                TrackedDepotFile* file = nullptr; // file required for cooking the resource
                uint64_t fileSize = 0; 
                uint64_t timestamp = 0;
            };

            struct TrackedCookedFile
            {
                res::ResourceKey key;
                Array<TrackedFileDependency> dependencies;
                SpinLock dependenciesLock;
                std::atomic<uint32_t> changed = 0;
            };

            typedef HashMap<res::ResourceKey, TrackedCookedFile*> TCookedAssetsMap;
            typedef Array<TrackedCookedFile*> TCookedAssets;
            TCookedAssetsMap m_cookedAssetsMap;
            TCookedAssets m_cookedAssets;
            SpinLock m_cookedAssetsLock;

            TCookedAssets m_changedAssets;
            SpinLock m_changedAssetsLock;

            //--

            TrackedCookedFile* resourceEntry(const res::ResourceKey& resourcePath, bool createIfMissing);

            bool checkUpToDate(const TrackedCookedFile& entry) CAN_YIELD;

            //--

            virtual void notifyFileChanged(StringView<char> depotFilePath) override;
            virtual void notifyFileAdded(StringView<char> depotFilePath) override;
        };

    } // cooker
} // base
