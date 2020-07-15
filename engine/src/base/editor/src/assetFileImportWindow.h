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

    // file entry in the import list
    class BASE_EDITOR_API AssetImportEntry : public base::IReferencable
    {
    public:
        AssetImportEntry();

        bool importFlag = true;

        base::StringBuf assetPath;

        const ManagedDirectory* targetDirectory = nullptr;

        base::StringBuf targetFileName;
        base::SpecificClassType<base::res::IResource> targetClass;

        base::Array<base::res::ResourceConfigurationPtr> configurations;

        void changeResourceType(base::SpecificClassType<base::res::IResource> targetClass); // will create missing configurations
    };

    typedef base::RefPtr<AssetImportEntry> AssetImportEntryPtr;

    //--

    // list model for displaying list of files to import
    class BASE_EDITOR_API AssetImportListModel : public ui::IAbstractItemModel
    {
    public:
        AssetImportListModel();
        virtual ~AssetImportListModel();

        //--

        // get list of files
        INLINE const Array<AssetImportEntryPtr>& files() const { return m_files; }

        //--

        // remove all files
        void clear();

        // add entry to list
        void addFile(AssetImportEntry* file);

        // remove entry form list
        void removeFile(AssetImportEntry* file);

        // get entry
        AssetImportEntry* findFile(const ui::ModelIndex& index) const;

        //--

    private:
        Array<AssetImportEntryPtr> m_files;

        virtual uint32_t rowCount(const ui::ModelIndex& parent = ui::ModelIndex()) const override final;
        virtual bool hasChildren(const ui::ModelIndex& parent = ui::ModelIndex()) const override final;
        virtual bool hasIndex(int row, int col, const ui::ModelIndex& parent = ui::ModelIndex()) const  override final;
        virtual ui::ModelIndex parent(const ui::ModelIndex& item = ui::ModelIndex()) const  override final;
        virtual ui::ModelIndex index(int row, int column, const ui::ModelIndex& parent = ui::ModelIndex()) const override final;

        virtual void visualize(const ui::ModelIndex& item, int columnCount, ui::ElementPtr& content) const override final;
        virtual bool compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex = 0) const override final;
        virtual bool filter(const ui::ModelIndex& id, const ui::SearchPattern& filter, int colIndex = 0) const override final;

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

        void addFile(const ManagedDirectory* currentDirectory, base::StringView<char> selectedAssetPath);
        void addFiles(const ManagedDirectory* currentDirectory, const base::Array<base::StringBuf>& selectedAssetPaths);

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
        ui::NotebookPtr m_configProps;
        //ui::DataInspectorPtr m_properties;

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

        void addFile(const ManagedDirectory* currentDirectory, base::StringView<char> selectedAssetPath);
        void addFiles(const ManagedDirectory* currentDirectory, const base::Array<base::StringBuf> &selectedAssetPaths);

    private:
        base::RefPtr<AssetImportPrepareTab> m_prepareTab;
    };

    //--

} // ed