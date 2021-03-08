/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: commands #]
***/

#include "build.h"

#include "commandImport.h"

#include "importer.h"

#include "core/app/include/command.h"
#include "core/app/include/commandline.h"
#include "core/resource/include/loader.h"
#include "core/resource/include/loader.h"
#include "core/resource/include/fileLoader.h"
#include "core/resource/include/fileSaver.h"
#include "core/object/include/object.h"
#include "core/resource/include/metadata.h"
#include "core/xml/include/xmlUtils.h"
#include "core/net/include/messageConnection.h"
#include "core/resource/include/depot.h"
#include "core/containers/include/path.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_CLASS(CommandImport);
    RTTI_METADATA(app::CommandNameMetadata).name("import");
RTTI_END_TYPE();

//--

class ImportQueueProgressReporter : public IImportProgressTracker
{
public:
    ImportQueueProgressReporter(IProgressTracker* progress)
        : m_progress(progress)
    {}

    virtual void jobAdded(const ImportJobInfo& info) override
    {
    }

    virtual void jobStarted(StringView depotPath) override
    {
    }

    virtual void jobFinished(StringView depotPath, ImportStatus status, double timeTaken) override
    {
    }

    virtual void jobProgressUpdate(StringView depotPath, uint64_t currentCount, uint64_t totalCount, StringView text) override
    {
    }

    virtual bool checkCancelation() const override
    {
        return m_progress ? m_progress->checkCancelation() : false;
    }

    virtual void reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text) override
    {
        if (m_progress)
            m_progress->reportProgress(currentCount, totalCount, text);
    }

private:
    IProgressTracker* m_progress = nullptr;
};

//--

static bool CollectImportJobs(const app::CommandLine& commandline, Array<ImportJobInfo>& outJobs)
{
    bool force = commandline.hasParam("force");

    const auto depotPath = commandline.singleValue("depotPath");
    const auto assetPath = commandline.singleValue("assetPath");
    if (depotPath && assetPath)
    {
        auto& info = outJobs.emplaceBack();
        info.assetFilePath = assetPath;
        info.depotFilePath = ReplaceExtension(depotPath, IResource::FILE_EXTENSION);

        const auto resourceClassText = commandline.singleValue("class");
        if (!resourceClassText)
        {
            TRACE_ERROR("Resource class should be specified via '-class=' param");
            return false;
        }

        info.resourceClass = RTTI::GetInstance().findClass(StringID(resourceClassText)).cast<IResource>();
        if (!info.resourceClass)
        {
            TRACE_ERROR("Unknown resource class '{}'", resourceClassText);
            return false;
        }

        info.force = force;

        if (commandline.hasParam("autoId"))
            info.id = ResourceID::Create();
    }

    /*const auto importListPath = commandline.singleValue("assetListPath");
    if (!importListPath.empty())
    {
        if (const auto assetListDoc = xml::LoadDocument(xml::ILoadingReporter::GetDefault(), importListPath))
        {
            if (const auto assetList = LoadObjectFromXML<ImportList>(assetListDoc))
            {
                TRACE_INFO("Found {} files at import list '{}'", assetList->files().size(), importListPath);
                for (const auto& file : assetList->files())
                {
                    ImportJobInfo info;
                    info.assetFilePath = file.assetPath;
                    info.depotFilePath = ReplaceExtension(depotPath, ResourceMetadata::FILE_EXTENSION);
                    info.resourceClass = file.resourceClass;
                    info.id = file.id;
                    info.userConfig = CloneObject(file.userConfiguration);
                    info.force = force;
                    outQueue.scheduleJob(info);
                    hasWork = true;
                }
            }
            else
            {
                TRACE_ERROR("Failed to parse import list from XML '{}'", importListPath);
            }
        }
        else
        {
            TRACE_ERROR("Failed to load import list from XML '{}'", importListPath);
        }
    }*/

    return true;
}

//--

CommandImport::CommandImport()
{
}

CommandImport::~CommandImport()
{
}

//--

bool CommandImport::run(IProgressTracker* progress, const app::CommandLine& commandline)
{
    // get the jobs
    Array<ImportJobInfo> jobs;
    if (!CollectImportJobs(commandline, jobs))
        return false;

    // process the jobs
    ImportQueueProgressReporter queueProgress(progress);
    ImportResources(queueProgress, jobs);

    // we are done
    return true;
}

//--

END_BOOMER_NAMESPACE()
