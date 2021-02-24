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
#include "base/ui/include/uiWindow.h"

BEGIN_BOOMER_NAMESPACE(ed)

//--

class AssetImportListModel;

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
class EDITOR_COMMON_API AssetImportListModel : public ui::IAbstractItemModel
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
    StringBuf fileSourceAssetAbsolutePath(const ui::ModelIndex& file) const;

    //--

    // generate the import list from currently selected resources
    res::ImportListPtr compileResourceList(bool activeOnly=false) const;

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

        res::ResourceConfigurationPtr configuration; // user configuration
    };

    Array<RefPtr<FileData>> m_files;

    StringBuf fileDepotPath(const FileData* file) const;
    ManagedFileNativeResource* fileManagedFile(const FileData* file) const;

    //--

    virtual bool hasChildren(const ui::ModelIndex& parent = ui::ModelIndex()) const override final;
    virtual ui::ModelIndex parent(const ui::ModelIndex& item = ui::ModelIndex()) const override final;
    virtual void children(const ui::ModelIndex& parent, base::Array<ui::ModelIndex>& outChildrenIndices) const override final;
    virtual void visualize(const ui::ModelIndex& item, int columnCount, ui::ElementPtr& content) const override final;
    virtual bool compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex = 0) const override final;
    virtual bool filter(const ui::ModelIndex& id, const ui::SearchPattern& filter, int colIndex = 0) const override final;

    virtual ui::PopupPtr contextMenu(ui::AbstractItemView* view, const Array<ui::ModelIndex>& indices) const override final;

    mutable HashMap<StringBuf, Array<SpecificClassType<res::IResource>>> m_classPerExtensionMap;

    const Array<SpecificClassType<res::IResource>>& getClassesForSourceExtension(StringView sourcePath) const;
};

//--

// tab with the list of files for import operation
class EDITOR_COMMON_API AssetImportPrepareDialog : public ui::Window
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetImportPrepareDialog, ui::Window);

public:
    AssetImportPrepareDialog();
    virtual ~AssetImportPrepareDialog();

    //--

    // add new files to list
    void addNewImportFiles(const ManagedDirectory* currentDirectory, TImportClass resourceClass, const Array<StringBuf>& selectedAssetPaths);

    // add request to reimport files
    void addReimportFiles(const Array<ManagedFileNativeResource*>& files);

    // add request to reimport single file with new configuration
    void addReimportFile(ManagedFileNativeResource* files, const res::ResourceConfigurationPtr& reimportConfiguration);

    //--

public:
    void cmdClearList();
    void cmdAddFiles();
    void cmdRemoveFiles();
    void cmdSaveList();
    void cmdLoadList();
    void cmdAppendList();
    void cmdStartImport(bool force);

    ui::ToolBarPtr m_toolbar;
    ui::ListViewPtr m_fileList;
    ui::NotebookPtr m_configTabs;
    ui::DataInspectorPtr m_configProperties;

    void updateSelection();

    void addFilesFromList(const res::ImportList& list);

    ManagedDirectory* contextDirectory();

    res::ImportListPtr compileResourceList() const;

    RefPtr<AssetImportListModel> m_filesListModel;
};

//--

END_BOOMER_NAMESPACE(ed)