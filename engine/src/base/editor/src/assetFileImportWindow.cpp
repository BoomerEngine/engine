/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"

#include "assetFileImportWindow.h"

#include "managedDirectory.h"
#include "managedFile.h"
#include "managedFileFormat.h"
#include "managedDepot.h"

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
#include "base/resource_compiler/include/importInterface.h"
#include "editorConfig.h"

namespace ed
{

    //--

    AssetImportEntry::AssetImportEntry()
    {
    }

    //--

    AssetImportListModel::AssetImportListModel()
    {}

    AssetImportListModel::~AssetImportListModel()
    {}

    void AssetImportListModel::clear()
    {
        beingRemoveRows(ui::ModelIndex(), 0, m_files.size());
        m_files.clear();
        endRemoveRows();
    }

    void AssetImportListModel::addFile(AssetImportEntry* file)
    {
        if (!m_files.contains(file))
        {
            beingInsertRows(ui::ModelIndex(), m_files.size(), 1);
            m_files.pushBack(AddRef(file));
            endInsertRows();
        }
    }

    void AssetImportListModel::removeFile(AssetImportEntry* file)
    {
        auto index = m_files.find(file);
        if (index != -1)
        {
            beingRemoveRows(ui::ModelIndex(), index, 1);
            m_files.erase(index);
            endRemoveRows();
        }
    }

    AssetImportEntry* AssetImportListModel::findFile(const ui::ModelIndex& index) const
    {
        if (!index)
            return nullptr;

        if (index.row() >= m_files.lastValidIndex())
            return nullptr;

        return m_files[index.row()];
    }

    uint32_t AssetImportListModel::rowCount(const ui::ModelIndex& parent) const
    {
        if (!parent)
            return m_files.size();
        else
            return 0;
    }

    bool AssetImportListModel::hasChildren(const ui::ModelIndex& parent) const
    {
        if (!parent)
            return true;
        else
            return false;
    }

    bool AssetImportListModel::hasIndex(int row, int col, const ui::ModelIndex& parent /*= ui::ModelIndex()*/) const
    {
        if (!parent)
            return (col == 0) && (row >= 0) && (row <= m_files.lastValidIndex());
        else
            return false;
    }

    ui::ModelIndex AssetImportListModel::parent(const ui::ModelIndex& item /*= ui::ModelIndex()*/) const
    {
        return ui::ModelIndex();
    }

    ui::ModelIndex AssetImportListModel::index(int row, int column, const ui::ModelIndex& parent) const
    {
        if (!parent && column == 0 && (row >= 0) && (row <= m_files.lastValidIndex()))
            return ui::ModelIndex(this, row, column);

        return ui::ModelIndex();
    }

    static base::StringView<char> GetClassDisplayName(ClassType currentClass)
    {
        auto name = currentClass.name().view();
        name = name.afterLastOrFull("::");
        return name;
    }

    template< typename T >
    static void FillClassList(ui::ComboBox* box, const Array<SpecificClassType<T>>& classList, SpecificClassType<T> currentClass)
    {
        box->clearOptions();

        for (uint32_t i = 0; i < classList.size(); ++i)
        {
            const auto displayName = GetClassDisplayName(classList[i]);
            box->addOption(base::StringBuf(displayName));

            if (classList[i] == currentClass)
                box->selectOption(i);
        }
    }

    template< typename T >
    static void SelectClass(ui::ComboBox* box, SpecificClassType<T> currentClass)
    {

    }


    void AssetImportListModel::visualize(const ui::ModelIndex& item, int columnCount, ui::ElementPtr& content) const
    {
        if (auto* file = findFile(item))
        {
            const auto& importableClasses = getClassesForSourceExtension(file->assetPath);

            if (!content)
            {
                content = base::CreateSharedPtr<ui::IElement>();
                content->layoutMode(ui::LayoutMode::Columns);

                {
                    auto importFlag = content->createNamedChild<ui::CheckBox>("ImportFlag"_id);
                    importFlag->state(file->importFlag);
                }

                {
                    auto fileName = content->createNamedChild<ui::EditBox>("FileName"_id);
                    fileName->text(file->targetFileName);
                }

                {
                    auto fileClass = content->createNamedChild<ui::ComboBox>("FileClass"_id);
                    FillClassList(fileClass, importableClasses, file->targetClass);
                }

                {
                    auto sourcePath = content->createNamedChild<ui::TextLabel>("SourcePath"_id);
                    sourcePath->text(file->assetPath);
                }

                {
                    auto dirPath = content->createNamedChild<ui::TextLabel>("DirPath"_id);
                    dirPath->text(file->targetDirectory->depotPath());
                }
            }
            else
            {
                if (auto elem = content->findChildByName<ui::CheckBox>("ImportFlag"_id))
                    elem->state(file->importFlag);

                if (auto elem = content->findChildByName<ui::EditBox>("FileName"_id))
                    elem->text(file->targetFileName);

                if (auto elem = content->findChildByName<ui::ComboBox>("FileClass"_id))
                    FillClassList(elem, importableClasses, file->targetClass);

                if (auto elem = content->findChildByName<ui::EditBox>("SourcePath"_id))
                    elem->text(file->assetPath);

                if (auto elem = content->findChildByName<ui::EditBox>("DirPath"_id))
                    elem->text(file->targetDirectory->depotPath());
            }
        }
    }

    bool AssetImportListModel::compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex /*= 0*/) const
    {
        const auto* firstFile = findFile(first);
        const auto* secondFile = findFile(second);
        if (firstFile && secondFile)
        {
            if (colIndex == 0)
                return (int)firstFile->importFlag < (int)secondFile->importFlag;

            if (colIndex == 1)
                return firstFile->targetFileName < secondFile->targetFileName;

            if (colIndex == 2)
                return firstFile->targetClass < secondFile->targetClass;

            if (colIndex == 3)
                return firstFile->assetPath < secondFile->assetPath;

            if (colIndex == 4)
                return firstFile->targetDirectory < secondFile->targetDirectory;
        }

        return first < second;
    }

    bool AssetImportListModel::filter(const ui::ModelIndex& id, const ui::SearchPattern& filter, int colIndex /*= 0*/) const
    {
        if (auto* file = findFile(id))
        {
            if (filter.testString(file->targetFileName))
                return true;
            if (filter.testString(file->assetPath))
                return true;
        }

        return true;
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
        m_toolbar->createCallback(ui::ToolbarButtonSetup().icon("file_delete").caption("Add files").tooltip("Remove files from import list"), [this]() { cmdRemoveFiles(); });
        m_toolbar->createSeparator();
        m_toolbar->createCallback(ui::ToolbarButtonSetup().icon("cog").caption("Start").tooltip("Start importing files"), [this]() { cmdStartImport(); });

        auto split = createChild<ui::Splitter>(ui::Direction::Vertical, 0.7f);

        {
            auto leftPanel = split->createChild<ui::IElement>();
            leftPanel->layoutVertical();
            leftPanel->expand();

            {
                auto bar = leftPanel->createChild<ui::ColumnHeaderBar>();
                bar->addColumn("", 30.0f, true, true, false);
                bar->addColumn("File name", 220.0f, false);
                bar->addColumn("Class", 150.0f, false, true, true);
                bar->addColumn("Source path", 600.0f, false);
                bar->addColumn("Directory", 600.0f, false);
            }

            m_fileList = leftPanel->createChild<ui::ListView>();
            m_fileList->expand();
            m_fileList->columnCount(5);

            m_filesListModel = base::CreateSharedPtr<AssetImportListModel>();
            m_fileList->model(m_filesListModel);
        }

        {
            auto rightPanel = split->createChild<ui::IElement>();
            rightPanel->layoutVertical();
            rightPanel->expand();

            m_configProps = rightPanel->createChild<ui::Notebook>();
        }
    }

    AssetImportPrepareTab::~AssetImportPrepareTab()
    {}

    void AssetImportPrepareTab::addFile(const ManagedDirectory* currentDirectory, base::StringView<char> selectedAssetPath)
    {

    }

    void AssetImportPrepareTab::addFiles(const ManagedDirectory* currentDirectory, const base::Array<base::StringBuf>& selectedAssetPaths)
    {

    }

    void AssetImportPrepareTab::cmdClearList()
    {

    }

    void AssetImportPrepareTab::cmdAddFiles()
    {

    }

    void AssetImportPrepareTab::cmdRemoveFiles()
    {

    }

    void AssetImportPrepareTab::cmdSaveList()
    {

    }

    void AssetImportPrepareTab::cmdLoadList()
    {

    }

    void AssetImportPrepareTab::cmdAppendList()
    {

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

        createChild<ui::WindowTitleBar>("BoomerEditor - Asset Import", "[img:import32]");
        customMinSize(1200, 900);

        auto notebook = createChild<ui::Notebook>();
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

    void AssetImportWindow::addFile(const ManagedDirectory* currentDirectory, base::StringView<char> selectedAssetPath)
    {
        m_prepareTab->addFile(currentDirectory, selectedAssetPath);
    }

    void AssetImportWindow::addFiles(const ManagedDirectory* currentDirectory, const base::Array<base::StringBuf>& selectedAssetPaths)
    {
        m_prepareTab->addFiles(currentDirectory, selectedAssetPaths);
    }

    //--

} // ed