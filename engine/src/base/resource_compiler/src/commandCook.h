/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: commands #]
***/

#pragma once

namespace base
{
    namespace res
    {
        //--

        class CommandCook : public app::ICommand
        {
            RTTI_DECLARE_VIRTUAL_CLASS(CommandCook, app::ICommand);

        public:
            virtual bool run(const app::CommandLine& commandline) override final;

        private:
            io::AbsolutePath m_outputDir;

            //--

            HashSet<ResourceKey> m_seedFiles;
            bool collectSeedFiles();
            void scanDepotDirectoryForSeedFiles(StringView<char> depotPath, Array<ResourceKey>& outList, uint32_t& outNumDirectoriesVisited) const;

            //--

            struct PendingCookingEntry
            {
                ResourceKey key;
                ResourcePtr alreadyLoadedResource;
            };

            bool processSeedFiles();
            void processSingleSeedFile(const ResourceKey& key);

            bool assembleCookedOutputPath(const ResourceKey& key, SpecificClassType<IResource> cookedClass, io::AbsolutePath& outPath) const;

            MetadataPtr loadFileMetadata(stream::IBinaryReader& reader) const;
            MetadataPtr loadFileMetadata(const io::AbsolutePath& cookedOutputPath) const;

            bool checkDependenciesUpToDate(const Metadata& deps) const;

            bool cookFile(const ResourceKey& key, SpecificClassType<IResource> cookedClass, io::AbsolutePath& outPath, Array<PendingCookingEntry>& outCookingQueue);
            void queueDependencies(const IResource& object, Array<PendingCookingEntry>& outCookingQueue);
            void queueDependencies(const io::AbsolutePath& cookedFile, Array<PendingCookingEntry>& outCookingQueue);

            HashSet<ResourceKey> m_allCollectedFiles;
            HashSet<ResourceKey> m_allCookedFiles;
            HashSet<ResourceKey> m_allSeenFile;

            uint32_t m_cookFileIndex = 0;
            uint32_t m_numTotalVisited = 0;
            uint32_t m_numTotalUpToDate = 0;
            uint32_t m_numTotalCopied = 0;
            uint32_t m_numTotalCooked = 0;
            uint32_t m_numTotalFailed = 0;

            bool m_captureLogs = true;
            bool m_discardCookedLogs = true;

            UniquePtr<Cooker> m_cooker;
            UniquePtr<CookerSaveThread> m_saveThread;
            IResourceLoader* m_loader = nullptr;

            //--
        };

        //--

    } // res
} // base
