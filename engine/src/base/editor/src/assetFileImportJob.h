/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

#include "backgroundCommand.h"

#include "base/resource_compiler/include/importQueue.h"

namespace ed
{
    //--

    class AssetProcessingListModel;
    class AssetImportDetailsDialog;

    //--

    // asset importer process
    class BASE_EDITOR_API AssetImportJob : public IBackgroundJob, public res::IImportQueueCallbacks
    {
        RTTI_DECLARE_VIRTUAL_CLASS(AssetImportJob, IBackgroundJob)

    public:
        AssetImportJob(const res::ImportListPtr& fileList);
        virtual ~AssetImportJob();

        //--

        // request to cancel import of a single file
        void cancelSingleFile(const StringBuf& depotPath);

        //--

    private:
        const res::ImportListPtr m_fileList;

        //--

        RefPtr<AssetImportDetailsDialog> m_detailsDialog;
        RefPtr<AssetProcessingListModel> m_listModel;

        //--

        std::atomic<BackgroundJobStatus> m_status = BackgroundJobStatus::Running;

        void runImportTask();

        //--

        // IBackgroundJob
        virtual bool start() override final;
        virtual BackgroundJobStatus update() override final;
        virtual ui::ElementPtr fetchDetailsDialog() override final;
        virtual ui::ElementPtr fetchStatusDialog() override final;

        // IImportQueueCallbacks
        virtual void queueJobAdded(const res::ImportJobInfo& info) override final;
        virtual void queueJobStarted(StringView depotPath) override final;
        virtual void queueJobFinished(StringView depotPath, res::ImportStatus status, double timeTaken) override final;
        virtual void queueJobProgressUpdate(StringView depotPath, uint64_t currentCount, uint64_t totalCount, StringView text) override final;
    };

    //--

} // ed