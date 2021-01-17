/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"

#include "editorService.h"
#include "assetFileImportJob.h"
#include "assetFileImportDetailsDialog.h"

#include "base/io/include/ioSystem.h"
#include "base/xml/include/xmlUtils.h"
#include "base/app/include/commandline.h"
#include "base/net/include/messageConnection.h"
#include "base/resource/include/resourceFileLoader.h"
#include "base/resource/include/resourceLoadingService.h"
#include "base/resource_compiler/include/importCommandHelper.h"
#include "base/resource_compiler/include/importInterface.h"
#include "base/resource_compiler/include/importFileList.h"

namespace ed
{

    ///--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetImportJob);
    RTTI_END_TYPE();

    AssetImportJob::AssetImportJob(const res::ImportListPtr& fileList)
        : IBackgroundJob("Import assets")
        , m_fileList(fileList)
    {
        m_listModel = RefNew<AssetProcessingListModel>();

        for (const auto& file : fileList->files())
            m_listModel->setFileStatus(file.depotPath, res::ImportStatus::Pending);

        m_detailsDialog = RefNew<AssetImportDetailsDialog>(m_listModel);
    }

    AssetImportJob::~AssetImportJob()
    {
    }

    void AssetImportJob::runImportTask()
    {
        if (res::ProcessImport(m_fileList, this, this))
            m_status.exchange(BackgroundJobStatus::Finished);
        else
            m_status.exchange(BackgroundJobStatus::Failed);
    }

    void AssetImportJob::cancelSingleFile(const StringBuf& depotPath)
    {
        /*res::ImportQueueFileCancel msg;
        msg.depotPath = depotPath;
        connection()->send(msg);*/
    }

    //--

    bool AssetImportJob::start()
    {
        RefPtr<AssetImportJob> selfRef = AddRef(this);
        RunFiber("ImportAssets") << [selfRef](FIBER_FUNC) // TODO: RunBackgroundTask!
        {
            selfRef->runImportTask();
        };

        return true;
    }

    BackgroundJobStatus AssetImportJob::update()
    {
        return m_status.load();
    }

    ui::ElementPtr AssetImportJob::fetchDetailsDialog()
    {
        return m_detailsDialog;
    }

    ui::ElementPtr AssetImportJob::fetchStatusDialog()
    {
        return nullptr;
    }

    //--

    void AssetImportJob::queueJobAdded(const res::ImportJobInfo& info)
    {
        auto model = m_listModel;

        RunSync() << [model, info](FIBER_FUNC)
        {
            model->setFileStatus(info.depotFilePath, res::ImportStatus::Pending);
        };
    }

    void AssetImportJob::queueJobStarted(StringView depotPath)
    {
        auto model = m_listModel;
        auto path = StringBuf(depotPath);

        RunSync() << [model, path](FIBER_FUNC)
        {
            model->setFileStatus(path, res::ImportStatus::Processing);
        };
    }

    void AssetImportJob::queueJobFinished(StringView depotPath, res::ImportStatus status, double timeTaken)
    {
        auto model = m_listModel;
        auto path = StringBuf(depotPath);

        RunSync() << [model, path, status, timeTaken](FIBER_FUNC)
        {
            model->setFileStatus(path, status, timeTaken);
        };
    }

    void AssetImportJob::queueJobProgressUpdate(StringView depotPath, uint64_t currentCount, uint64_t totalCount, StringView text)
    {
        auto model = m_listModel;
        auto pathCopy = StringBuf(depotPath);
        auto textCopy = StringBuf(text);

        RunSync() << [model, pathCopy, currentCount, totalCount, textCopy](FIBER_FUNC)
        {
            model->setFileProgress(pathCopy, currentCount, totalCount, textCopy);
        };
    }

    //--

} // ed