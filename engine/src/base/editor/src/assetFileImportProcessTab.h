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

    DECLARE_UI_EVENT(EVENT_ASSET_IMPORT_FINISHED);

    //--

    class AssetImportCommand;
    class AssetProcessingListModel;

    //--

    // list model for displaying files being processed by the importer
    class BASE_EDITOR_API AssetProcessingListModel : public ui::IAbstractItemModel
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
        void setFileProgress(const StringBuf& depotFileName, uint64_t count, uint64_t total, StringView<char> message);

        //--

        // get depot path for given item
        const base::StringBuf& fileDepotPath(ui::ModelIndex index) const;

        //--

    private:
        struct FileData : public IReferencable
        {
            StringBuf m_depotPath;

            res::ImportStatus m_status = res::ImportStatus::Pending;
            float m_time = 0.0f; // time it took to check/process the resource

            StringBuf m_progressLastStatus;
            uint64_t m_progressLastCount = 0;
            uint64_t m_progressLastTotal = 0;
        };

        Array<RefPtr<FileData>> m_files;
        HashMap<StringBuf, FileData*> m_fileMap;

        ui::ModelIndex indexForFile(const FileData* fileData) const;
        RefPtr<FileData> fileForIndex(const ui::ModelIndex& index) const;
        FileData* fileForIndexFast(const ui::ModelIndex& index) const;

        //--

        virtual uint32_t rowCount(const ui::ModelIndex& parent = ui::ModelIndex()) const override final;
        virtual bool hasChildren(const ui::ModelIndex& parent = ui::ModelIndex()) const override final;
        virtual bool hasIndex(int row, int col, const ui::ModelIndex& parent = ui::ModelIndex()) const  override final;
        virtual ui::ModelIndex parent(const ui::ModelIndex& item = ui::ModelIndex()) const  override final;
        virtual ui::ModelIndex index(int row, int column, const ui::ModelIndex& parent = ui::ModelIndex()) const override final;

        virtual void visualize(const ui::ModelIndex& item, int columnCount, ui::ElementPtr& content) const override final;
        virtual bool compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex = 0) const override final;
        virtual bool filter(const ui::ModelIndex& id, const ui::SearchPattern& filter, int colIndex = 0) const override final;
    };

    //--

    // the "for import" tab in the import window
    class BASE_EDITOR_API AssetImportMainTab : public ui::DockPanel
    {
        RTTI_DECLARE_VIRTUAL_CLASS(AssetImportMainTab, ui::DockPanel);

    public:
        AssetImportMainTab();
        virtual ~AssetImportMainTab();

        //--

        // are we currently importing anything ?
        bool hasImportInProgress() const;

        // start importing given list of assets
        bool startAssetImport(res::ImportListPtr files);

        // stop/cancel any asset import in progress
        void cancelAssetImport();

    public:
        void cmdCancel();

        ui::ToolBarPtr m_toolbar;
        ui::ListViewPtr m_fileList;

        bool m_importCancelRequested = false;
        bool m_importKillRequested = false;

        void updateSelection();

        //--

        BackgroundJobPtr m_backgroundJob;
        RefPtr<AssetImportCommand> m_backgroundCommand;

        ui::Timer m_updateTimer;

        void updateState();
        void showFilesContextMenu();
        void showSelectedFilesInBrowser();

        //--

        RefPtr<AssetProcessingListModel> m_filesListModel;
    };

    //--

} // ed