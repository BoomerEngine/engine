/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#include "build.h"
#include "importer.h"

#include "core/resource/include/metadata.h"
#include "core/resource/include/depot.h"
#include "core/containers/include/path.h"

BEGIN_BOOMER_NAMESPACE()

//--

IImportDepotLoader::~IImportDepotLoader()
{}

//--

IImportProgressTracker::~IImportProgressTracker()
{}

//--

class ImportQueueProgressTracker : public IProgressTracker
{
public:
    ImportQueueProgressTracker(IImportProgressTracker& parentTracker, StringView depotPath)
        : m_depotPath(depotPath)
        , m_parentTracker(parentTracker)
    {}

    virtual bool checkCancelation() const override final
    {
        return m_parentTracker.checkCancelation();
    }

    virtual void reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text) override final
    {
        m_parentTracker.jobProgressUpdate(m_depotPath, currentCount, totalCount, text);
    }

private:
    StringBuf m_depotPath;
    IImportProgressTracker& m_parentTracker;
};

//--

class ImportStackHelper : public NoCopy
{
public:
    ImportStackHelper(const IImportDepotLoader* loader, IImportProgressTracker& progress)
        : m_importer(loader)
        , m_progress(progress)
        , m_loader(loader ? loader : &IImportDepotLoader::GetGlobalDepotLoader())
    {
    }

    ~ImportStackHelper()
    {
        TRACE_INFO("Finished importing, run {} jobs in {}. Imported {} files, {} up to date, {} failed",
            m_numJobs, m_timer, m_numImported, m_numUpToDate, m_numFailed);
    }

    bool validateImportJob(ImportJobInfo& job) const
    {
        DEBUG_CHECK_RETURN_EX_V(!job.depotFilePath.empty(), "Depot path is empty", false);
        DEBUG_CHECK_RETURN_EX_V(job.depotFilePath.endsWith(IResource::FILE_EXTENSION), "Depot file path is not valid", false);

        const auto metadataPath = ReplaceExtension(job.depotFilePath, ResourceMetadata::FILE_EXTENSION);
        if (const auto metadata = m_loader->loadExistingMetadata(metadataPath))
        {
            if (!job.resourceClass)
                job.resourceClass = metadata->resourceClassType;

            if (!job.assetFilePath && !metadata->importDependencies.empty())
                job.assetFilePath = metadata->importDependencies[0].importPath;

            if (!job.id && metadata->ids.empty())
                job.id = metadata->ids[0];
        }

        if (!job.resourceClass)
        {
            TRACE_ERROR("Import job for '{}' does not specify resource class", job.depotFilePath);
            return false;
        }

        if (!job.assetFilePath)
        {
            TRACE_ERROR("Import job for '{}' does not specify source asset path", job.depotFilePath);
            return false;
        }

        if (!job.id)
        {
            TRACE_ERROR("Import job for '{}' does not specify resource ID", job.depotFilePath);
            return false;
        }

        return true;
    }

    void prepareJobs(const Array<ImportJobInfo>& jobs, Array<ImportJobInfo>& outStack)
    {
        outStack.reserve(jobs.size());

        for (auto i : jobs.indexRange().reversed())
        {
            auto sourceJob = jobs[i];

            // make sure each file is only imported once
            const auto key = sourceJob.depotFilePath.toLower();
            if (m_scheduledDepotFiles.insert(key))
            {
                if (validateImportJob(sourceJob))
                    outStack.pushBack(sourceJob);
            }
        }
    }

    virtual bool saveInitialMetadata(const ImportJobInfo& job)
    {
        return true;
    }

    void createAdditionalJob(const ImportJobInfo& parentJob, const ImportJobFollowup& followup, Array<ImportJobInfo>& outStack)
    {
        DEBUG_CHECK_RETURN_EX(followup.depotFilePath, "Missing depot file");
        DEBUG_CHECK_RETURN_EX(followup.depotFilePath.endsWith(IResource::FILE_EXTENSION), "Depot file path is not valid");
        DEBUG_CHECK_RETURN_EX(followup.assetFilePath, "Missing asset file");
        DEBUG_CHECK_RETURN_EX(followup.resourceClass, "No resource class specified");
        DEBUG_CHECK_RETURN_EX(followup.id, "No resource ID specified");

        const auto key = followup.depotFilePath.toLower();
        if (m_scheduledDepotFiles.insert(key))
        {
            auto& job = outStack.emplaceBack();
            job.assetFilePath = followup.assetFilePath;
            job.depotFilePath = followup.depotFilePath;
            job.baseConfig = followup.externalConfig;
            job.resourceClass = followup.resourceClass;
            job.force = parentJob.force;
            job.recurse = parentJob.recurse;
            job.userConfig = nullptr; // keep existing

            if (!saveInitialMetadata(job))
            {
                TRACE_ERROR("Failed to save initial metadata for '{}', skipping file import", job.depotFilePath);
                outStack.popBack();
            }
        }
    }

    void process(const Array<ImportJobInfo>& jobs)
    {
        ScopeTimer timer;

        // make sure all data we got make sense
        InplaceArray<ImportJobInfo, 256> jobStack;
        prepareJobs(jobs, jobStack);

        // process the stack of jobs
        bool canceled = false;
        while (!jobStack.empty())
        {
            // canceled
            if (m_progress.checkCancelation())
            {
                canceled = true;
                break;
            }

            auto job = jobStack.back();
            jobStack.popBack();

            // update general progress
            {
                const auto coreFileName = job.assetFilePath.view().fileName();
                m_progress.reportProgress(m_numJobs, m_numJobs + jobStack.size(), TempString("Importing {} from '{}'", job.resourceClass, coreFileName));
            }

            // notify callbacks
            ScopeTimer timer;
            m_progress.jobStarted(job.depotFilePath);

            // import resource
            ImportJobResult result;
            {
                ImportQueueProgressTracker localProgressTracker(m_progress, job.depotFilePath);
                if (m_importer.importResource(job, result, &localProgressTracker))
                {
                    if (result.resource)
                    {
                        m_progress.jobFinished(job.depotFilePath, ImportStatus::FinishedNewContent, timer.timeElapsed());
                        m_numImported += 1;
                    }
                    else
                    {
                        m_progress.jobFinished(job.depotFilePath, ImportStatus::FinishedUpTodate, timer.timeElapsed());
                        m_numUpToDate += 1;
                    }
                }
                else
                {
                    m_progress.jobFinished(job.depotFilePath, ImportStatus::Failed, timer.timeElapsed());
                    m_numFailed += 1;
                }

                m_numJobs += 1;
                processResult(result);
            }

            // insert followup jobs
            if (job.recurse)
            {
                for (const auto& followup : result.followupImports)
                    createAdditionalJob(job, followup, jobStack);
            }
        }

        // if we were canceled then mark all non processed jobs as canceled
        if (canceled)
            for (const auto& job : jobStack)
                m_progress.jobFinished(job.depotFilePath, ImportStatus::Canceled, 0.0f);
    }

    virtual void processResult(const ImportJobResult& result) = 0;

private:
    IImportProgressTracker& m_progress;
    const IImportDepotLoader* m_loader = nullptr;

    ScopeTimer m_timer;

    Importer m_importer;

    uint32_t m_numJobs = 0;

    uint32_t m_numFailed = 0;
    uint32_t m_numImported = 0;
    uint32_t m_numUpToDate = 0;

    HashSet<StringBuf> m_scheduledDepotFiles;
};

//--

class ImportStackHelperCollector : public ImportStackHelper
{
public:
    ImportStackHelperCollector(const IImportDepotLoader* loader, IImportProgressTracker& progress, Array<ImportJobResult>& outResults)
        : ImportStackHelper(loader, progress)
        , m_outResults(outResults)
    {}

    void processResult(const ImportJobResult& result) override final
    {
        auto lock = CreateLock(m_lock);
        m_outResults.pushBack(result);
    }

private:
    SpinLock m_lock;
    Array<ImportJobResult>& m_outResults;
};

void ImportResources(IImportProgressTracker& progress, const Array<ImportJobInfo>& jobs, Array<ImportJobResult>& outResults, const IImportDepotLoader* loader)
{
    ImportStackHelperCollector stack(loader, progress, outResults);
    stack.process(jobs);
}

//--

class LocalDepotSaver : public IImportDepotSaver
{
public:
    virtual bool saveMetadata(StringView depotPath, const ResourceMetadata* metadata) override final
    {
        DEBUG_CHECK_RETURN_EX_V(metadata, "No data to save", false);
        DEBUG_CHECK_RETURN_EX_V(depotPath, "Invalid depot path", false);
        DEBUG_CHECK_RETURN_EX_V(depotPath.endsWith(ResourceMetadata::FILE_EXTENSION), "Invalid depot path", false);

        return GetService<DepotService>()->saveFileFromXMLObject(depotPath, metadata);
    }

    virtual bool saveResource(StringView depotPath, const IResource* data) override final
    {
        DEBUG_CHECK_RETURN_EX_V(data, "No data to save", false);
        DEBUG_CHECK_RETURN_EX_V(depotPath, "Invalid depot path", false);
        //DEBUG_CHECK_RETURN_EX_V(depotPath.endsWith(ResourceMetadata::FILE_EXTENSION), "Invalid depot path", false);

        return GetService<DepotService>()->saveFileFromResource(depotPath, data);
    }
};

IImportDepotSaver::~IImportDepotSaver()
{}

IImportDepotSaver& IImportDepotSaver::GetGlobalDepotSaver()
{
    static LocalDepotSaver theDepotSaver;
    return theDepotSaver;
}

//--

class ImportStackHelperSaver : public ImportStackHelper
{
public:
    ImportStackHelperSaver(const IImportDepotLoader* loader, IImportProgressTracker& progress, IImportDepotSaver* saver)
        : ImportStackHelper(loader, progress)
        , m_saver(saver ? saver : &IImportDepotSaver::GetGlobalDepotSaver())
    {}

    virtual bool saveInitialMetadata(const ImportJobInfo& job)
    {
        DEBUG_CHECK_RETURN_EX_V(job.id, "Empty resource ID", false);

        const auto metadataPath = ReplaceExtension(job.depotFilePath, ResourceMetadata::FILE_EXTENSION);
        auto metadata = m_loader->loadExistingMetadata(metadataPath);
        if (metadata)
        {
            // skip save if we already have that ID
            if (metadata->ids.contains(job.id))
                return true;
        }
        else
        {
            // create empty (seed) metadata
            metadata = RefNew<ResourceMetadata>();
        }

        // setup data
        metadata->resourceClassType = job.resourceClass;
        metadata->ids.pushBackUnique(job.id);

        // create source file dependency
        if (job.assetFilePath)
        {
            auto& sourceDep = metadata->importDependencies.emplaceBack();
            sourceDep.importPath = job.assetFilePath;
        }

        // setup configuration
        if (job.baseConfig)
        {
            metadata->importBaseConfiguration = CloneObject(job.baseConfig);
            if (metadata->importBaseConfiguration)
                metadata->importBaseConfiguration->parent(metadata);
        }

        // save metadata
        return m_saver->saveMetadata(metadataPath, metadata);
    }

    virtual void processResult(const ImportJobResult& result) override final
    {
        if (result.depotPath)
        {
            if (result.metadata)
            {
                const auto metadataPath = ReplaceExtension(result.depotPath, ResourceMetadata::FILE_EXTENSION);
                m_saver->saveMetadata(metadataPath, result.metadata);
            }

            if (result.resource)
                m_saver->saveResource(result.depotPath, result.resource);
        }
    }

private:
    const IImportDepotLoader* m_loader = nullptr;
    IImportDepotSaver* m_saver = nullptr;
};

void ImportResources(IImportProgressTracker& progress, const Array<ImportJobInfo>& jobs, IImportDepotSaver* saver, const IImportDepotLoader* loader)
{
    ImportStackHelperSaver stack(loader, progress, saver);
    stack.process(jobs);
}

//--

class SingleResourceLoader : public IImportDepotLoader
{
public:
    SingleResourceLoader(const IImportDepotLoader* loader, StringView depotPath, const IResource* existingResource, const ResourceMetadata* existingMetadata)
        : m_baseLoader(loader)
        , m_existingResource(AddRef(existingResource))
        , m_existingMetadata(AddRef(existingMetadata))
    {
        DEBUG_CHECK_RETURN_EX(ValidateDepotFilePath(depotPath), "Invalid depot path");
        m_existingResourcePath = ReplaceExtension(depotPath, IResource::FILE_EXTENSION);
        m_existingMetadataPath = ReplaceExtension(depotPath, ResourceMetadata::FILE_EXTENSION);
    }

    virtual ResourceID queryResourceID(StringView depotPath) const override
    {
        if (0 == m_existingResourcePath.compareWithNoCase(depotPath))
            return m_existingMetadata->ids.front();

        return m_baseLoader->queryResourceID(depotPath);
    }

    virtual ResourceMetadataPtr loadExistingMetadata(StringView depotPath) const override
    {
        if (0 == m_existingMetadataPath.compareWithNoCase(depotPath))
            return m_existingMetadata;

        return m_baseLoader->loadExistingMetadata(depotPath);
    }

    virtual ResourcePtr loadExistingResource(StringView depotPath) const override
    {
        if (0 == m_existingResourcePath.compareWithNoCase(depotPath))
            return m_existingResource;

        return m_baseLoader->loadExistingResource(depotPath);
    }

    virtual bool fileExists(StringView depotPath) const override
    {
        if (depotPath == m_existingMetadataPath || depotPath == m_existingResourcePath)
            return true;

        return m_baseLoader->fileExists(depotPath);
    }

    virtual bool findFile(StringView depotPath, StringView fileName, uint32_t maxDepth, StringBuf& outFoundFileDepotPath, ResourceID* outFoundResourceID) const override final
    {
        return m_baseLoader->findFile(depotPath, fileName, maxDepth, outFoundFileDepotPath, outFoundResourceID);
    }

private:
    const IImportDepotLoader* m_baseLoader = nullptr;

    StringBuf m_existingResourcePath;
    ResourcePtr m_existingResource;

    StringBuf m_existingMetadataPath;
    ResourceMetadataPtr m_existingMetadata;
};

class SingleResourceSaver : public IImportDepotSaver
{
public:
    SingleResourceSaver(StringView depotPath)
    {
        DEBUG_CHECK_RETURN_EX(ValidateDepotFilePath(depotPath), "Invalid depot path");
        m_existingResourcePath = ReplaceExtension(depotPath, IResource::FILE_EXTENSION);
        m_existingMetadataPath = ReplaceExtension(depotPath, ResourceMetadata::FILE_EXTENSION);
    }

    virtual bool saveMetadata(StringView depotPath, const ResourceMetadata* metadata) override
    {
        if (m_existingMetadataPath.compareWithNoCase(depotPath))
        {
            m_savedMetadata = AddRef(metadata);
            return true;
        }

        return false;
    }

    virtual bool saveResource(StringView depotPath, const IResource* data) override
    {
        if (m_existingResourcePath.compareWithNoCase(depotPath))
        {
            m_savedResource = AddRef(data);
            return true;
        }

        return false;
    }

    //--

    StringBuf m_existingResourcePath;
    StringBuf m_existingMetadataPath;

    ResourcePtr m_savedResource;
    ResourceMetadataPtr m_savedMetadata;
};

class SingleResourceProgress : public IImportProgressTracker
{
public:
    SingleResourceProgress(StringView depotPath, IProgressTracker* localProgress)
        : m_localProgress(localProgress)
        , m_depotPath(depotPath)
    {}

    virtual void jobProgressUpdate(StringView depotPath, uint64_t currentCount, uint64_t totalCount, StringView text) override final
    {
        if (m_localProgress && m_depotPath.compareWithNoCase(depotPath))
            m_localProgress->reportProgress(currentCount, totalCount, text);
    }

    virtual bool checkCancelation() const override
    {
        return m_localProgress ? m_localProgress->checkCancelation() : false;
    }

    virtual void reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text) override
    {
        // nothing
    }

private:
    IProgressTracker* m_localProgress = nullptr;
    StringBuf m_depotPath;
};

bool ReimportResource(StringView depotPath, const IResource* existingResource, const ResourceMetadata* existingMetadata, ResourcePtr& outResource, ResourceMetadataPtr& outMetadata, IProgressTracker* localProgress)
{
    DEBUG_CHECK_RETURN_EX_V(ValidateDepotFilePath(depotPath), "Invalid depot path", false);
    DEBUG_CHECK_RETURN_EX_V(existingResource, "Nothing to import", false);
    DEBUG_CHECK_RETURN_EX_V(existingMetadata, "Missing metadata", false);
    DEBUG_CHECK_RETURN_EX_V(!existingMetadata->importDependencies.empty(), "File not imported", false);
    DEBUG_CHECK_RETURN_EX_V(!existingMetadata->resourceClassType, "File not imported", false);

    const auto& baseLoader = IImportDepotLoader::GetGlobalDepotLoader();

    SingleResourceLoader singleLoader(&baseLoader, depotPath, existingResource, existingMetadata);
    SingleResourceSaver singleSaver(depotPath);
    SingleResourceProgress singleProgress(depotPath, localProgress);

    {
        InplaceArray<ImportJobInfo, 1> jobs;

        auto& job = jobs.emplaceBack();
        job.depotFilePath = StringBuf(depotPath);
        job.assetFilePath = existingMetadata->importDependencies[0].importPath;
        job.resourceClass = existingMetadata->resourceClassType;
        //job.userConfig = 
        job.force = true;
        job.recurse = false;

        ImportStackHelperSaver stack(&singleLoader, singleProgress, &singleSaver);
        stack.process(jobs);
    }

    if (!singleSaver.m_savedResource)
        return false;

    return true;
}

//--

END_BOOMER_NAMESPACE();