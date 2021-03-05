/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: commands #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//--

class CommandCook : public app::ICommand
{
    RTTI_DECLARE_VIRTUAL_CLASS(CommandCook, app::ICommand);

public:
    virtual bool run(IProgressTracker* progress, const app::CommandLine& commandline) override final;

private:
    StringBuf m_outputDir;

    //--

    HashSet<StringBuf> m_seedFiles;
    bool collectSeedFiles();
    void scanDepotDirectoryForSeedFiles(StringView depotPath, Array<StringBuf>& outList, uint32_t& outNumDirectoriesVisited) const;

    //--

    struct PendingCookingEntry
    {
        StringBuf path;
        ResourcePtr alreadyLoadedResource;
    };

    bool processSeedFiles();
    void processSingleSeedFile(const StringBuf& key);

    bool assembleCookedOutputPath(const StringBuf& key, SpecificClassType<IResource> cookedClass, StringBuf& outPath) const;

    ResourceMetadataPtr loadFileMetadata(StringView cookedOutputPath) const;

    bool checkDependenciesUpToDate(const ResourceMetadata& deps) const;

    bool cookFile(const StringBuf& key, SpecificClassType<IResource> cookedClass, StringBuf& outPath, Array<PendingCookingEntry>& outCookingQueue);
    void queueDependencies(const IResource& object, Array<PendingCookingEntry>& outCookingQueue);
    void queueDependencies(StringView cookedFile, Array<PendingCookingEntry>& outCookingQueue);

    HashSet<StringBuf> m_allCollectedFiles;
    HashSet<StringBuf> m_allCookedFiles;
    HashSet<StringBuf> m_allSeenFile;

    uint32_t m_cookFileIndex = 0;
    uint32_t m_numTotalVisited = 0;
    uint32_t m_numTotalUpToDate = 0;
    uint32_t m_numTotalCopied = 0;
    uint32_t m_numTotalCooked = 0;
    uint32_t m_numTotalFailed = 0;

    bool m_captureLogs = true;
    bool m_discardCookedLogs = true;

    UniquePtr<CookerSaveThread> m_saveThread;
    ResourceLoader* m_loader = nullptr;

    //--
};

//--

END_BOOMER_NAMESPACE()
