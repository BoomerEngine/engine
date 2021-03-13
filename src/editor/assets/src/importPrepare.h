/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

#include "engine/ui/include/uiSimpleTreeModel.h"
#include "engine/ui/include/uiElement.h"
#include "engine/ui/include/uiWindow.h"
#include "engine/ui/include/uiListViewEx.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

#if 0
// list model for displaying list of files to import
class EDITOR_ASSETS_API AssetImportListEntry : public ui::IListItem
{
public:
    AssetImportListEntry();
    virtual ~AssetImportListEntry();

    //--

    bool m_enabled = true;

    StringBuf m_sourceAssetPath;
    StringBuf m_targetDirectoryDepotPath;
    StringBuf m_targetFileName;

    TImportClass m_targetClass;

    ResourceConfigurationPtr m_configuration; // user configuration

    //--

    // add new import file to the list
    static RefPtr<AssetImportListEntry> CreateNewFileEntry(StringView sourceAssetFile, StringView depotDirectoryPath, TImportClass importClass, StringView fileName = "", const ResourceConfigurationPtr& specificUserConfiguration = nullptr, bool enabled = true);

    // create entry for file reimport
    ui::ModelIndex addReimportFile(StringView depotPath, const ResourceConfigurationPtr& specificUserConfiguration = nullptr);

    //--

    // remove all files
    void clearFiles();

    // remove all given files from list
    void removeFiles(const Array<ui::ModelIndex>& files);

    // remove specific file
    void removeFile(ManagedFileNativeResource* file);

    //--

    // get import configuration for given element
    ResourceConfigurationPtr fileConfiguration(const ui::ModelIndex& file) const;

    // get file depot path
    StringBuf fileDepotPath(const ui::ModelIndex& file) const;

    // get managed file 
    ManagedFileNativeResource* fileManagedFile(const ui::ModelIndex& file) const;

    // get source asset
    StringBuf fileSourceAssetPath(const ui::ModelIndex& file) const;

    // get absolute path to source asset
    StringBuf fileSourceAssetAbsolutePath(const ui::ModelIndex& file) const;

    //--

    // generate the import list from currently selected resources
    ImportListPtr compileResourceList(bool activeOnly=false) const;

    //--

private:
    struct FileData : public IReferencable
    {
        ui::ModelIndex index;

        bool importFlag = true;

        const ManagedFileNativeResource* existingFile = nullptr; // NULL for new files, not null for reimports

        StringBuf sourceAssetPath;

        const ManagedDirectory* targetDirectory = nullptr;
        StringBuf targetFileName;

        TImportClass targetClass;

        ResourceConfigurationPtr configuration; // user configuration
    };

    Array<RefPtr<FileData>> m_files;

    StringBuf fileDepotPath(const FileData* file) const;
    ManagedFileNativeResource* fileManagedFile(const FileData* file) const;

    //--

    virtual bool hasChildren(const ui::ModelIndex& parent = ui::ModelIndex()) const override final;
    virtual ui::ModelIndex parent(const ui::ModelIndex& item = ui::ModelIndex()) const override final;
    virtual void children(const ui::ModelIndex& parent, Array<ui::ModelIndex>& outChildrenIndices) const override final;
    virtual void visualize(const ui::ModelIndex& item, int columnCount, ui::ElementPtr& content) const override final;
    virtual bool compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex = 0) const override final;
    virtual bool filter(const ui::ModelIndex& id, const ui::SearchPattern& filter, int colIndex = 0) const override final;

    virtual ui::PopupPtr contextMenu(ui::AbstractItemView* view, const Array<ui::ModelIndex>& indices) const override final;

    mutable HashMap<StringBuf, Array<SpecificClassType<IResource>>> m_classPerExtensionMap;

    const Array<SpecificClassType<IResource>>& getClassesForSourceExtension(StringView sourcePath) const;
};
#endif

//--

/// an element in the import list
class EDITOR_ASSETS_API AssetImportListElement : public ui::IListItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetImportListElement, ui::IListItem);

public:
    struct Setup
    {
        bool enabled = true;
        bool newFile = false;
        StringBuf fileName;
        StringBuf sourcePath;
        StringBuf depotDirectoryPath;
        ResourceClass cls;
        ResourceID id;
    };

    AssetImportListElement(const Setup& setup);

    bool enabled() const;

    StringBuf compileDepotPath() const;
    void compileJob(ImportJobInfo& outJob) const;
    
    INLINE const ResourceConfigurationPtr& userConfig() const { return m_userConfig; }

private:
    ui::CheckBoxPtr m_boxEnabled;
    ui::EditBoxPtr m_boxName;

    StringBuf m_sourcePath;
    StringBuf m_depotDirectoryPath;
    ResourceClass m_class;
    StringBuf m_originalFileName;

    ResourceConfigurationPtr m_baseConfig;
    ResourceConfigurationPtr m_userConfig;

    ResourceID m_id;

    void buildConfig();

    virtual bool handleItemFilter(const ui::ICollectionView* view, const ui::SearchPattern& filter) const;
    virtual void handleItemSort(const ui::ICollectionView* view, int colIndex, SortingData& outInfo) const;
    virtual void handleItemActivate(const ui::ICollectionView* view);
};

//--

// tab with the list of files for import operation
class EDITOR_ASSETS_API AssetImportPrepareDialog : public ui::Window
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetImportPrepareDialog, ui::Window);

public:
    AssetImportPrepareDialog();
    virtual ~AssetImportPrepareDialog();

    //--

    bool importFile(ResourceClass resourceClass, StringView depotPath, StringView sourcePath);

    bool reimportFile(StringView depotPath);

    //--

public:
    void cmdClearList();
    void cmdSaveList();
    void cmdLoadList();
    void cmdAppendList();
    void cmdStartImport(bool force);

    ui::ToolBarPtr m_toolbar;
    ui::ListViewExPtr m_fileList;
    ui::NotebookPtr m_configTabs;
    ui::DataInspectorPtr m_configProperties;

    void updateSelection();
    void addFilesFromList(const ImportList& list);

    void compileImportJobs(Array<ImportJobInfo>& outJobs) const;
};

//--

END_BOOMER_NAMESPACE_EX(ed)
