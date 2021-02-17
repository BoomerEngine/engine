/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

#include "managedDepot.h"
#include "base/ui/include/uiSimpleTreeModel.h"
#include "base/ui/include/uiElement.h"

namespace ed
{

    //--

    // list model for displaying files being processed by the importer
    class EDITOR_COMMON_API AssetProcessingListModel : public ui::IAbstractItemModel
    {
    public:
        AssetProcessingListModel();
        virtual ~AssetProcessingListModel();

        //--

        // clear the list
        void clear();

        // set file status, will add file if not there
        void setFileStatus(const StringBuf& depotFileName, res::ImportStatus status, float time = 0.0);

        // set file progress information
        void setFileProgress(const StringBuf& depotFileName, uint64_t count, uint64_t total, StringView message);

        //--

        // get depot path for given item
        const base::StringBuf& fileDepotPath(ui::ModelIndex index) const;

        //--

    private:
        struct FileData : public IReferencable
        {
            ui::ModelIndex index;

            StringBuf depotPath;

            res::ImportStatus status = res::ImportStatus::Pending;
            float time = 0.0f; // time it took to check/process the resource

            StringBuf progressLastStatus;
            uint64_t progressLastCount = 0;
            uint64_t progressLastTotal = 0;
        };

        Array<RefPtr<FileData>> m_files;
        HashMap<StringBuf, FileData*> m_fileMap;

        //--

        virtual ui::ModelIndex parent(const ui::ModelIndex& item = ui::ModelIndex()) const  override final;
        virtual bool hasChildren(const ui::ModelIndex& parent = ui::ModelIndex()) const override final;
        virtual void children(const ui::ModelIndex& parent, base::Array<ui::ModelIndex>& outChildrenIndices) const override final;
        virtual void visualize(const ui::ModelIndex& item, int columnCount, ui::ElementPtr& content) const override final;
        virtual bool compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex = 0) const override final;
        virtual bool filter(const ui::ModelIndex& id, const ui::SearchPattern& filter, int colIndex = 0) const override final;
    };

    //--

    // dialog for the import job
    class EDITOR_COMMON_API AssetImportDetailsDialog : public ui::IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(AssetImportDetailsDialog, ui::IElement);

    public:
        AssetImportDetailsDialog(AssetProcessingListModel* listModel);
        virtual ~AssetImportDetailsDialog();

        //--

    public:
        ui::ListViewPtr m_fileList;
        RefPtr<AssetProcessingListModel> m_filesListModel;

        void updateSelection();

        //--

        ui::Timer m_updateTimer;

        void updateState();
        void showFilesContextMenu();
        void showSelectedFilesInBrowser();

        //--
    };

    //--

} // ed