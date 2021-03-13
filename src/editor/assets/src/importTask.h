/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

#include "core/resource_compiler/include/importer.h"
#include "editor/common/include/task.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

class AssetProcessingListModel;
class AssetImportDetailsDialog;

//--

// asset importer process
class EDITOR_ASSETS_API AssetImportTask : public IEditorTask, public IImportProgressTracker
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetImportTask, IEditorTask)

public:
    AssetImportTask(const Array<ImportJobInfo>& initialJobs);
    virtual ~AssetImportTask();

    //--

    // request to cancel import of a single file
    void cancelSingleFile(const StringBuf& depotPath);

    //--

private:
    Array<ImportJobInfo> m_initialJobs;

    //--

    RefPtr<AssetImportDetailsDialog> m_details;

    //--

    std::atomic<bool> m_finished = false;

    void runImportTask();

    //--

    // IBackgroundJob
    virtual bool start() override final;
    virtual bool update() override final;
    virtual ui::ElementPtr fetchDetailsDialog() override final;
    
    // IImportQueueCallbacks
    virtual void jobAdded(const ImportJobInfo& info) override final;
    virtual void jobStarted(StringView depotPath) override final;
    virtual void jobFinished(StringView depotPath, ImportStatus status, double timeTaken) override final;
    virtual void jobProgressUpdate(StringView depotPath, uint64_t currentCount, uint64_t totalCount, StringView text) override final;

    // IProgressTracker
    virtual bool checkCancelation() const override final;
    virtual void reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text) override final;
};

//--

END_BOOMER_NAMESPACE_EX(ed)
