/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

#include "backgroundCommand.h"

namespace ed
{
    //--

    class AssetProcessingListModel;

    //--

    // asset importer process
    class BASE_EDITOR_API AssetImportCommand : public IBackgroundCommand
    {
        RTTI_DECLARE_VIRTUAL_CLASS(AssetImportCommand, IBackgroundCommand)

    public:
        AssetImportCommand(const res::ImportListPtr& fileList, AssetProcessingListModel* listModel);
        virtual ~AssetImportCommand();

        //--

        // request to cancel import of a single file
        void cancelSingleFile(const StringBuf& depotPath);

        //--

    private:
        const res::ImportListPtr m_fileList;

        //--

        RefPtr<AssetProcessingListModel> m_listModel;
        bool m_statusCheckSent = false;

        //--

        io::AbsolutePath m_tempListPath;
        void deleteTempFile();

        //--

        virtual bool configure(app::CommandLine& outCommandline) override final;
        virtual void update() override final;

        //--

        void handleImportQueueFileStatusChangeMessage(const res::ImportQueueFileStatusChangeMessage& msg);
    };

    //--

} // ed