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

    class AssetImportListModel;

    typedef base::SpecificClassType<base::res::IResource> TImportClass;
    typedef base::Array<TImportClass> TImportClassRegistry;
    typedef base::HashMap<base::StringBuf, TImportClassRegistry> TImportClassRegistryMap;

    typedef base::SpecificClassType<base::res::ResourceConfiguration> TConfigClass;

    // internal import check status
    enum class AssetImportCheckStatus : uint8_t
    {
        Unknown,
        Checking,
        New,
        UpToDate,
        NewConfig,
        NewContent,
        MissingFiles,
    };

    //--

    // list model for displaying list of files to import
    class BASE_EDITOR_API AssetImportListModel : public ui::IAbstractItemModel
    {
    public:
        AssetImportListModel();
        virtual ~AssetImportListModel();

        //--

        // add new import file to the list
        ui::ModelIndex addNewImportFile(const base::StringBuf& assetPath, TImportClass importClass, const StringBuf& fileName, const ManagedDirectory* directory, const base::res::ResourceConfigurationPtr& specificUserConfiguration = nullptr);

        // add new reimport file
        ui::ModelIndex addReimportFile(ManagedFile* file);

        //--

        // remove all files
        void clearFiles();

        // remove all given files from list
        void removeFiles(const base::Array<ui::ModelIndex>& files);

        //--

        // get import configuration for given element
        base::res::ResourceConfigurationPtr fileConfiguration(const ui::ModelIndex& file) const;

        // get file depot path
        base::StringBuf fileDepotPath(const ui::ModelIndex& file) const;

        // get managed file 
        ManagedFile* fileManagedFile(const ui::ModelIndex& file) const;

        // get source asset
        base::StringBuf fileSourceAssetPath(const ui::ModelIndex& file) const;

        // get absolute path to source asset
        base::io::AbsolutePath fileSourceAssetAbsolutePath(const ui::ModelIndex& file) const;

        //--

        // generate the import list from currently selected resources
        base::res::ImportListPtr compileResourceList() const;

        //--

    private:
        struct FileData : public base::IReferencable
        {
            bool m_importFlag = true;

            const ManagedFile* m_existingFile = nullptr; // NULL for new files, not null for reimports

            base::StringBuf m_sourceAssetPath;

            const ManagedDirectory* m_targetDirectory = nullptr;
            base::StringBuf m_targetFileName;
            TImportClass m_targetClass;

            base::res::ResourceConfigurationPtr m_configuration; // user configuration
        };


        base::Array<base::RefPtr<FileData>> m_files;

        base::StringBuf fileDepotPath(const FileData* file) const;
        ManagedFile* fileManagedFile(const FileData* file) const;

        ui::ModelIndex indexForFile(const FileData* fileData) const;
        base::RefPtr<FileData> fileForIndex(const ui::ModelIndex& index) const;
        FileData* fileForIndexFast(const ui::ModelIndex& index) const;

        ui::ModelIndex addFileInternal(const base::RefPtr<FileData>& data);

        //--

        virtual uint32_t rowCount(const ui::ModelIndex& parent = ui::ModelIndex()) const override final;
        virtual bool hasChildren(const ui::ModelIndex& parent = ui::ModelIndex()) const override final;
        virtual bool hasIndex(int row, int col, const ui::ModelIndex& parent = ui::ModelIndex()) const  override final;
        virtual ui::ModelIndex parent(const ui::ModelIndex& item = ui::ModelIndex()) const  override final;
        virtual ui::ModelIndex index(int row, int column, const ui::ModelIndex& parent = ui::ModelIndex()) const override final;

        virtual void visualize(const ui::ModelIndex& item, int columnCount, ui::ElementPtr& content) const override final;
        virtual bool compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex = 0) const override final;
        virtual bool filter(const ui::ModelIndex& id, const ui::SearchPattern& filter, int colIndex = 0) const override final;

        virtual ui::PopupPtr contextMenu(ui::AbstractItemView* view, const base::Array<ui::ModelIndex>& indices) const override final;

        mutable base::HashMap<base::StringBuf, base::Array<base::SpecificClassType<base::res::IResource>>> m_classPerExtensionMap;

        const base::Array<base::SpecificClassType<base::res::IResource>>& getClassesForSourceExtension(StringView<char> sourcePath) const;
    };

    //--

    // the "perare for import" tab in the import window
    class BASE_EDITOR_API AssetImportPrepareTab : public ui::DockPanel
    {
        RTTI_DECLARE_VIRTUAL_CLASS(AssetImportPrepareTab, ui::DockPanel);

    public:
        AssetImportPrepareTab();
        virtual ~AssetImportPrepareTab();

        //--

        ui::ElementEventProxy OnStartImport;

        //--

        // add new files to list
        void addNewImportFiles(const ManagedDirectory* currentDirectory, TImportClass resourceClass, const base::Array<base::StringBuf>& selectedAssetPaths);

        // add request to reimprot files
        void addReimportFiles(const base::Array<ManagedFile*>& files);

    public:
        void cmdClearList();
        void cmdAddFiles();
        void cmdRemoveFiles();
        void cmdSaveList();
        void cmdLoadList();
        void cmdAppendList();
        void cmdStartImport();

        ui::ToolBarPtr m_toolbar;
        ui::ListViewPtr m_fileList;
        ui::NotebookPtr m_configTabs;
        ui::DataInspectorPtr m_configProperties;

        void updateSelection();

        void addFilesFromList(const base::res::ImportList& list);

        ManagedDirectory* contextDirectory();

        base::RefPtr<AssetImportListModel> m_filesListModel;
    };

    //--

    // the "Asset importer" window, non dockable
    class BASE_EDITOR_API AssetImportWindow : public ui::Window
    {
        RTTI_DECLARE_VIRTUAL_CLASS(AssetImportWindow, ui::Window);

    public:
        AssetImportWindow();
        virtual ~AssetImportWindow();

        void loadConfig(const ConfigGroup& config);
        void saveConfig(ConfigGroup& config) const;

        void addNewImportFiles(const ManagedDirectory* currentDirectory, TImportClass resourceClass, const base::Array<base::StringBuf>& selectedAssetPaths);
        void addReimportFiles(const base::Array<ManagedFile*>& files);

    private:
        base::RefPtr<AssetImportPrepareTab> m_prepareTab;
    };

    //--

} // ed