/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"

#include "browserService.h"
#include "importTask.h"
#include "importDetails.h"

#include "core/io/include/io.h"
#include "core/xml/include/xmlUtils.h"
#include "core/app/include/commandline.h"
#include "core/net/include/messageConnection.h"
#include "core/resource/include/fileLoader.h"
#include "core/resource_compiler/include/importInterface.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

///--

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetImportTask);
RTTI_END_TYPE();

AssetImportTask::AssetImportTask(const Array<ImportJobInfo>& initialJobs)
    : IEditorTask("Import assets")
    , m_initialJobs(initialJobs)
{
    m_details = RefNew<AssetImportDetailsDialog>();
    m_details->clearFiles();
}

AssetImportTask::~AssetImportTask()
{
}

void AssetImportTask::runImportTask()
{
    ImportResources(*this, m_initialJobs);
    m_finished.exchange(true);
}

void AssetImportTask::cancelSingleFile(const StringBuf& depotPath)
{
    /*ImportQueueFileCancel msg;
    msg.depotPath = depotPath;
    connection()->send(msg);*/
}

//--

bool AssetImportTask::start()
{
    RefPtr<AssetImportTask> selfRef = AddRef(this);
    RunFiber("ImportAssets") << [selfRef](FIBER_FUNC) // TODO: RunBackgroundTask!
    {
        selfRef->runImportTask();
    };

    return true;
}

bool AssetImportTask::update()
{
    return m_finished;
}

ui::ElementPtr AssetImportTask::fetchDetailsDialog()
{
    return m_details;
}

//--

void AssetImportTask::jobAdded(const ImportJobInfo& info)
{
    m_details->setFileStatus_AnyThread(info.depotFilePath, ImportStatus::Pending);
}

void AssetImportTask::jobStarted(StringView depotPath)
{
    m_details->setFileStatus_AnyThread(StringBuf(depotPath), ImportStatus::Processing);
}

void AssetImportTask::jobFinished(StringView depotPath, ImportStatus status, double timeTaken)
{
    m_details->setFileStatus_AnyThread(StringBuf(depotPath), status, timeTaken);
}

void AssetImportTask::jobProgressUpdate(StringView depotPath, uint64_t currentCount, uint64_t totalCount, StringView text)
{
    m_details->setFileProgress_AnyThread(StringBuf(depotPath), currentCount, totalCount, StringBuf(text));
}

bool AssetImportTask::checkCancelation() const
{
    return IEditorTask::checkCancelation();
}

void AssetImportTask::reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text)
{
    IEditorTask::reportProgress(currentCount, totalCount, text);
}

//--

END_BOOMER_NAMESPACE_EX(ed)
