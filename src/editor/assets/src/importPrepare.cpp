/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"

#include "browserService.h"

#include "importChecks.h"
#include "importTask.h"
#include "importPrepare.h"

#include "editor/common/include/service.h"

#include "engine/ui/include/uiImage.h"
#include "engine/ui/include/uiTextLabel.h"
#include "engine/ui/include/uiMenuBar.h"
#include "engine/ui/include/uiWindow.h"
#include "engine/ui/include/uiAbstractItemView.h"
#include "engine/ui/include/uiCheckBox.h"
#include "engine/ui/include/uiStyleValue.h"
#include "engine/ui/include/uiEditBox.h"
#include "engine/ui/include/uiComboBox.h"
#include "engine/ui/include/uiToolBar.h"
#include "engine/ui/include/uiSplitter.h"
#include "engine/ui/include/uiListViewEx.h"
#include "engine/ui/include/uiColumnHeaderBar.h"
#include "engine/ui/include/uiNotebook.h"
#include "engine/ui/include/uiDockNotebook.h"
#include "engine/ui/include/uiDataInspector.h"
#include "engine/ui/include/uiTextValidation.h"
#include "engine/ui/include/uiSearchBar.h"

#include "core/resource_compiler/include/importInterface.h"
#include "core/io/include/io.h"
#include "core/resource/include/metadata.h"
#include "core/containers/include/path.h"
#include "core/resource_compiler/include/sourceAssetService.h"
#include "core/resource/include/depot.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetImportListElement);
RTTI_END_TYPE();

AssetImportListElement::AssetImportListElement(const Setup& setup)
    : m_depotDirectoryPath(setup.depotDirectoryPath)
    , m_sourcePath(setup.sourcePath)
    , m_originalFileName(setup.fileName)
    , m_class(setup.cls)
    , m_id(setup.id)
{
    layoutColumns();

    {
        m_boxEnabled = createChild<ui::CheckBox>();
        m_boxEnabled->allowFocusFromKeyboard(false); // disable to avoid confusion of the highest orda
        m_boxEnabled->state(setup.enabled);
    }

    {
        auto fileClass = createChild<ui::TextLabel>();
        fileClass->customMargins(4, 0, 4, 0);
        fileClass->text(TempString("[img:class] {}", setup.cls));
    }

    if (setup.newFile)
    {
        m_boxName = createChild<ui::EditBox>();
        m_boxName->customMargins(4, 0, 4, 0);
        m_boxName->text(setup.fileName);
        m_boxName->validation(ui::MakeFilenameValidationFunction());
        m_boxName->expand();
    }
    else
    {
        auto fileName = createChild<ui::TextLabel>();
        fileName->customMargins(4, 0, 4, 0);
        fileName->text(setup.fileName);
    }


    {
        auto sourcePath = createChild<ui::TextLabel>();
        sourcePath->expand();
        sourcePath->customMargins(4, 0, 4, 0);
        sourcePath->text(setup.sourcePath);
    }

    /*{
        auto dirPath = createChild<ui::TextLabel>();
        dirPath->expand();
        dirPath->customMargins(4, 0, 4, 0);
        dirPath->text(setup.depotDirectoryPath);
    }*/

    buildConfig();
}

bool AssetImportListElement::enabled() const
{
    return m_boxEnabled->stateBool() && (!m_boxName || ValidateFileName(m_boxName->text()));
}

StringBuf AssetImportListElement::compileDepotPath() const
{
    if (m_boxName)
        return TempString("{}{}.{}", m_depotDirectoryPath, m_boxName->text(), IResource::FILE_EXTENSION);
    else
        return TempString("{}{}.{}", m_depotDirectoryPath, m_originalFileName, IResource::FILE_EXTENSION);
}

void AssetImportListElement::compileJob(ImportJobInfo& outJob) const
{
    outJob.id = m_id;
    outJob.assetFilePath = m_sourcePath;
    outJob.baseConfig = m_baseConfig;
    outJob.userConfig = m_userConfig;
    outJob.recurse = true;
    outJob.resourceClass = m_class;
    outJob.depotFilePath = compileDepotPath();
}

bool AssetImportListElement::handleItemFilter(const ui::ICollectionView* view, const ui::SearchPattern& filter) const
{
    if (m_boxName)
        return filter.testString(m_boxName->text());
    else
        return filter.testString(m_originalFileName);
}

void AssetImportListElement::handleItemSort(const ui::ICollectionView* view, int colIndex, SortingData& outInfo) const
{
    outInfo.index = uniqueIndex();
}

void AssetImportListElement::handleItemActivate(const ui::ICollectionView* view)
{
    StringBuf absolutePath;
    if (GetService<SourceAssetService>()->resolveContextPath(m_sourcePath, absolutePath))
        ShowFileExplorer(absolutePath);
}

void AssetImportListElement::buildConfig()
{
    // get required config class, we need the source path to know what we will be importing
    SpecificClassType<ResourceConfiguration> configClass;
    IResourceImporter::ListImportConfigurationForExtension(m_sourcePath.view().extensions(), m_class, configClass);

    // build the base import config from source asset directory
    auto mergedBaseConfig = configClass->create<ResourceConfiguration>();
    GetService<SourceAssetService>()->collectImportConfiguration(mergedBaseConfig, m_sourcePath);

    // load metadata
    const auto metadataPath = ReplaceExtension(compileDepotPath(), ResourceMetadata::FILE_EXTENSION);
    auto metadata = GetService<DepotService>()->loadFileToXMLObject<ResourceMetadata>(metadataPath);
    if (metadata)
    {
        m_baseConfig = CloneObject(metadata->importBaseConfiguration);
        if (!m_baseConfig)
        {
            m_baseConfig = configClass->create<ResourceConfiguration>();
            m_baseConfig->rebase(mergedBaseConfig);
            m_baseConfig->setupDefaultImportMetadata();
        }
        else
        {
            m_baseConfig->rebase(mergedBaseConfig);
        }

        m_userConfig = CloneObject(metadata->importUserConfiguration);
        if (!m_userConfig)
            m_userConfig = configClass->create<ResourceConfiguration>();
        m_userConfig->rebase(m_baseConfig);
    }
    else
    {
        m_baseConfig = configClass->create<ResourceConfiguration>();
        m_baseConfig->rebase(mergedBaseConfig);
        m_baseConfig->setupDefaultImportMetadata();

        m_userConfig = configClass->create<ResourceConfiguration>();
        m_userConfig->rebase(m_baseConfig);
    }

    if (!m_id)
    {
        if (metadata && !metadata->ids.empty())
            m_id = metadata->ids[0];
        else
            m_id = ResourceID::Create();
    }
}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetImportPrepareDialog);
RTTI_END_TYPE();

AssetImportPrepareDialog::AssetImportPrepareDialog()
    : ui::Window(ui::WindowFeatureFlagBit::DEFAULT_DIALOG_RESIZABLE, "Import assets into engine")
{
    layoutVertical();

    customMaxSize(1200, 900);

    m_toolbar = createChild<ui::ToolBar>();
    m_toolbar->createButton(ui::ToolBarButtonInfo().caption("Load list", "open").tooltip("Load current import list from XML"))
        = [this]() { cmdLoadList(); };
    m_toolbar->createButton(ui::ToolBarButtonInfo().caption("Save list", "save").tooltip("Save current import list to XML"))
        = [this]() { cmdSaveList(); };
    m_toolbar->createSeparator();

    m_toolbar->createButton(ui::ToolBarButtonInfo().caption("Clear", "delete").tooltip("Clear import list"))
        = [this]() { cmdClearList(); };
    m_toolbar->createSeparator();

    m_toolbar->createButton(ui::ToolBarButtonInfo().caption("[tag:#8A9]Start[/tag]", "cog").tooltip("Start importing files"))
        = [this]() { cmdStartImport(false); };
    m_toolbar->createButton(ui::ToolBarButtonInfo().caption("[tag:#A89]Force[/tag]", "cog").tooltip("Force reimport all selected files"))
        = [this]() { cmdStartImport(true); };

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
            bar->addColumn("Class", 150.0f, true, true, true);
            bar->addColumn("File name", 200.0f, false);
            bar->addColumn("Source path", 600.0f, false);
        }

        m_fileList = leftPanel->createChild<ui::ListViewEx>();
        m_fileList->expand();
        m_fileList->customInitialSize(800, 600);

        //filter->bindItemView(m_fileList);

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

static bool ShouldBeEnabledByDefault(ResourceClass resourceClass, StringView assetPath)
{
    const auto fileExtension = assetPath.extensions();
    const auto fileName = assetPath.fileStem();

    if (fileExtension.caseCmp("fbx") == 0)
    {
        if (fileName.endsWithNoCase("_phys") || fileName.endsWithNoCase("_l1") || fileName.endsWithNoCase("_l2")
            || fileName.endsWithNoCase("_l3") || fileName.endsWithNoCase("_l4") || fileName.endsWithNoCase("_l5"))
        {
            return false;
        }
    }

    return true;
}

bool AssetImportPrepareDialog::importFile(ResourceClass resourceClass, StringView depotPath, StringView sourcePath)
{
    DEBUG_CHECK_RETURN_EX_V(resourceClass, "Invalid resource class", false);
    DEBUG_CHECK_RETURN_EX_V(depotPath, "Invalid depot path", false);
    DEBUG_CHECK_RETURN_EX_V(sourcePath, "Invalid source path", false);
    DEBUG_CHECK_RETURN_EX_V(ValidateDepotDirPath(depotPath), "Invalid depot path", false);

    AssetImportListElement::Setup setup;
    setup.enabled = ShouldBeEnabledByDefault(resourceClass, sourcePath);
    setup.cls = resourceClass;
    setup.depotDirectoryPath = StringBuf(depotPath);
    setup.sourcePath = StringBuf(sourcePath);
    setup.fileName = StringBuf(sourcePath.fileStem());
    setup.newFile = true;

    // create import entry
    auto entry = RefNew<AssetImportListElement>(setup);
    m_fileList->addItem(entry);

    return true;
}

bool AssetImportPrepareDialog::reimportFile(StringView depotPath)
{
    const auto metadataPath = ReplaceExtension(depotPath, ResourceMetadata::FILE_EXTENSION);
    const auto metadata = GetService<DepotService>()->loadFileToXMLObject<ResourceMetadata>(metadataPath);
    if (!metadata)
        return false; // invalid or missing metadata

    if (!metadata->importDependencies.size())
        return false; // not imported

    if (!metadata->resourceClassType)
        return false; // unknown import class

    AssetImportListElement::Setup setup;
    setup.enabled = true;
    setup.cls = metadata->resourceClassType;
    setup.depotDirectoryPath = StringBuf(depotPath.baseDirectory());
    setup.sourcePath = metadata->importDependencies[0].importPath;
    setup.fileName = StringBuf(depotPath.fileStem());
    setup.newFile = false;

    // create import entry
    auto entry = RefNew<AssetImportListElement>(setup);
    m_fileList->addItem(entry);
    return true;
}

void AssetImportPrepareDialog::updateSelection()
{
    auto first = m_fileList->selection().first<AssetImportListElement>();
    auto config = first ? first->userConfig() : nullptr;
    m_configProperties->bindObject(config);
}

void AssetImportPrepareDialog::cmdClearList()
{

}

void AssetImportPrepareDialog::cmdSaveList()
{
    /*GetEditor()->saveToXML(this, "AssetImportList", [this]() {
        return m_filesListModel->compileResourceList();
        });*/
}

void AssetImportPrepareDialog::cmdLoadList()
{
    /*if (const auto fileList = GetEditor()->loadFromXML<ImportList>(this, "AssetImportList"))
    {
        m_filesListModel->clearFiles();
        addFilesFromList(*fileList);
    }*/
}

void AssetImportPrepareDialog::cmdAppendList()
{
    /*if (const auto fileList = GetEditor()->loadFromXML<ImportList>(this, "AssetImportList"))
    {
        addFilesFromList(*fileList);
    }*/
}

void AssetImportPrepareDialog::addFilesFromList(const ImportList& list)
{
    /*InplaceArray<ui::ModelIndex, 20> indices;

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

                const auto resourceClass = IResource::FindResourceClassByExtension(fileExt);
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
    updateSelection();*/
}

void AssetImportPrepareDialog::compileImportJobs(Array<ImportJobInfo>& outJobs) const
{
    m_fileList->iterate<AssetImportListElement>([&outJobs](const AssetImportListElement* elem)
        {
            if (elem->enabled())
                elem->compileJob(outJobs.emplaceBack());
        });
}

void AssetImportPrepareDialog::cmdStartImport(bool force)
{
    Array<ImportJobInfo> jobs;
    compileImportJobs(jobs);

    for (auto& job : jobs)
        job.force = force;
    
    if (!jobs.empty())
    {
        auto job = RefNew<AssetImportTask>(jobs);
        GetService<EditorService>()->scheduleTask(job, true);

        requestClose(0);
    }
    else
    {
        ui::PostWindowMessage(this, ui::MessageType::Warning, "ImportList"_id, "Nothing to import");
    }
}

///--

END_BOOMER_NAMESPACE_EX(ed)
