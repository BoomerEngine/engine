/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

#include "editorBackgroundTask.h"

#include "core/resource_compiler/include/importQueue.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

class AssetProcessingListModel;
class AssetImportDetailsDialog;

//--

// asset importer process
class EDITOR_COMMON_API AssetImportJob : public IBackgroundTask, public IImportQueueCallbacks
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetImportJob, IBackgroundTask)

public:
    AssetImportJob(const ImportListPtr& fileList, bool force);
    virtual ~AssetImportJob();

    //--

    // request to cancel import of a single file
    void cancelSingleFile(const StringBuf& depotPath);

    //--

private:
    const ImportListPtr m_fileList;

    //--

    bool m_force = false;

    RefPtr<AssetImportDetailsDialog> m_detailsDialog;
    RefPtr<AssetProcessingListModel> m_listModel;

    //--

    std::atomic<BackgroundTaskStatus> m_status = BackgroundTaskStatus::Running;

    void runImportTask();

    //--

    // IBackgroundJob
    virtual bool start() override final;
    virtual BackgroundTaskStatus update() override final;
    virtual ui::ElementPtr fetchDetailsDialog() override final;
    virtual ui::ElementPtr fetchStatusDialog() override final;

    // IImportQueueCallbacks
    virtual void queueJobAdded(const ImportJobInfo& info) override final;
    virtual void queueJobStarted(StringView depotPath) override final;
    virtual void queueJobFinished(StringView depotPath, ImportStatus status, double timeTaken) override final;
    virtual void queueJobProgressUpdate(StringView depotPath, uint64_t currentCount, uint64_t totalCount, StringView text) override final;
};

//--

END_BOOMER_NAMESPACE_EX(ed)
