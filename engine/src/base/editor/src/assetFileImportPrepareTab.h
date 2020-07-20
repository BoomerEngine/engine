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

    typedef SpecificClassType<res::IResource> TImportClass;
    typedef Array<TImportClass> TImportClassRegistry;
    typedef HashMap<StringBuf, TImportClassRegistry> TImportClassRegistryMap;

    typedef SpecificClassType<res::ResourceConfiguration> TConfigClass;

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

        // do we have files
        bool hasFiles() const;

        // do we have importable files ?
        bool hasImportableFiles() const;

        //--

        // add new import file to the list
        ui::ModelIndex addNewImportFile(const StringBuf& assetPath, TImportClass importClass, const StringBuf& fileName, const ManagedDirectory* directory, const res::ResourceConfigurationPtr& specificUserConfiguration = nullptr);

        // add new reimport file
        ui::ModelIndex addReimportFile(ManagedFileNativeResource* file, const res::ResourceConfigurationPtr& specificUserConfiguration = nullptr);

        //--

        // remove all files
        void clearFiles();

        // remove all given files from list
        void removeFiles(const Array<ui::ModelIndex>& files);

        // remove specific file
        void removeFile(ManagedFileNativeResource* file);

        //--

        // get import configuration for given element
        res::ResourceConfigurationPtr fileConfiguration(const ui::ModelIndex& file) const;

        // get file depot path
        StringBuf fileDepotPath(const ui::ModelIndex& file) const;

        // get managed file 
        ManagedFileNativeResource* fileManagedFile(const ui::ModelIndex& file) const;

        // get source asset
        StringBuf fileSourceAssetPath(const ui::ModelIndex& file) const;

        // get absolute path to source asset
        io::AbsolutePath fileSourceAssetAbsolutePath(const ui::ModelIndex& file) const;

        //--

        // generate the import list from currently selected resources
        res::ImportListPtr compileResourceList(bool activeOnly=false) const;

        //--

    private:
        struct FileData : public IReferencable
        {
            bool m_importFlag = true;

            const ManagedFileNativeResource* m_existingFile = nullptr; // NULL for new files, not null for reimports

            StringBuf m_sourceAssetPath;

            const ManagedDirectory* m_targetDirectory = nullptr;
            StringBuf m_targetFileName;

            TImportClass m_targetClass;

            res::ResourceConfigurationPtr m_configuration; // user configuration
        };

        Array<RefPtr<FileData>> m_files;

        StringBuf fileDepotPath(const FileData* file) const;
        ManagedFileNativeResource* fileManagedFile(const FileData* file) const;

        ui::ModelIndex indexForFile(const FileData* fileData) const;
        RefPtr<FileData> fileForIndex(const ui::ModelIndex& index) const;
        FileData* fileForIndexFast(const ui::ModelIndex& index) const;

        ui::ModelIndex addFileInternal(const RefPtr<FileData>& data);

        //--

        virtual uint32_t rowCount(const ui::ModelIndex& parent = ui::ModelIndex()) const override final;
        virtual bool hasChildren(const ui::ModelIndex& parent = ui::ModelIndex()) const override final;
        virtual bool hasIndex(int row, int col, const ui::ModelIndex& parent = ui::ModelIndex()) const  override final;
        virtual ui::ModelIndex parent(const ui::ModelIndex& item = ui::ModelIndex()) const  override final;
        virtual ui::ModelIndex index(int row, int column, const ui::ModelIndex& parent = ui::ModelIndex()) const override final;

        virtual void visualize(const ui::ModelIndex& item, int columnCount, ui::ElementPtr& content) const override final;
        virtual bool compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex = 0) const override final;
        virtual bool filter(const ui::ModelIndex& id, const ui::SearchPattern& filter, int colIndex = 0) const override final;

        virtual ui::PopupPtr contextMenu(ui::AbstractItemView* view, const Array<ui::ModelIndex>& indices) const override final;

        mutable HashMap<StringBuf, Array<SpecificClassType<res::IResource>>> m_classPerExtensionMap;

        const Array<SpecificClassType<res::IResource>>& getClassesForSourceExtension(StringView<char> sourcePath) const;
    };

    //--

    // tab with the list of files for import operation
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
        void addNewImportFiles(const ManagedDirectory* currentDirectory, TImportClass resourceClass, const Array<StringBuf>& selectedAssetPaths);

        // add request to reimport files
        void addReimportFiles(const Array<ManagedFileNativeResource*>& files);

        // add request to reimport single file with new configuration
        void addReimportFile(ManagedFileNativeResource* files, const res::ResourceConfigurationPtr& reimportConfiguration);

        //--

        // compile files list for importing
        res::ImportListPtr compileResourceList() const;

        //--

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

        void addFilesFromList(const res::ImportList& list);

        ManagedDirectory* contextDirectory();

        RefPtr<AssetImportListModel> m_filesListModel;
    };

    //--

} // ed