/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"

#include "managedDirectory.h"
#include "managedFile.h"
#include "managedFileFormat.h"
#include "managedDepot.h"
#include "managedFileAssetChecks.h"

#include "editorConfig.h"
#include "editorService.h"

#include "assetFileImportWindow.h"
#include "assetBrowserTabFiles.h"

#include "base/ui/include/uiImage.h"
#include "base/ui/include/uiTextLabel.h"
#include "base/ui/include/uiMenuBar.h"
#include "base/ui/include/uiWindow.h"
#include "base/ui/include/uiAbstractItemView.h"
#include "base/ui/include/uiCheckBox.h"
#include "base/ui/include/uiStyleValue.h"
#include "base/ui/include/uiEditBox.h"
#include "base/ui/include/uiComboBox.h"
#include "base/ui/include/uiToolBar.h"
#include "base/ui/include/uiSplitter.h"
#include "base/ui/include/uiListView.h"
#include "base/ui/include/uiColumnHeaderBar.h"
#include "base/ui/include/uiNotebook.h"
#include "base/ui/include/uiDockNotebook.h"
#include "base/ui/include/uiDataInspector.h"
#include "base/resource_compiler/include/importInterface.h"
#include "base/resource_compiler/include/importFileService.h"
#include "base/io/include/ioSystem.h"
#include "base/resource/include/resourceMetadata.h"
#include "base/resource_compiler/include/importFileList.h"
#include "base/ui/include/uiSearchBar.h"

namespace ed
{

    //--

    AssetImportListModel::AssetImportListModel()
    {}

    AssetImportListModel::~AssetImportListModel()
    {}

    base::res::ImportListPtr AssetImportListModel::compileResourceList() const
    {
        base::Array<base::res::ImportFileEntry> fileEntries;

        for (const auto& file : m_files)
        {
            if (file->m_importFlag)
            {
                auto& outEntry = fileEntries.emplaceBack();
                outEntry.assetPath = file->m_sourceAssetPath;
                outEntry.depotPath = fileDepotPath(file);
                outEntry.userConfiguration = CloneObject(file->m_configuration);
            }
        }

        return base::CreateSharedPtr<base::res::ImportList>(std::move(fileEntries));
    }

    void AssetImportListModel::clearFiles()
    {
        beingRemoveRows(ui::ModelIndex(), 0, m_files.size());

        for (auto& file : m_files)
        {
        }
        m_files.clear();

        endRemoveRows();
    }

    static base::StringView<char> ExtractFileExtension(base::StringView<char> assetPath)
    {
        return assetPath.afterLast(".");
    }

    static base::StringView<char> ExtractFileName(base::StringView<char> assetPath)
    {
        return assetPath.afterLastOrFull("/").afterLastOrFull("\\").beforeFirstOrFull(".");
    }

    base::res::ResourceConfigurationPtr AssetImportListModel::fileConfiguration(const ui::ModelIndex& file) const
    {
        if (const auto data = fileForIndex(file))
            return data->m_configuration;
        return nullptr;
    }

    base::StringBuf AssetImportListModel::fileDepotPath(const FileData* data) const
    {
        if (data)
        {
            if (data->m_targetDirectory && data->m_targetFileName)
            {
                if (const auto ext = base::res::IResource::GetResourceExtensionForClass(data->m_targetClass))
                {
                    return base::StringBuf(base::TempString("{}{}.{}", data->m_targetDirectory->depotPath(), data->m_targetFileName, ext));
                }
            }
        }

        return base::StringBuf::EMPTY();
    }

    base::StringBuf AssetImportListModel::fileDepotPath(const ui::ModelIndex& file) const
    {
        return fileDepotPath(fileForIndex(file));
    }

    ManagedFile* AssetImportListModel::fileManagedFile(const FileData* file) const
    {
        if (const auto depotPath = fileDepotPath(file))
            return base::GetService<Editor>()->managedDepot().findManagedFile(depotPath);
        return nullptr;
    }

    ManagedFile* AssetImportListModel::fileManagedFile(const ui::ModelIndex& file) const
    {
        if (const auto depotPath = fileDepotPath(file))
            return base::GetService<Editor>()->managedDepot().findManagedFile(depotPath);
        return nullptr;
    }

    base::StringBuf AssetImportListModel::fileSourceAssetPath(const ui::ModelIndex& file) const
    {
        if (const auto data = fileForIndex(file))
            return data->m_sourceAssetPath;
        return base::StringBuf::EMPTY();
    }

    base::io::AbsolutePath AssetImportListModel::fileSourceAssetAbsolutePath(const ui::ModelIndex& file) const
    {
        if (const auto data = fileForIndex(file))
        {
            base::StringBuf contextPath;
            if (base::GetService<base::res::ImportFileService>()->resolveContextPath(data->m_sourceAssetPath, contextPath))
                return base::io::AbsolutePath::Build(base::UTF16StringBuf(contextPath.c_str()));
        }

        return base::io::AbsolutePath();
    }

    ui::ModelIndex AssetImportListModel::addFileInternal(const base::RefPtr<FileData>& data)
    {
        auto index = m_files.size();

        beingInsertRows(ui::ModelIndex(), index, 1);
        m_files.pushBack(data);
        endInsertRows();

        return indexForFile(data);
    }

    ui::ModelIndex AssetImportListModel::addNewImportFile(const base::StringBuf& sourcePath, TImportClass resourceClass, const StringBuf& fileName, const ManagedDirectory* directory, const base::res::ResourceConfigurationPtr& specificUserConfiguration)
    {
        if (sourcePath && resourceClass && directory)
        {
            // do not add the same file twice
            for (const auto& existing : m_files)
                if (existing->m_targetDirectory == directory && existing->m_targetClass == resourceClass && existing->m_targetFileName == fileName)
                    return indexForFile(existing);

            // get required config class, we need the source path to know what we will be importing
            TConfigClass configClass;
            const auto sourceExtension = ExtractFileExtension(sourcePath.view());
            base::res::IResourceImporter::ListImportConfigurationForExtension(sourceExtension, resourceClass, configClass);
            DEBUG_CHECK_EX(configClass, "No config class specified");
            if (!configClass)
                return ui::ModelIndex();

            // create new entry
            auto fileEntry = CreateSharedPtr<FileData>();
            fileEntry->m_existingFile = nullptr;
            fileEntry->m_importFlag = true;
            fileEntry->m_sourceAssetPath = sourcePath;
            fileEntry->m_targetClass = resourceClass;
            fileEntry->m_targetFileName = fileName;
            fileEntry->m_targetDirectory = directory;

            // build the base import config from source asset directory
            auto baseConfig = base::GetService<base::res::ImportFileService>()->compileBaseResourceConfiguration(sourcePath, configClass);

            // create default file configuration
            if (specificUserConfiguration && specificUserConfiguration->is(configClass))
            {
                fileEntry->m_configuration = CloneObject(specificUserConfiguration);
                fileEntry->m_configuration->rebase(baseConfig);
            }
            else
            {
                fileEntry->m_configuration = configClass.create();
                fileEntry->m_configuration->rebase(baseConfig);
            }

            // always set the imported by stuff
            // TODO: change to VSC user name
            fileEntry->m_configuration->setupDefaultImportMetadata();

            // add to internal file list and create UI model index
            return addFileInternal(fileEntry);
        }

        return ui::ModelIndex();
    }

    ui::ModelIndex AssetImportListModel::addReimportFile(ManagedFile* file)
    {
        // no file, what a waste
        if (!file)
            return ui::ModelIndex();

        // do not add the same file twice
        for (const auto& existing : m_files)
            if (existing->m_existingFile == file)
                return indexForFile(existing);

        // get resource class and the required import config class
        const auto resourceClass = file->fileFormat().nativeResourceClass();
        DEBUG_CHECK_EX(resourceClass, "File was not imported");

        // load the metadata for the file, if it's gone/not accessible file can't be reimported
        const auto metadata = file->loadMetadata();
        DEBUG_CHECK_EX(metadata && metadata->importDependencies.empty(), "File was not imported");
        if (!metadata || metadata->importDependencies.empty())
            return ui::ModelIndex();

        // get the source asset path
        const auto& sourcePath = metadata->importDependencies[0].importPath;
        const auto sourceExtension = ExtractFileExtension(sourcePath.view());

        // get required config class, we need the source path to know what we will be importing
        TConfigClass configClass;
        base::res::IResourceImporter::ListImportConfigurationForExtension(sourceExtension, resourceClass, configClass);
        DEBUG_CHECK_EX(configClass, "No config class specified");
        if (!configClass)
            return ui::ModelIndex();

        // build the base import config from source asset directory
        auto baseConfig = base::GetService<base::res::ImportFileService>()->compileBaseResourceConfiguration(sourcePath, configClass);

        // apply the ASSET SPECIFIC config from a followup-import
        DEBUG_CHECK_EX(metadata->importBaseConfiguration, "No base configuration stored in metadata, very strange");
        if (metadata->importBaseConfiguration)
        {
            metadata->importBaseConfiguration->rebase(baseConfig);
            metadata->importBaseConfiguration->parent(nullptr);
            baseConfig = metadata->importBaseConfiguration;
        }

        // create new entry
        auto fileEntry = CreateSharedPtr<FileData>();
        fileEntry->m_existingFile = file;
        fileEntry->m_importFlag = true;
        fileEntry->m_sourceAssetPath = sourcePath;
        fileEntry->m_targetClass = resourceClass;
        fileEntry->m_targetFileName = file->name();
        fileEntry->m_targetDirectory = file->parentDirectory();

        // use the loaded user configuration
        DEBUG_CHECK_EX(metadata->importUserConfiguration, "No user configuration stored in metadata, very strange");
        DEBUG_CHECK_EX(!metadata->importUserConfiguration || metadata->importUserConfiguration->cls() == configClass, "User configuration stored in metadata has different class that currenyl recommended one");
        if (metadata->importUserConfiguration && metadata->importUserConfiguration->cls() == configClass)
        {
            fileEntry->m_configuration = metadata->importUserConfiguration;
            metadata->importUserConfiguration->parent(nullptr);
        }
        else
        {
            fileEntry->m_configuration = configClass.create();
        }

        // always set the imported by stuff
        // TODO: change to VSC user name
        fileEntry->m_configuration->setupDefaultImportMetadata();

        // rebase user config onto the folded base config
        fileEntry->m_configuration->rebase(baseConfig);

        // add to internal file list and create UI model index
        return addFileInternal(fileEntry);
    }

    void AssetImportListModel::removeFiles(const base::Array<ui::ModelIndex>& indices)
    {
        base::Array<base::RefPtr<FileData>> files;
        files.reserve(indices.size());

        for (const auto index : indices)
            if (auto file = fileForIndex(index))
                files.pushBack(file);

        for (const auto& file : files)
        {
            auto fileIndex = m_files.find(file);
            if (fileIndex != INDEX_NONE)
            {
                beingRemoveRows(ui::ModelIndex(), fileIndex, 1);
                m_files.erase(fileIndex);
                endRemoveRows();
            }
        }
    }
    
    AssetImportListModel::FileData* AssetImportListModel::fileForIndexFast(const ui::ModelIndex& index) const
    {
        return static_cast<AssetImportListModel::FileData*>(index.weakRef().lock());
    }

    base::RefPtr<AssetImportListModel::FileData> AssetImportListModel::fileForIndex(const ui::ModelIndex& index) const
    {
        auto* ptr = fileForIndexFast(index);
        return base::RefPtr<AssetImportListModel::FileData>(NoAddRef(ptr));
    }

    ui::ModelIndex AssetImportListModel::indexForFile(const FileData* entry) const
    {
        auto index = m_files.find(entry);
        if (index != -1)
            return ui::ModelIndex(this, index, 0, m_files[index]);
        return ui::ModelIndex();
    }

    uint32_t AssetImportListModel::rowCount(const ui::ModelIndex& parent) const
    {
        if (!parent.valid())
            return m_files.size();
        return 0;
    }

    bool AssetImportListModel::hasChildren(const ui::ModelIndex& parent) const
    {
        return !parent.valid();
    }

    bool AssetImportListModel::hasIndex(int row, int col, const ui::ModelIndex& parent /*= ui::ModelIndex()*/) const
    {
        return !parent.valid() && col == 0 && row >= 0 && row < (int)m_files.size();
    }

    ui::ModelIndex AssetImportListModel::parent(const ui::ModelIndex& item /*= ui::ModelIndex()*/) const
    {
        return ui::ModelIndex();
    }

    ui::ModelIndex AssetImportListModel::index(int row, int column, const ui::ModelIndex& parent) const
    {
        if (hasIndex(row, column, parent))
            return ui::ModelIndex(this, row, column, m_files[row]);
        return ui::ModelIndex();
    }

    static base::StringView<char> GetClassDisplayName(ClassType currentClass)
    {
        auto name = currentClass.name().view();
        name = name.afterLastOrFull("::");
        return name;
    }

    static base::StringView<char> StaticText(AssetImportCheckStatus status)
    {
        switch (status)
        {
            case AssetImportCheckStatus::Checking: return "[color:#888]Checking...[/color]";
            case AssetImportCheckStatus::New: return "[color:#FF8]New[/color]";
            case AssetImportCheckStatus::UpToDate: return "[color:#88F]Up to date[/color]";
            case AssetImportCheckStatus::NewConfig: return "[color:#8F8]New Config[/color]";
            case AssetImportCheckStatus::NewContent: return "[color:#8F8]New Content[/color]";
            case AssetImportCheckStatus::MissingFiles: return "[color:#F88]Missing Files[/color]";
        }

        return "[color:#888]Unknown[/color]";
    }


    void AssetImportListModel::visualize(const ui::ModelIndex& item, int columnCount, ui::ElementPtr& content) const
    {
        if (auto file = fileForIndexFast(item))
        {
            if (!content)
            {
                content = base::CreateSharedPtr<ui::IElement>();
                content->layoutMode(ui::LayoutMode::Columns);

                {
                    auto importFlag = content->createNamedChild<ui::CheckBox>("ImportFlag"_id);
                    importFlag->OnClick = [this, file](ui::CheckBox* box, ui::IElement* parent)
                    {
                        file->m_importFlag = box->stateBool();
                    };
                }

                {
                    auto fileName = content->createNamedChild<ui::TextLabel>("Status"_id);
                    fileName->customMargins(4, 0, 4, 0);

                    if (file->m_existingFile == nullptr)
                        fileName->text("[tag:#8A8]New[/tag]");
                    else
                        fileName->text("[tag:#888]Reimport[/tag]");

                    fileName->expand();
                }

                if (file->m_existingFile == nullptr)
                {
                    auto fileName = content->createNamedChild<ui::EditBox>("FileName"_id);
                    fileName->customMargins(4, 0, 4, 0);
                    fileName->text(file->m_targetFileName);
                    fileName->expand();

                    fileName->OnTextModified = [this, file](ui::EditBox* box, ui::IElement* parent)
                    {
                        const auto name = box->text();
                        if (file->m_targetFileName != name)
                        {
                            file->m_targetFileName = name;
                        }
                    };
                }
                else
                {
                    auto fileName = content->createNamedChild<ui::TextLabel>("FileNameStatic"_id);
                    fileName->customMargins(4, 0, 4, 0);
                    fileName->text(file->m_targetFileName);
                }

                {
                    auto fileClass = content->createNamedChild<ui::TextLabel>("FileClass"_id);
                    fileClass->customMargins(4, 0, 4, 0);
                }

                {
                    auto sourcePath = content->createNamedChild<ui::TextLabel>("SourcePath"_id);
                    sourcePath->expand();
                    sourcePath->customMargins(4, 0, 4, 0);
                    sourcePath->text(file->m_sourceAssetPath);
                }

                {
                    auto dirPath = content->createNamedChild<ui::TextLabel>("DirPath"_id);
                    dirPath->expand();
                    dirPath->customMargins(4, 0, 4, 0);
                }
            }

            if (auto elem = content->findChildByName<ui::CheckBox>("ImportFlag"_id))
                elem->state(file->m_importFlag);

            /*if (auto elem = content->findChildByName<ui::TextLabel>("Status"_id))
                elem->text(StaticText(file->m_importStatus));*/

            if (auto elem = content->findChildByName<ui::TextLabel>("FileClass"_id))
                elem->text(TempString("[img:class] {}", file->m_targetClass));

            if (auto elem = content->findChildByName<ui::TextLabel>("DirPath"_id))
                elem->text(file->m_targetDirectory->depotPath());
        }
    }

    bool AssetImportListModel::compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex /*= 0*/) const
    {
        const auto* firstFile = fileForIndexFast(first);
        const auto* secondFile = fileForIndexFast(second);
        if (firstFile && secondFile)
        {
            if (colIndex == 0)
                return (int)firstFile->m_importFlag < (int)secondFile->m_importFlag;

            /*if (colIndex == 1)
                return (int)firstFile->m_importStatus < (int)secondFile->m_importStatus;*/

            if (colIndex == 2)
                return firstFile->m_targetFileName < secondFile->m_targetFileName;

            if (colIndex == 3)
                return firstFile->m_targetClass < secondFile->m_targetClass;

            if (colIndex == 4)
                return firstFile->m_sourceAssetPath < secondFile->m_sourceAssetPath;

            if (colIndex == 5)
                return firstFile->m_targetDirectory < secondFile->m_targetDirectory;
        }

        return first < second;
    }

    ui::PopupPtr AssetImportListModel::contextMenu(ui::AbstractItemView* view, const base::Array<ui::ModelIndex>& indices) const
    {
        /*base::InplaceArray<FileData*, 20> files;
        for (const auto& index : indices)
            if (auto file = fileForIndex(index))
                files.pushBack(file);

        if (files.empty())
            return nullptr;*/

        auto menu = base::CreateSharedPtr<ui::MenuButtonContainer>();

        if (indices.size() == 1)
        {
            auto rootFile = indices[0];
            if (auto* managedFile = fileManagedFile(rootFile))
            {
                menu->createCallback("Show in depot...", "[img:zoom]") = [managedFile]() { 
                    base::GetService<ed::Editor>()->selectFile(managedFile);
                };
            }

            if (const auto path = fileSourceAssetAbsolutePath(rootFile))
            {
                if (IO::GetInstance().fileExists(path))
                {
                    menu->createCallback("Show source asset...", "[img:find_blue]") = [path]() {
                        IO::GetInstance().showFileExplorer(path);
                    };
                }
            }

            menu->createSeparator();
        }

        {
            menu->createCallback("Remove from list", "[img:delete]") = [this, indices]() {
                const_cast<AssetImportListModel*>(this)->removeFiles(indices);
            };
            menu->createSeparator();
        }

        // TODO: change class
        // TODO: change directory
        // TODO: copy settings/paste settings

        return menu->convertToPopup();
    }

    bool AssetImportListModel::filter(const ui::ModelIndex& id, const ui::SearchPattern& filter, int colIndex /*= 0*/) const
    {
        if (auto* file = fileForIndexFast(id))
        {
            if (filter.testString(file->m_targetFileName))
                return true;
            if (filter.testString(file->m_sourceAssetPath))
                return true;
        }

        return false;
    }

    const base::Array<base::SpecificClassType<base::res::IResource>>& AssetImportListModel::getClassesForSourceExtension(StringView<char> sourcePath) const
    {
        const auto ext = sourcePath.afterLast(".");
        if (const auto* list = m_classPerExtensionMap.find(ext))
            return *list;

        auto& entry = m_classPerExtensionMap[StringBuf(ext)];
        base::res::IResourceImporter::ListImportableResourceClassesForExtension(ext, entry);
        return entry;
    }

    ///--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetImportPrepareTab);
    RTTI_END_TYPE();

    AssetImportPrepareTab::AssetImportPrepareTab()
        : ui::DockPanel("[img: import] File list")
        , OnStartImport(this, "StartImport"_id)
    {
        layoutVertical();

        m_toolbar = createChild<ui::ToolBar>();
        m_toolbar->createCallback(ui::ToolbarButtonSetup().icon("open").caption("Load list").tooltip("Load current import list from XML"), [this]() { cmdLoadList(); });
        m_toolbar->createCallback(ui::ToolbarButtonSetup().icon("save").caption("Save list").tooltip("Save current import list to XML"), [this]() { cmdSaveList(); });
        m_toolbar->createSeparator();
        m_toolbar->createCallback(ui::ToolbarButtonSetup().icon("delete").caption("Clear").tooltip("Clear import list"), [this]() { cmdClearList(); });
        m_toolbar->createSeparator();
        m_toolbar->createCallback(ui::ToolbarButtonSetup().icon("file_add").caption("Add files").tooltip("Add files to import list"), [this]() { cmdAddFiles(); });
        m_toolbar->createCallback(ui::ToolbarButtonSetup().icon("file_delete").caption("Remove files").tooltip("Remove files from import list"), [this]() { cmdRemoveFiles(); });
        m_toolbar->createSeparator();
        m_toolbar->createCallback(ui::ToolbarButtonSetup().icon("cog").caption("[tag:#8A9]Start[/tag]").tooltip("Start importing files"), [this]() { cmdStartImport(); });

        auto split = createChild<ui::Splitter>(ui::Direction::Vertical, 0.7f);

        {
            auto leftPanel = split->createChild<ui::IElement>();
            leftPanel->layoutVertical();
            leftPanel->expand();

            auto filter = leftPanel->createChild<ui::SearchBar>();

            {
                auto bar = leftPanel->createChild<ui::ColumnHeaderBar>();
                bar->addColumn("", 30.0f, true, true, false);
                bar->addColumn("Status", 80.0f, true, true, false);
                bar->addColumn("File name", 200.0f, false);
                bar->addColumn("Class", 150.0f, true, true, true);
                bar->addColumn("Source path", 600.0f, false);
                bar->addColumn("Directory", 600.0f, false);
            }

            m_fileList = leftPanel->createChild<ui::ListView>();
            m_fileList->expand();
            m_fileList->columnCount(5);

            m_filesListModel = base::CreateSharedPtr<AssetImportListModel>();
            m_fileList->model(m_filesListModel);
            filter->bindItemView(m_fileList);

            m_fileList->OnSelectionChanged = [this]()
            {
                updateSelection();
            };
        }

        {
            auto rightPanel = split->createChild<ui::IElement>();
            rightPanel->layoutVertical();
            rightPanel->expand();

            m_configTabs = rightPanel->createChild<ui::Notebook>();
            m_configTabs->expand();

            m_configProperties = base::CreateSharedPtr<ui::DataInspector>();
            m_configProperties->customStyle<base::StringBuf>("title"_id, "[img:properties] Config");
            m_configProperties->expand();

            m_configTabs->attachTab(m_configProperties);
        }
    }

    AssetImportPrepareTab::~AssetImportPrepareTab()
    {}
    
    static const TImportClassRegistry& ExtractResourceForExtension(base::StringView<char> ext, TImportClassRegistryMap& registry)
    {
        if (const auto* classList = registry.find(ext))
            return *classList;

        auto& classList = registry[StringBuf(ext)];
        base::res::IResourceImporter::ListImportableResourceClassesForExtension(ext, classList);
        return classList;
    }

    void AssetImportPrepareTab::addNewImportFiles(const ManagedDirectory* currentDirectory, base::SpecificClassType<base::res::IResource> resourceClass, const base::Array<base::StringBuf>& selectedAssetPaths)
    {
        static TImportClassRegistryMap ClassRegistry;

        base::InplaceArray<ui::ModelIndex, 20> createdIndices;
        for (const auto& path : selectedAssetPaths)
        {
            if (path)
            {
                const auto ext = ExtractFileExtension(path);
                const auto fileName = ExtractFileName(path);

                const auto& targetClasses = ExtractResourceForExtension(ext, ClassRegistry);
                if (!targetClasses.empty())
                {
                    auto importClass = resourceClass;
                    if (!targetClasses.contains(importClass))
                        importClass = targetClasses[0];

                    if (auto index = m_filesListModel->addNewImportFile(path, importClass, base::StringBuf(fileName), currentDirectory))
                    {
                        createdIndices.pushBack(index);
                    }
                }
            }
        }

        m_fileList->select(createdIndices);
        updateSelection();
    }

    void AssetImportPrepareTab::addReimportFiles(const base::Array<ManagedFile*>& files)
    {
        base::InplaceArray<ui::ModelIndex, 20> createdIndices;
        for (auto* file : files)
        {
            if (file)
            {
                if (auto index = m_filesListModel->addReimportFile(file))
                {
                    createdIndices.pushBack(index);
                }
            }
        }

        m_fileList->select(createdIndices);
        updateSelection();
    }

    void AssetImportPrepareTab::updateSelection()
    {
        auto selection = m_fileList->selection().keys();

        base::Array<base::res::ResourceConfigurationPtr> configurations;
        configurations.reserve(selection.size());

        for (const auto& index : selection)
        {
            if (auto config = m_filesListModel->fileConfiguration(index))
                configurations.pushBack(config);
        }

        if (configurations.empty())
        {
            m_configProperties->bindNull();
        }
        else if (configurations.size() == 1)
        {
            m_configProperties->bindObject(configurations[0]);
        }
        else
        {
            // TODO: multi object
        }
    }

    void AssetImportPrepareTab::cmdClearList()
    {
        m_filesListModel->clearFiles();
    }

    ManagedDirectory* AssetImportPrepareTab::contextDirectory()
    {
        return base::GetService<Editor>()->selectedDirectory();
    }

    void AssetImportPrepareTab::cmdAddFiles()
    {
        if (auto* dir = contextDirectory())
            ImportNewFiles(this, nullptr, dir);
    }

    void AssetImportPrepareTab::cmdRemoveFiles()
    {
        auto selection = m_fileList->selection().keys();
        m_filesListModel->removeFiles(selection);

        updateSelection();
    }

    void AssetImportPrepareTab::cmdSaveList()
    {
        base::GetService<Editor>()->saveToXML(this, "AssetImportList", [this]() {
            return m_filesListModel->compileResourceList();
            });
    }

    void AssetImportPrepareTab::cmdLoadList()
    {
        if (const auto fileList = base::GetService<Editor>()->loadFromXML<base::res::ImportList>(this, "AssetImportList"))
        {
            m_filesListModel->clearFiles();
            addFilesFromList(*fileList);
        }
    }

    void AssetImportPrepareTab::cmdAppendList()
    {
        if (const auto fileList = base::GetService<Editor>()->loadFromXML<base::res::ImportList>(this, "AssetImportList"))
        {
            addFilesFromList(*fileList);
        }
    }

    void AssetImportPrepareTab::addFilesFromList(const base::res::ImportList& list)
    {
        base::InplaceArray<ui::ModelIndex, 20> indices;

        for (const auto& entry : list.files())
        {
            if (auto* file = base::GetService<Editor>()->managedDepot().findManagedFile(entry.depotPath))
            {
                if (auto index = m_filesListModel->addReimportFile(file))
                    indices.pushBack(index);
            }
            else
            {
                if (auto* directory = base::GetService<Editor>()->managedDepot().findPath(entry.depotPath))
                {
                    const auto fileName = entry.depotPath.view().afterLastOrFull("/").beforeFirstOrFull(".");
                    const auto fileExt = entry.depotPath.view().afterLast(".");

                    const auto resourceClass = base::res::IResource::FindResourceClassByExtension(fileExt);
                    if (auto index = m_filesListModel->addNewImportFile(entry.assetPath, resourceClass, base::StringBuf(fileName), directory, entry.userConfiguration))
                        indices.pushBack(index);
                }
            }
        }

        if (indices.empty())
        {
            ui::PostWindowMessage(this, ui::MessageType::Warning, "ImportList"_id, base::TempString("No files out of {} were added to the list", list.files().size()));
        }
        else if (indices.size() != list.files().size())
        {
            ui::PostWindowMessage(this, ui::MessageType::Warning, "ImportList"_id, base::TempString("Added {} files (out of {}) to the list", indices.size(), list.files().size()));
        }
        else
        {
            ui::PostWindowMessage(this, ui::MessageType::Info, "ImportList"_id, base::TempString("All {} files added to the list", list.files().size()));
        }

        m_fileList->select(indices);
        updateSelection();
    }

    void AssetImportPrepareTab::cmdStartImport()
    {

    }

    ///--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetImportWindow);
    RTTI_END_TYPE();

    AssetImportWindow::AssetImportWindow()
    {
        layoutVertical();

        createChild<ui::WindowTitleBar>("BoomerEditor - Asset Import", "[img:app_icon_import32]");
        customMinSize(1200, 900);

        auto notebook = createChild<ui::DockNotebook>(nullptr);
        notebook->expand();

        {
            m_prepareTab = notebook->createChild<AssetImportPrepareTab>();
            m_prepareTab->expand();
        }
    }

    AssetImportWindow::~AssetImportWindow()
    {}

    void AssetImportWindow::loadConfig(const ConfigGroup& config)
    {}

    void AssetImportWindow::saveConfig(ConfigGroup& config) const
    {
    }

    void AssetImportWindow::addNewImportFiles(const ManagedDirectory* currentDirectory, TImportClass resourceClass, const base::Array<base::StringBuf>& selectedAssetPaths)
    {
        m_prepareTab->addNewImportFiles(currentDirectory, resourceClass, selectedAssetPaths);
    }

    void AssetImportWindow::addReimportFiles(const base::Array<ManagedFile*>& files)
    {
        m_prepareTab->addReimportFiles(files);
    }

    //--

} // ed