/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"

#include "managedDirectory.h"
#include "managedDepot.h"
#include "managedFile.h"
#include "managedFileFormat.h"
#include "managedFileAssetChecks.h"
#include "managedFileNativeResource.h"

#include "editorService.h"

#include "assetFileImportPrepareDialog.h"
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
#include "base/ui/include/uiTextValidation.h"
#include "base/resource_compiler/include/importInterface.h"
#include "base/resource_compiler/include/importFileService.h"
#include "base/io/include/ioSystem.h"
#include "base/resource/include/resourceMetadata.h"
#include "base/resource_compiler/include/importFileList.h"
#include "base/ui/include/uiSearchBar.h"
#include "assetFileImportJob.h"


namespace ed
{

    //--

    AssetImportListModel::AssetImportListModel()
    {}

    AssetImportListModel::~AssetImportListModel()
    {}

    res::ImportListPtr AssetImportListModel::compileResourceList(bool activeOnly) const
    {
        Array<res::ImportFileEntry> fileEntries;

        for (const auto& file : m_files)
        {
            if (file->importFlag || !activeOnly)
            {
                if (activeOnly && !ManagedItem::ValidateFileName(file->targetFileName))
                    continue;

                auto& outEntry = fileEntries.emplaceBack();
                outEntry.enabled = file->importFlag;
                outEntry.assetPath = file->sourceAssetPath;
                outEntry.depotPath = fileDepotPath(file);
                outEntry.userConfiguration = CloneObject(file->configuration);
            }
        }

        return RefNew<res::ImportList>(std::move(fileEntries));
    }

    bool AssetImportListModel::hasFiles() const
    {
        return m_files.size() != 0;
    }

    bool AssetImportListModel::hasImportableFiles() const
    {
        for (const auto& file : m_files)
            if (file->importFlag)
                return true;

        return false;
    }

    void AssetImportListModel::clearFiles()
    {
        m_files.clear();
        reset();
    }

    static StringView ExtractFileExtension(StringView assetPath)
    {
        return assetPath.afterLastOrFull("/").afterLastOrFull("\\").afterFirst(".");
    }

    static StringView ExtractFileName(StringView assetPath)
    {
        return assetPath.afterLastOrFull("/").afterLastOrFull("\\").beforeFirstOrFull(".");
    }

    res::ResourceConfigurationPtr AssetImportListModel::fileConfiguration(const ui::ModelIndex& file) const
    {
        if (file.model() == this)
            if (const auto data = file.unsafe<FileData>())
                return data->configuration;

        return nullptr;
    }

    StringBuf AssetImportListModel::fileDepotPath(const FileData* data) const
    {
        if (data)
        {
            if (data->targetDirectory && data->targetFileName)
            {
                if (const auto ext = res::IResource::GetResourceExtensionForClass(data->targetClass))
                {
                    return StringBuf(TempString("{}{}.{}", data->targetDirectory->depotPath(), data->targetFileName, ext));
                }
            }
        }

        return StringBuf::EMPTY();
    }

    StringBuf AssetImportListModel::fileDepotPath(const ui::ModelIndex& file) const
    {
        if (file.model() == this)
            if (const auto data = file.unsafe<FileData>())
                return fileDepotPath(data);

        return StringBuf::EMPTY();
    }

    ManagedFileNativeResource* AssetImportListModel::fileManagedFile(const FileData* file) const
    {
        if (const auto depotPath = fileDepotPath(file))
            return rtti_cast<ManagedFileNativeResource>(GetEditor()->managedDepot().findManagedFile(depotPath));
        return nullptr;
    }

    ManagedFileNativeResource* AssetImportListModel::fileManagedFile(const ui::ModelIndex& file) const
    {
        if (const auto depotPath = fileDepotPath(file))
            return rtti_cast<ManagedFileNativeResource>(GetEditor()->managedDepot().findManagedFile(depotPath));
        return nullptr;
    }

    StringBuf AssetImportListModel::fileSourceAssetPath(const ui::ModelIndex& file) const
    {
        if (file.model() == this)
            if (const auto data = file.unsafe<FileData>())
                return data->sourceAssetPath;

        return StringBuf::EMPTY();
    }

    StringBuf AssetImportListModel::fileSourceAssetAbsolutePath(const ui::ModelIndex& file) const
    {
        if (file.model() == this)
        {
            if (const auto data = file.unsafe<FileData>())
            {
                StringBuf contextPath;
                if (GetService<res::ImportFileService>()->resolveContextPath(data->sourceAssetPath, contextPath))
                    return contextPath;
            }
        }

        return StringBuf::EMPTY();
    }

    ui::ModelIndex AssetImportListModel::addNewImportFile(const StringBuf& sourcePath, TImportClass resourceClass, const StringBuf& fileName, const ManagedDirectory* directory, const res::ResourceConfigurationPtr& specificUserConfiguration)
    {
        if (sourcePath && resourceClass && directory)
        {
            // do not add the same file twice
            for (const auto& existing : m_files)
                if (existing->targetDirectory == directory && existing->targetClass == resourceClass && existing->targetFileName == fileName)
                    return existing->index;

            // get required config class, we need the source path to know what we will be importing
            TConfigClass configClass;
            const auto sourceExtension = ExtractFileExtension(sourcePath.view());
            res::IResourceImporter::ListImportConfigurationForExtension(sourceExtension, resourceClass, configClass);
            DEBUG_CHECK_EX(configClass, "No config class specified");
            if (!configClass)
                return ui::ModelIndex();

            // create new entry
            auto fileEntry = RefNew<FileData>();
            fileEntry->index = ui::ModelIndex(this, fileEntry);
            fileEntry->existingFile = nullptr;
            fileEntry->importFlag = true;
            fileEntry->sourceAssetPath = sourcePath;
            fileEntry->targetClass = resourceClass;
            fileEntry->targetFileName = fileName;
            fileEntry->targetDirectory = directory;

            // build the base import config from source asset directory
            auto baseConfig = GetService<res::ImportFileService>()->compileBaseResourceConfiguration(sourcePath, configClass);

            // create default file configuration
            if (specificUserConfiguration && specificUserConfiguration->is(configClass))
            {
                fileEntry->configuration = CloneObject(specificUserConfiguration);
                fileEntry->configuration->rebase(baseConfig);
            }
            else
            {
                fileEntry->configuration = configClass.create();
                fileEntry->configuration->rebase(baseConfig);
            }

            // always set the imported by stuff
            // TODO: change to VSC user name
            fileEntry->configuration->setupDefaultImportMetadata();

            // add to internal file list and create UI model index
            m_files.pushBack(fileEntry);
            notifyItemAdded(ui::ModelIndex(), fileEntry->index);
            return fileEntry->index;
        }

        return ui::ModelIndex();
    }

    ui::ModelIndex AssetImportListModel::addReimportFile(ManagedFileNativeResource* file, const res::ResourceConfigurationPtr& specificUserConfiguration)
    {
        // no file, what a waste
        if (!file)
            return ui::ModelIndex();

        // do not add the same file twice
        for (const auto& existing : m_files)
            if (existing->existingFile == file)
                return existing->index;

        // get resource class and the required import config class
        const auto resourceClass = file->fileFormat().nativeResourceClass();
        DEBUG_CHECK_EX(resourceClass, "File was not imported");

        // load the metadata for the file, if it's gone/not accessible file can't be reimported
        const auto metadata = file->loadMetadata();
        DEBUG_CHECK_EX(metadata && !metadata->importDependencies.empty(), "File was not imported");
        if (!metadata || metadata->importDependencies.empty())
            return ui::ModelIndex();

        // get the source asset path
        const auto& sourcePath = metadata->importDependencies[0].importPath;
        const auto sourceExtension = ExtractFileExtension(sourcePath.view());

        // get required config class, we need the source path to know what we will be importing
        TConfigClass configClass;
        res::IResourceImporter::ListImportConfigurationForExtension(sourceExtension, resourceClass, configClass);
        DEBUG_CHECK_EX(configClass, "No config class specified");
        if (!configClass)
            return ui::ModelIndex();

        // build the base import config from source asset directory
        auto baseConfig = GetService<res::ImportFileService>()->compileBaseResourceConfiguration(sourcePath, configClass);

        // apply the ASSET SPECIFIC config from a followup-import
        DEBUG_CHECK_EX(metadata->importBaseConfiguration, "No base configuration stored in metadata, very strange");
        if (metadata->importBaseConfiguration && metadata->importBaseConfiguration->is(configClass))
        {
            metadata->importBaseConfiguration->rebase(baseConfig);
            metadata->importBaseConfiguration->parent(nullptr);
            baseConfig = metadata->importBaseConfiguration;
        }

        // create new entry
        auto fileEntry = RefNew<FileData>();
        fileEntry->index = ui::ModelIndex(this, fileEntry);
        fileEntry->existingFile = file;
        fileEntry->importFlag = true;
        fileEntry->sourceAssetPath = sourcePath;
        fileEntry->targetClass = resourceClass;
        fileEntry->targetFileName = file->name().stringBeforeLast(".");
        fileEntry->targetDirectory = file->parentDirectory();

        // use the loaded user configuration
        if (specificUserConfiguration && specificUserConfiguration->is(configClass))
        {
            fileEntry->configuration = CloneObject(specificUserConfiguration);
        }
        else
        {
            DEBUG_CHECK_EX(metadata->importUserConfiguration, "No user configuration stored in metadata, very strange");
            DEBUG_CHECK_EX(!metadata->importUserConfiguration || metadata->importUserConfiguration->cls() == configClass, "User configuration stored in metadata has different class that currenyl recommended one");
            if (metadata->importUserConfiguration && metadata->importUserConfiguration->cls() == configClass)
            {
                fileEntry->configuration = metadata->importUserConfiguration;
                metadata->importUserConfiguration->parent(nullptr);
            }
            else
            {
                fileEntry->configuration = configClass.create();
            }
        }

        // always set the imported by stuff
        // TODO: change to VSC user name
        fileEntry->configuration->setupDefaultImportMetadata();

        // rebase user config onto the folded base config
        fileEntry->configuration->rebase(baseConfig);

        // add to internal file list and create UI model index
        m_files.pushBack(fileEntry);
        notifyItemAdded(ui::ModelIndex(), fileEntry->index);
        return fileEntry->index;
    }

    void AssetImportListModel::removeFile(ManagedFileNativeResource* managedFile)
    {
        for (auto index : m_files.indexRange())
        {
            if (m_files[index]->existingFile == managedFile)
            {
                auto file = m_files[index];
                m_files.erase(index);
                notifyItemRemoved(ui::ModelIndex(), file->index);

                break;
            }
        }
    }

    void AssetImportListModel::removeFiles(const Array<ui::ModelIndex>& indices)
    {
        Array<RefPtr<FileData>> files;
        files.reserve(indices.size());

        for (const auto index : indices)
            if (index.model() == this)
                if (auto file = index.unsafe<FileData>())
                    files.remove(file);

        notifyItemsRemoved(ui::ModelIndex(), indices);
    }

    bool AssetImportListModel::hasChildren(const ui::ModelIndex& parent) const
    {
        return !parent.valid();
    }

    ui::ModelIndex AssetImportListModel::parent(const ui::ModelIndex& item /*= ui::ModelIndex()*/) const
    {
        return ui::ModelIndex();
    }

    void AssetImportListModel::children(const ui::ModelIndex& parent, base::Array<ui::ModelIndex>& outChildrenIndices) const
    {
        if (!parent)
        {
            outChildrenIndices.reserve(m_files.size());

            for (const auto& file : m_files)
                outChildrenIndices.pushBack(file->index);
        }
    }

    static StringView GetClassDisplayName(ClassType currentClass)
    {
        auto name = currentClass.name().view();
        name = name.afterLastOrFull("::");
        return name;
    }

    static StringView StaticText(AssetImportCheckStatus status)
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
        if (item.model() == this)
        {
            if (auto file = item.unsafe<FileData>())
            {
                if (!content)
                {
                    content = RefNew<ui::IElement>();
                    content->layoutMode(ui::LayoutMode::Columns);

                    {
                        auto importFlag = content->createNamedChild<ui::CheckBox>("ImportFlag"_id);
                        importFlag->allowFocusFromKeyboard(false); // disable to avoid confusion of the highest orda
                        importFlag->bind(ui::EVENT_CLICKED) = [this, file](bool state)
                        {
                            file->importFlag = state;
                        };
                    }

                    {
                        auto fileName = content->createNamedChild<ui::TextLabel>("Status"_id);
                        fileName->customMargins(4, 0, 4, 0);
                        fileName->expand();
                    }

                    if (file->existingFile == nullptr)
                    {
                        auto fileName = content->createNamedChild<ui::EditBox>("FileName"_id);
                        fileName->customMargins(4, 0, 4, 0);
                        fileName->text(file->targetFileName);
                        fileName->validation(ui::MakeFilenameValidationFunction());
                        fileName->expand();

                        fileName->bind(ui::EVENT_TEXT_MODIFIED) = [this, file](base::StringBuf name)
                        {
                            if (file->targetFileName != name)
                            {
                                file->targetFileName = name;
                                requestItemUpdate(file->index);
                            }
                        };
                    }
                    else
                    {
                        auto fileName = content->createNamedChild<ui::TextLabel>("FileNameStatic"_id);
                        fileName->customMargins(4, 0, 4, 0);
                        fileName->text(file->targetFileName);
                    }

                    {
                        auto fileClass = content->createNamedChild<ui::TextLabel>("FileClass"_id);
                        fileClass->customMargins(4, 0, 4, 0);
                    }

                    {
                        auto sourcePath = content->createNamedChild<ui::TextLabel>("SourcePath"_id);
                        sourcePath->expand();
                        sourcePath->customMargins(4, 0, 4, 0);
                        sourcePath->text(file->sourceAssetPath);
                    }

                    {
                        auto dirPath = content->createNamedChild<ui::TextLabel>("DirPath"_id);
                        dirPath->expand();
                        dirPath->customMargins(4, 0, 4, 0);
                    }
                }

                if (auto elem = content->findChildByName<ui::CheckBox>("ImportFlag"_id))
                    elem->state(file->importFlag);

                if (auto elem = content->findChildByName<ui::TextLabel>("Status"_id))
                {
                    StringView statusString;

                    if (!ManagedItem::ValidateFileName(file->targetFileName))
                    {
                        statusString = "[tag:#A88]Invalid name[/tag]";
                    }
                    else
                    {
                        if (file->existingFile == nullptr)
                        {
                            if (fileManagedFile(file))
                                statusString = "[tag:#AA8]Overrwrite[/tag]";
                            else
                                statusString = "[tag:#8A8]New[/tag]";
                        }
                        else
                        {
                            statusString = "[tag:#888]Reimport[/tag]";
                        }
                    }

                    elem->text(statusString);
                }

                if (auto elem = content->findChildByName<ui::TextLabel>("FileClass"_id))
                    elem->text(TempString("[img:class] {}", file->targetClass));

                if (auto elem = content->findChildByName<ui::TextLabel>("DirPath"_id))
                    elem->text(file->targetDirectory->depotPath());
            }
        }
    }

    bool AssetImportListModel::compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex /*= 0*/) const
    {
        if (first.model() == this && second.model() == this)
        {
            const auto* firstFile = first.unsafe<FileData>();
            const auto* secondFile = second.unsafe<FileData>();
            if (firstFile && secondFile)
            {
                if (colIndex == 0)
                    return (int)firstFile->importFlag < (int)secondFile->importFlag;

                /*if (colIndex == 1)
                    return (int)firstFile->m_importStatus < (int)secondFile->m_importStatus;*/

                if (colIndex == 2)
                    return firstFile->targetFileName < secondFile->targetFileName;

                if (colIndex == 3)
                    return firstFile->targetClass < secondFile->targetClass;

                if (colIndex == 4)
                    return firstFile->sourceAssetPath < secondFile->sourceAssetPath;

                if (colIndex == 5)
                    return firstFile->targetDirectory < secondFile->targetDirectory;
            }
        }

        return first < second;
    }

    ui::PopupPtr AssetImportListModel::contextMenu(ui::AbstractItemView* view, const Array<ui::ModelIndex>& indices) const
    {
        auto menu = RefNew<ui::MenuButtonContainer>();

        if (indices.size() == 1)
        {
            auto rootFile = indices[0];
            if (auto* managedFile = fileManagedFile(rootFile))
            {
                menu->createCallback("Show in depot...", "[img:zoom]") = [managedFile]() { 
                    ed::GetEditor()->showFile(managedFile);
                };
            }

            if (const auto path = fileSourceAssetAbsolutePath(rootFile))
            {
                if (base::io::FileExists(path))
                {
                    menu->createCallback("Show source asset...", "[img:find_blue]") = [path]() {
                        base::io::ShowFileExplorer(path);
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
        if (id.model() == this)
        {
            if (auto* file = id.unsafe<FileData>())
            {
                if (filter.testString(file->targetFileName))
                    return true;
                if (filter.testString(file->sourceAssetPath))
                    return true;
            }
        }

        return false;
    }

    const Array<SpecificClassType<res::IResource>>& AssetImportListModel::getClassesForSourceExtension(StringView sourcePath) const
    {
        const auto ext = sourcePath.afterLast(".");
        if (const auto* list = m_classPerExtensionMap.find(ext))
            return *list;

        auto& entry = m_classPerExtensionMap[StringBuf(ext)];
        res::IResourceImporter::ListImportableResourceClassesForExtension(ext, entry);
        return entry;
    }

    ///--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetImportPrepareDialog);
    RTTI_END_TYPE();

    AssetImportPrepareDialog::AssetImportPrepareDialog()
        : ui::Window(ui::WindowFeatureFlagBit::DEFAULT_DIALOG_RESIZABLE, "Import assets into engine")
    {
        layoutVertical();

        customMaxSize(1200, 900);

        actions().bindCommand("Prepare.ClearList"_id) = [this]() { cmdClearList(); };
        actions().bindFilter("Prepare.ClearList"_id) = [this]() { return m_filesListModel->hasFiles(); };

        actions().bindCommand("Prepare.RemoveFiles"_id) = [this]() { cmdRemoveFiles(); };
        actions().bindFilter("Prepare.RemoveFiles"_id) = [this]() { return !m_fileList->selection().empty(); };

        actions().bindCommand("Prepare.StartImport"_id) = [this]() { cmdStartImport(false); };
        actions().bindFilter("Prepare.StartImport"_id) = [this]() { return m_filesListModel->hasImportableFiles(); };
        actions().bindCommand("Prepare.StartImportForced"_id) = [this]() { cmdStartImport(true); };
        actions().bindFilter("Prepare.StartImportForced"_id) = [this]() { return m_filesListModel->hasImportableFiles(); };

        m_toolbar = createChild<ui::ToolBar>();
        m_toolbar->createCallback(ui::ToolbarButtonSetup().icon("open").caption("Load list").tooltip("Load current import list from XML")) = [this]() { cmdLoadList(); };
        m_toolbar->createCallback(ui::ToolbarButtonSetup().icon("save").caption("Save list").tooltip("Save current import list to XML")) = [this]() { cmdSaveList(); };
        m_toolbar->createSeparator();
        m_toolbar->createButton("Prepare.ClearList"_id, ui::ToolbarButtonSetup().icon("delete").caption("Clear").tooltip("Clear import list"));
        m_toolbar->createSeparator();
        m_toolbar->createCallback(ui::ToolbarButtonSetup().icon("file_add").caption("Add files").tooltip("Add files to import list")) = [this]() { cmdAddFiles(); };
        m_toolbar->createButton("Prepare.RemoveFiles"_id, ui::ToolbarButtonSetup().icon("file_delete").caption("Remove files").tooltip("Remove files from import list"));
        m_toolbar->createSeparator();
        m_toolbar->createButton("Prepare.StartImport"_id, ui::ToolbarButtonSetup().icon("cog").caption("[tag:#8A9]Start[/tag]").tooltip("Start importing files"));
        m_toolbar->createButton("Prepare.StartImportForced"_id, ui::ToolbarButtonSetup().icon("cog").caption("[tag:#A89]Force[/tag]").tooltip("Force reimport all selected files"));

        //--

        //--

        auto split = createChild<ui::Splitter>(ui::Direction::Vertical, 0.7f);

        {
            auto leftPanel = split->createChild<ui::IElement>();
            leftPanel->layoutVertical();
            leftPanel->expand();

            auto filter = leftPanel->createChild<ui::SearchBar>();

            {
                auto bar = leftPanel->createChild<ui::ColumnHeaderBar>();
                bar->addColumn("", 30.0f, true, true, false);
                bar->addColumn("Status", 100.0f, true, true, false);
                bar->addColumn("File name", 200.0f, false);
                bar->addColumn("Class", 150.0f, true, true, true);
                bar->addColumn("Source path", 600.0f, false);
                bar->addColumn("Directory", 600.0f, false);
            }

            m_fileList = leftPanel->createChild<ui::ListView>();
            m_fileList->expand();
            m_fileList->columnCount(5);
            m_fileList->customInitialSize(800, 600);

            m_filesListModel = RefNew<AssetImportListModel>();
            m_fileList->model(m_filesListModel);
            filter->bindItemView(m_fileList);

            m_fileList->bind(ui::EVENT_ITEM_SELECTION_CHANGED) = [this]()
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

            m_configProperties = RefNew<ui::DataInspector>();
            m_configProperties->customStyle<StringBuf>("title"_id, "[img:properties] Config");
            m_configProperties->expand();

            m_configTabs->attachTab(m_configProperties);
        }
    }

    AssetImportPrepareDialog::~AssetImportPrepareDialog()
    {}
    
    static const TImportClassRegistry& ExtractResourceForExtension(StringView ext, TImportClassRegistryMap& registry)
    {
        if (const auto* classList = registry.find(ext))
            return *classList;

        auto& classList = registry[StringBuf(ext)];
        res::IResourceImporter::ListImportableResourceClassesForExtension(ext, classList);
        return classList;
    }

    void AssetImportPrepareDialog::addNewImportFiles(const ManagedDirectory* currentDirectory, SpecificClassType<res::IResource> resourceClass, const Array<StringBuf>& selectedAssetPaths)
    {
        static TImportClassRegistryMap ClassRegistry;

        InplaceArray<ui::ModelIndex, 20> createdIndices;
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

                    if (auto index = m_filesListModel->addNewImportFile(path, importClass, StringBuf(fileName), currentDirectory))
                    {
                        createdIndices.pushBack(index);
                    }
                }
            }
        }

        m_fileList->select(createdIndices);
        updateSelection();
    }

    void AssetImportPrepareDialog::addReimportFiles(const Array<ManagedFileNativeResource*>& files)
    {
        InplaceArray<ui::ModelIndex, 20> createdIndices;
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

    void AssetImportPrepareDialog::addReimportFile(ManagedFileNativeResource* file, const res::ResourceConfigurationPtr& reimportConfiguration)
    {
        InplaceArray<ui::ModelIndex, 20> createdIndices;
        if (file)
        {
            m_filesListModel->removeFile(file);

            if (auto index = m_filesListModel->addReimportFile(file, reimportConfiguration))
            {
                createdIndices.pushBack(index);
            }
        }

        m_fileList->select(createdIndices);
        updateSelection();
    }

    res::ImportListPtr AssetImportPrepareDialog::compileResourceList() const
    {
        return m_filesListModel->compileResourceList(true);
    }

    void AssetImportPrepareDialog::updateSelection()
    {
        auto selection = m_fileList->selection().keys();

        Array<res::ResourceConfigurationPtr> configurations;
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

    void AssetImportPrepareDialog::cmdClearList()
    {
        m_filesListModel->clearFiles();
    }

    ManagedDirectory* AssetImportPrepareDialog::contextDirectory()
    {
        return GetEditor()->selectedDirectory();
    }

    void AssetImportPrepareDialog::cmdAddFiles()
    {
        if (auto* dir = contextDirectory())
            ImportNewFiles(this, nullptr, dir);
    }

    void AssetImportPrepareDialog::cmdRemoveFiles()
    {
        auto selection = m_fileList->selection().keys();
        m_filesListModel->removeFiles(selection);

        updateSelection();
    }

    void AssetImportPrepareDialog::cmdSaveList()
    {
        GetEditor()->saveToXML(this, "AssetImportList", [this]() {
            return m_filesListModel->compileResourceList();
            });
    }

    void AssetImportPrepareDialog::cmdLoadList()
    {
        if (const auto fileList = GetEditor()->loadFromXML<res::ImportList>(this, "AssetImportList"))
        {
            m_filesListModel->clearFiles();
            addFilesFromList(*fileList);
        }
    }

    void AssetImportPrepareDialog::cmdAppendList()
    {
        if (const auto fileList = GetEditor()->loadFromXML<res::ImportList>(this, "AssetImportList"))
        {
            addFilesFromList(*fileList);
        }
    }

    void AssetImportPrepareDialog::addFilesFromList(const res::ImportList& list)
    {
        InplaceArray<ui::ModelIndex, 20> indices;

        for (const auto& entry : list.files())
        {
            if (auto* file = GetEditor()->managedDepot().findManagedFile(entry.depotPath))
            {
                if (auto* nativeFile = rtti_cast<ManagedFileNativeResource>(file))
                    if (auto index = m_filesListModel->addReimportFile(nativeFile))
                        indices.pushBack(index);
            }
            else
            {
                if (auto* directory = GetEditor()->managedDepot().findPath(entry.depotPath))
                {
                    const auto fileName = entry.depotPath.view().afterLastOrFull("/").beforeFirstOrFull(".");
                    const auto fileExt = entry.depotPath.view().afterLast(".");

                    const auto resourceClass = res::IResource::FindResourceClassByExtension(fileExt);
                    if (auto index = m_filesListModel->addNewImportFile(entry.assetPath, resourceClass, StringBuf(fileName), directory, entry.userConfiguration))
                        indices.pushBack(index);
                }
            }
        }

        if (indices.empty())
        {
            ui::PostWindowMessage(this, ui::MessageType::Warning, "ImportList"_id, TempString("No files out of {} were added to the list", list.files().size()));
        }
        else if (indices.size() != list.files().size())
        {
            ui::PostWindowMessage(this, ui::MessageType::Warning, "ImportList"_id, TempString("Added {} files (out of {}) to the list", indices.size(), list.files().size()));
        }
        else
        {
            ui::PostWindowMessage(this, ui::MessageType::Info, "ImportList"_id, TempString("All {} files added to the list", list.files().size()));
        }

        m_fileList->select(indices);
        updateSelection();
    }

    void AssetImportPrepareDialog::cmdStartImport(bool force)
    {
        if (auto files = compileResourceList())
        {
            requestClose(0);

            auto job = RefNew<AssetImportJob>(files, force);
            GetEditor()->scheduleBackgroundJob(job);
        }
    }

    ///--

} // ed