/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"
#include "editorService.h"

#include "managedDepot.h"
#include "managedFile.h"
#include "managedDirectory.h"
#include "managedItemCollection.h"

#include "resourceEditor.h"
#include "assetBrowserDialogs.h"
#include "assetFileListSimpleModel.h"

#include "base/ui/include/uiTextLabel.h"
#include "base/ui/include/uiListView.h"
#include "base/ui/include/uiSearchBar.h"
#include "base/ui/include/uiButton.h"
#include "base/ui/include/uiColumnHeaderBar.h"
#include "base/io/include/ioSystem.h"
#include "base/ui/include/uiEditBox.h"
#include "base/ui/include/uiCheckBox.h"
#include "base/ui/include/uiProgressBar.h"
#include "base/ui/include/uiTextValidation.h"
//#include "base/resource_compiler/include/depotStructureRename.h"
#include "base/ui/include/uiMessageBox.h"
#include "base/system/include/thread.h"

BEGIN_BOOMER_NAMESPACE(ed)

//---

bool SaveDepotFiles(ui::IElement* owner, const ManagedFileCollection& files)
{
    DEBUG_CHECK_RETURN_V(owner, false);

    if (files.empty())
        return true;

    auto window = RefNew<ui::Window>(ui::WindowFeatureFlagBit::DEFAULT_DIALOG, "Save files");
    auto windowRef = window.get();

    auto message = window->createChild<ui::TextLabel>("Following files are [b][color:#F88]modified[/color][/b] and should be saved:");
    message->customMargins(5.0f);

    auto fileList = RefNew<AssetItemsSimpleListModel>();
    for (auto* file : files.files())
        fileList->addItem(file);

    auto searchBar = window->createChild<ui::SearchBar>();

    auto listView = window->createChild<ui::ListView>();
    listView->columnCount(1);
    listView->model(fileList);
    listView->expand();
    listView->customInitialSize(500, 400);

    searchBar->bindItemView(listView);

    auto buttons = window->createChild<ui::IElement>();
    buttons->layoutHorizontal();
    buttons->customPadding(5);
    buttons->customHorizontalAligment(ui::ElementHorizontalLayout::Right);

    {
        window->actions().bindShortcut("ToggleItems"_id, "Space");
        window->actions().bindCommand("ToggleItems"_id) = [fileList, listView]()
        {
            for (const auto& index : listView->selection())
                fileList->checked(index, !fileList->checked(index));
        };
    }

    {
        window->actions().bindShortcut("Cancel"_id, "Escape");
        window->actions().bindCommand("Cancel"_id) = [windowRef]()
        {
            windowRef->requestClose(0);
        };
    }

    {
        auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "[img:disk] Save");
        button->addStyleClass("green"_id);

        button->bind(ui::EVENT_CLICKED) = [fileList, windowRef]()
        {
            auto listToSave = fileList->exportList();

            for (const auto& item : listToSave)
            {
                bool saved = true;
                if (auto file = rtti_cast<ManagedFile>(item))
                {
                    if (file->isModified())
                    {
                        if (auto editor = GetEditor()->findFileEditor(file))
                        {
                            if (!editor->save())
                            {
                                fileList->comment(item, "  [img:error] Unable to save file");
                                saved = false;
                            }
                        }
                    }
                    else
                    {
                        saved = true;
                    }
                }

                if (saved)
                    fileList->removeItem(item);
            }

            if (fileList->empty())
                windowRef->requestClose(1);
        };
    }

    {
        auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "[img:delete] Don't save");
        button->addStyleClass("red"_id);

        button->bind(ui::EVENT_CLICKED) = [windowRef]() {
            windowRef->requestClose(1);
        };
    }

    {
        auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "Cancel");
        button->bind(ui::EVENT_CLICKED) = [windowRef]()
        {
            windowRef->requestClose(0);
        };
    }

    return window->runModal(owner);
}

//---

// from the initial untyped item list generate list of files and directories (recursive)
static void CollectFilesAndDirs(const Array<ManagedItem*>& items, Array<ManagedFile*>& outFiles, Array<ManagedDirectory*>& outDirs)
{
    ManagedDirectory::CollectionContext context;
    ManagedFileCollection finalFiles;
    for (auto* item : items)
    {
        if (auto file = rtti_cast<ManagedFile>(item))
        {
            finalFiles.collectFile(file);
        }
        else if (auto dir = rtti_cast<ManagedDirectory>(item))
        {
            dir->collectFiles(context, true, finalFiles);
        }
    }

    outFiles = finalFiles.files();
    outDirs = (Array<ManagedDirectory*>&) context.visitedDirectories.keys();
}

//---

void DeleteDepotFiles(ui::IElement* owner, const ManagedFileCollection& assetItems)
{
    DeleteDepotItems(owner, assetItems.items());
}

void DeleteDepotItems(ui::IElement* owner, const Array<ManagedItem*>& items)
{
    auto window = RefNew<ui::Window>(ui::WindowFeatureFlagBit::DEFAULT_DIALOG, "Delete files");
    auto windowRef = window.get();

    // get the actual list of files and directories to delete
    Array<ManagedFile*> filesToDelete;
    Array<ManagedDirectory*> dirsToDelete;
    CollectFilesAndDirs(items, filesToDelete, dirsToDelete);

    {
        auto elem = window->createChild<ui::TextLabel>();
        elem->customMargins(4, 4, 4, 4);

        StringBuilder txt;
        txt << "Following";
        if (filesToDelete.size())
            txt.appendf(" {} file{}", filesToDelete.size(), filesToDelete.size() == 1 ? "" : "s");
        if (dirsToDelete.size() && filesToDelete.size())
            txt << " and";
        if (dirsToDelete.size())
            txt.appendf(" {} director{}", dirsToDelete.size(), dirsToDelete.size() == 1 ? "y" : "ies");
        txt << " will be ";

        txt << "[b][color:#F33]";
        txt << "PERMAMENTLY DELETED!";
        txt << "[/color][/b]";

        elem->text(txt.view());
    }

    auto fileList = RefNew<AssetItemsSimpleListModel>();
    for (auto* file : filesToDelete)
    {
        auto canDelete = !GetEditor()->findFileEditor(file); // we can only delete files that are not in use
        fileList->addItem(file, canDelete);
    }

    for (auto* dir : dirsToDelete)
    {
        auto canDelete = !dir->isBookmarked();
        fileList->addItem(dir, canDelete);
    }

    auto listView = window->createChild<ui::ListView>();
    listView->columnCount(1);
    listView->model(fileList);
    listView->expand();
    listView->customInitialSize(500, 400);

    {
        window->actions().bindShortcut("ToggleItems"_id, "Space");
        window->actions().bindCommand("ToggleItems"_id) = [fileList, listView]()
        {
            for (const auto& index : listView->selection())
                fileList->checked(index, !fileList->checked(index));
        };
    }

    {
        window->actions().bindShortcut("Cancel"_id, "Escape");
        window->actions().bindCommand("Cancel"_id) = [windowRef]()
        {
            windowRef->requestClose(0);
        };
    }

    auto buttons = window->createChild<ui::IElement>();
    buttons->layoutHorizontal();
    buttons->customPadding(5);
    buttons->customHorizontalAligment(ui::ElementHorizontalLayout::Right);

    {
        auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "[img:delete] Delete");
        button->addStyleClass("red"_id);
        button->bind(ui::EVENT_CLICKED) = [fileList, windowRef]()
        {
            auto listToDelete = fileList->exportList();

            for (const auto& item : listToDelete)
            {
                bool deleted = item->isDeleted();

                if (item->absolutePath().empty())
                {
                    fileList->comment(item, "  [img:error] Not a physical entry on disk");
                    deleted = false;
                }
                else
                {
                    if (auto file = rtti_cast<ManagedFile>(item))
                    {
                        if (GetEditor()->findFileEditor(file))
                        {
                            fileList->comment(item, "  [img:error] File is opened in editor");
                        }
                        else if (!base::io::DeleteFile(file->absolutePath()))
                        {
                            fileList->comment(item, "  [img:error] Failed to delete physical file");
                        }
                        else
                        {
                            deleted = true;
                        }
                    }
                    else if (auto dir = rtti_cast<ManagedDirectory>(item))
                    {
                        if (dir->isBookmarked())
                        {
                            fileList->comment(item, "  [img:error] Directory is bookmarked");
                        }
                        else if (!base::io::DeleteDir(dir->absolutePath()))
                        {
                            fileList->comment(item, "  [img:error] Failed to delete physical directory");
                        }
                        else
                        {
                            deleted = true;
                        }
                    }
                }

                if (deleted)
                    fileList->removeItem(item);
            }

            if (fileList->empty())
                windowRef->requestClose();
        };
    }

    {
        auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "Cancel");
        button->bind(ui::EVENT_CLICKED) = [windowRef]() {
            windowRef->requestClose();
        };
    }

    window->runModal(owner, listView);
}

//--

ConfigProperty<base::Array<ui::SearchPattern>> cvFileListSearchFilter("Editor", "FileListSearchFilter", {});

void ShowOpenedFilesList(ui::IElement* owner, ManagedFile* focusFile)
{
    auto window = RefNew<ui::Window>(ui::WindowFeatureFlagBit::DEFAULT_DIALOG, "Opened files");
    auto windowRef = window.get();

    window->actions().bindCommand("Cancel"_id) = [windowRef]() { windowRef->requestClose(); };
    window->actions().bindShortcut("Cancel"_id, "Escape");

    auto& depot = GetEditor()->managedDepot();

    auto fileList = RefNew<AssetPlainFilesSimpleListModel>();

    InplaceArray<ResourceEditorPtr, 100> openedEditors;
    GetEditor()->collectResourceEditors(openedEditors);

    for (const auto& editor : openedEditors)
        fileList->addFile(editor->file(), editor->tabLocked());

    {
        auto bar = window->createChild<ui::ColumnHeaderBar>();
        bar->addColumn("", 30.0f, true, true, false);
        bar->addColumn("", 30.0f, true, true, false);
        bar->addColumn("Tags", 220.0f, false, false, true);
        bar->addColumn("Name", 150.0f, false, true, true);
        bar->addColumn("Directory", 600.0f, false, true, true);
    }

    auto searchBar = window->createChild<ui::SearchBar>();
    searchBar->loadHistory(cvFileListSearchFilter.get());

    auto listView = window->createChild<ui::ListView>();
    listView->customPadding(10, 0, 10, 0);
    listView->customInitialSize(700, 800);
    listView->sort(0);
    listView->expand();
    listView->columnCount(5);
    listView->model(fileList);

    searchBar->bindItemView(listView);

    listView->bind(ui::EVENT_ITEM_ACTIVATED) = [windowRef, listView, fileList]() {
        if (auto item = listView->current())
            if (auto file = fileList->file(item))
                if (GetEditor()->showFileEditor(file))
                    windowRef->requestClose();
    };

    if (focusFile)
    {
        if (auto item = fileList->index(focusFile))
        {
            listView->select(item);
            listView->ensureVisible(item);
        }
    }

    //--

    auto buttons = window->createChild<ui::IElement>();
    buttons->layoutHorizontal();
    buttons->customPadding(5);
    buttons->customHorizontalAligment(ui::ElementHorizontalLayout::Right);

    {
        auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "[img:cancel] Close all (discard)");
        button->addStyleClass("red"_id);
        button->bind(ui::EVENT_CLICKED) = [windowRef]() {
            //windowRef->requestClose();
        };
    }

    {
        auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "[img:cancel] Close not modified");
        button->bind(ui::EVENT_CLICKED) = [windowRef]() {
            //windowRef->requestClose();
        };
    }

    {
        auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "Done");
        button->bind(ui::EVENT_CLICKED) = [windowRef, searchBar]() {
            searchBar->saveHistory(cvFileListSearchFilter.get());
            windowRef->requestClose();
        };
    }

    window->runModal(owner, searchBar);
}

//--

bool ShowSaveAsFileDialog(ui::IElement* owner, ManagedDirectory* specificDirectory, ClassType resourceClass, StringView message, StringView initialFileName, StringBuf& outDepotPath)
{
    auto window = base::RefNew<ui::Window>(ui::WindowFeatureFlagBit::DEFAULT_DIALOG, "Save as...");
    window->layoutVertical();

    auto windowRef = window.get();
    window->actions().bindCommand("Cancel"_id) = [windowRef]() { windowRef->requestClose(0); };
    window->actions().bindShortcut("Cancel"_id, "Escape");

    if (!message.empty())
    {
        auto label = window->createChild<ui::TextLabel>(message);
        label->customMargins(5, 5, 5, 0);
    }

    auto dirBar = window->createChild<ui::IElement>();
    dirBar->layoutHorizontal();
    dirBar->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
    dirBar->customMargins(5, 5, 5, 0);

    if (!specificDirectory)
        specificDirectory = GetEditor()->selectedDirectory();
    if (!specificDirectory)
        specificDirectory = GetEditor()->managedDepot().root();

    auto editBoxFlags = ui::EditBoxFeatureBit::AcceptsEnter;

    auto dirName = dirBar->createChild<ui::EditBox>(editBoxFlags).get();
    dirName->text(specificDirectory->depotPath());
    dirName->validation(ui::MakeDirectoryValidationFunction(false));
    dirName->expand();

    auto dirPick = dirBar->createChild<ui::Button>("[img:page_zoom] Pick");
    dirPick->styleType("BackgroundButton"_id);
    dirPick->customVerticalAligment(ui::ElementVerticalLayout::Middle);
    dirPick->customMargins(ui::Offsets(5, 0, 5, 0));

    auto editText = window->createChild<ui::EditBox>(editBoxFlags).get();
    editText->customInitialSize(500, 20);
    editText->customMargins(5, 5, 5, 5);
    editText->text(initialFileName);
    editText->validation(ui::MakeFilenameValidationFunction(false));
    editText->expand();

    auto buttons = window->createChild();
    buttons->layoutHorizontal();
    buttons->customHorizontalAligment(ui::ElementHorizontalLayout::Right);

    {
        auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "[img:save] Save").get();

        auto checkFunc = [editText, resourceClass, dirName, button]()
        {
            bool valid = false;

            if (editText->validationResult() && dirName->validationResult())
            {
                auto extension = res::IResource::GetResourceExtensionForClass(resourceClass);
                auto depotPath = StringBuf(TempString("{}{}.{}", dirName->text(), editText->text(), extension));
                if (ValidateDepotPath(depotPath))
                {
                    const auto* existingFile = GetEditor()->managedDepot().findManagedFile(depotPath);
                    if (existingFile == nullptr)
                        valid = true;
                }
            }

            button->enable(valid);
        };

        auto acceptFunc = [windowRef, editText, resourceClass, dirName, button, &outDepotPath]() {
            if (button->isEnabled() && editText->validationResult() && dirName->validationResult()) {
                auto extension = res::IResource::GetResourceExtensionForClass(resourceClass);
                auto depotPath = StringBuf(TempString("{}{}.{}", dirName->text(), editText->text(), extension));
                if (ValidateDepotPath(depotPath))
                {
                    outDepotPath = depotPath;
                    windowRef->requestClose(1);
                }
            }
        };

        editText->bind(ui::EVENT_TEXT_MODIFIED) = checkFunc;
        dirName->bind(ui::EVENT_TEXT_MODIFIED) = checkFunc;

        dirName->bind(ui::EVENT_TEXT_ACCEPTED) = [editText]() {
            editText->focus();
        };

        editText->bind(ui::EVENT_TEXT_ACCEPTED) = acceptFunc;

        button->bind(ui::EVENT_CLICKED) = acceptFunc;

        checkFunc();
    }

    {
        auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "Cancel");
        button->bind(ui::EVENT_CLICKED) = [windowRef]() {
            windowRef->requestClose(0);
        };
    }

    return window->runModal(owner, editText);
}

//--

bool ShowGenericRenameDialog(ui::IElement* owner, const ManagedItem* item, base::res::RenameConfiguration& outSettings)
{
    /*auto* file = rtti_cast<ManagedFile>(item);
    auto* dir = rtti_cast<ManagedDirectory>(item);

    auto window = base::RefNew<ui::Window>(ui::WindowFeatureFlagBit::DEFAULT_DIALOG, file ? "Rename file" : "Rename directory");
    window->layoutVertical();
    window->customPadding(5, 5, 5, 5);

    auto windowRef = window.get();
    window->actions().bindCommand("Cancel"_id) = [windowRef]() { windowRef->requestClose(0); };
    window->actions().bindShortcut("Cancel"_id, "Escape");

    auto label = window->createChild<ui::TextLabel>(file ? "Please enter new name of the file:" : "Please enter new name of the directory:");
    label->customMargins(0, 0, 0, 5);

    auto dirBar = window->createChild<ui::IElement>();
    dirBar->layoutHorizontal();
    dirBar->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
    label->customMargins(0, 0, 0, 5);

    auto* specificDirectory = item->parentDirectory();
    auto editBoxFlags = ui::EditBoxFeatureBit::AcceptsEnter;

    auto dirName = dirBar->createChild<ui::EditBox>(editBoxFlags).get();
    dirName->text(specificDirectory->depotPath());
    dirName->validation(ui::MakeDirectoryValidationFunction(false));
    dirName->expand();

    auto dirPick = dirBar->createChild<ui::Button>("[img:page_zoom] Pick");
    dirPick->styleType("BackgroundButton"_id);
    dirPick->customVerticalAligment(ui::ElementVerticalLayout::Middle);

    auto editText = window->createChild<ui::EditBox>(editBoxFlags).get();
    editText->customInitialSize(500, 20);
    editText->customMargins(0, 0, 5, 0);

    StringView fileExtension;
    if (file)
    {
        fileExtension = file->name().view().afterFirst(".");
        editText->text(file->name().view().beforeFirst("."));
    }
    else if (dir)
    {
        editText->text(dir->name().view());
    }
    editText->validation(ui::MakeFilenameValidationFunction(false));
    editText->expand();

    if (dir != nullptr)
    {
        auto line = window->createChild<ui::IElement>();
        line->layoutHorizontal();
        line->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        line->customMargins(0, 0, 0, 5);

        auto check = line->createChild<ui::CheckBox>(outSettings.fixupInternalReferences);
        check->bind(ui::EVENT_CLICKED) = [windowRef, &outSettings](ui::CheckBox* box) { outSettings.fixupInternalReferences = box->stateBool(); };

        auto label = line->createChild<ui::TextLabel>("Fix internal references");
        label->tooltip("Change paths inside moved/copied files to point to new file locations");
    }

    {
        auto line = window->createChild<ui::IElement>();
        line->layoutHorizontal();
        line->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        line->customMargins(0, 0, 0, 5);

        auto check = line->createChild<ui::CheckBox>(outSettings.applyLinks);
        check->bind(ui::EVENT_CLICKED) = [windowRef, &outSettings](ui::CheckBox* box) { outSettings.applyLinks = box->stateBool(); };

        auto label = line->createChild<ui::TextLabel>("Apply existing links");
        label->tooltip("When applying new paths to a files make sure all existing links (thumbstones) are applied as well.");
    }

    if (outSettings.moveFilesNotCopy)
    {
        {
            auto line = window->createChild<ui::IElement>();
            line->layoutHorizontal();
            line->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            line->customMargins(0, 0, 0, 5);

            auto check = line->createChild<ui::CheckBox>(outSettings.createThumbstones);
            check->bind(ui::EVENT_CLICKED) = [windowRef, &outSettings](ui::CheckBox* box) { outSettings.createThumbstones = box->stateBool(); };

            auto label = line->createChild<ui::TextLabel>("Create thumbstones");
            label->tooltip("Create thumbstones (links files) that will point to the new location of the files.[br]This will prevent moved directoreis from being deleted.");
        }

        {
            auto line = window->createChild<ui::IElement>();
            line->layoutHorizontal();
            line->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            line->customMargins(0, 0, 0, 5);

            auto check = line->createChild<ui::CheckBox>(outSettings.removeEmptyDirectories);
            check->bind(ui::EVENT_CLICKED) = [windowRef, &outSettings](ui::CheckBox* box) { outSettings.removeEmptyDirectories = box->stateBool(); };

            auto label = line->createChild<ui::TextLabel>("Remove empty directories");
            label->tooltip("After moving is done cleanup directories that are empty");
        }
    }

    auto buttons = window->createChild();
    buttons->layoutHorizontal();
    buttons->customHorizontalAligment(ui::ElementHorizontalLayout::Right);
    buttons->customMargins(0, 0, 0, 5);

    {
        auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "[img:file_go] Rename").get();

        auto checkFunc = [editText, fileExtension, dirName, button, &outSettings]()
        {
            bool valid = false;

            if (editText->validationResult() && dirName->validationResult())
            {
                if (fileExtension.empty())
                {
                    auto depotPath = StringBuf(TempString("{}{}/", dirName->text(), editText->text()));
                    if (ValidateDepotPath(depotPath, DepotPathClass::AbsoluteDirectoryPath))
                    {
                        const auto* existingDir = GetEditor()->managedDepot().findPath(depotPath);
                        if (existingDir == nullptr)
                            valid = true; // we can't rename onto existing file
                    }
                }
                else
                {
                    auto depotPath = StringBuf(TempString("{}{}.{}", dirName->text(), editText->text(), fileExtension));
                    if (ValidateDepotPath(depotPath, DepotPathClass::AbsoluteFilePath))
                    {
                        const auto* existingFile = GetEditor()->managedDepot().findManagedFile(depotPath);
                        if (existingFile == nullptr)
                            valid = true; // we can't rename onto existing file
                    }
                }
            }

            button->enable(valid);
        };

        auto acceptFunc = [windowRef, editText, fileExtension, dirName, item, button, &outSettings]() {
            if (button->isEnabled() && editText->validationResult() && dirName->validationResult()) {

                outSettings.elements.reset();

                if (fileExtension.empty())
                {
                    auto depotPath = StringBuf(TempString("{}{}/", dirName->text(), editText->text()));
                    if (ValidateDepotPath(depotPath, DepotPathClass::AbsoluteDirectoryPath))
                    {
                        const auto* existingDir = GetEditor()->managedDepot().findPath(depotPath);
                        if (existingDir == nullptr)
                        {
                            auto& entry = outSettings.elements.emplaceBack();
                            entry.file = false;
                            entry.sourceDepotPath = item->depotPath();
                            entry.renamedDepotPath = depotPath;
                        }
                    }
                }
                else
                {
                    auto depotPath = StringBuf(TempString("{}{}.{}", dirName->text(), editText->text(), fileExtension));
                    if (ValidateDepotPath(depotPath, DepotPathClass::AbsoluteFilePath))
                    {
                        const auto* existingFile = GetEditor()->managedDepot().findManagedFile(depotPath);
                        if (existingFile == nullptr)
                        {
                            auto& entry = outSettings.elements.emplaceBack();
                            entry.file = true;
                            entry.sourceDepotPath = item->depotPath();
                            entry.renamedDepotPath = depotPath;
                        }
                    }
                }


                if (!outSettings.elements.empty())
                    windowRef->requestClose(1);
            }
        };

        editText->bind(ui::EVENT_TEXT_MODIFIED) = checkFunc;
        dirName->bind(ui::EVENT_TEXT_MODIFIED) = checkFunc;

        dirName->bind(ui::EVENT_TEXT_ACCEPTED) = [editText]() {
            editText->focus();
        };

        editText->bind(ui::EVENT_TEXT_ACCEPTED) = acceptFunc;

        button->bind(ui::EVENT_CLICKED) = acceptFunc;

        checkFunc();
    }

    {
        auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "Cancel");
        button->bind(ui::EVENT_CLICKED) = [windowRef]() {
            windowRef->requestClose(0);
        };
    }

    return window->runModal(owner, editText);*/
    return false;
}

//--

/*struct RenameFileListModelEntry
{
    StringBuf sourceDepotPath;
    const depot::FileRenameJob* job = nullptr;

    bool finishedOK = false;
    bool finishedFailed = false;
};

class RenameFileListModel : public ui::SimpleTypedListModel<RenameFileListModelEntry>
{
public:
    virtual bool compare(const RenameFileListModelEntry& a, const RenameFileListModelEntry& b, int colIndex) const override
    {
        return a.sourceDepotPath < b.sourceDepotPath;
    }

    virtual bool filter(const RenameFileListModelEntry& data, const ui::SearchPattern& filter, int colIndex) const override
    {
        return filter.testString(data.sourceDepotPath);
    }

    virtual base::StringBuf content(const RenameFileListModelEntry& data, int colIndex) const override
    {
        StringBuilder txt;

        txt << "[img:file] ";

        if (!data.job)
            txt << "[tag:#A88]Invalid[/tag]";
        else if (data.job->deleteOriginalFile)
            txt << "[tag:#AA8]Move[/tag]";
        else
            txt << "[tag:#8A8]Copy[/tag]";

        if (data.job && !data.job->pathChanges.empty())
            txt << "[tag:#888]Update Paths[/tag]";

        txt << " ";
        txt << data.sourceDepotPath;

        return txt.toString();
    }
};*/

/*class InspectRenameDialog : public ui::Window
{
public:        
    InspectRenameDialog()
        : ui::Window(ui::WindowFeatureFlagBit::DEFAULT_DIALOG, "Inspect rename/move")
    {
        customPadding(5, 5, 5, 5);

        createChild<ui::TextLabel>("Inspect files to rename:");

        m_actionListModel = RefNew<RenameFileListModel>();
        m_actionList = createChild<ui::ListView>();
        m_actionList->columnCount(1);
        m_actionList->model(m_actionListModel);
        m_actionList->expand();
        m_actionList->customInitialSize(1200, 900);
        m_actionList->customMinSize(1200, 900);            

        m_actionList->bind(ui::EVENT_ITEM_SELECTION_CHANGED) = [this]()
        {
            //auto selection = 
        };

        auto buttons = createChild();
        buttons->customMargins(0, 5, 0, 5);
        buttons->layoutHorizontal();
        buttons->customHorizontalAligment(ui::ElementHorizontalLayout::Right);

        {
            auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "[img:rename] Rename");
            button->addStyleClass("red"_id);
            button->bind(ui::EVENT_CLICKED) = [this]() {
                requestClose(1);
            };
        }

        {
            auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "Cancel");
            button->bind(ui::EVENT_CLICKED) = [this]() {
                requestClose(0);
            };
        }
    }

    void addData(const Array<depot::FileRenameJob*>& jobs, const Array<StringBuf>& failedFiles)
    {
        for (const auto& path : failedFiles)
        {
            RenameFileListModelEntry entry;
            entry.sourceDepotPath = path;
            m_actionListModel->add(entry);
        }

        for (const auto* job : jobs)
        {
            RenameFileListModelEntry entry;
            entry.job = job;
            entry.sourceDepotPath = job->sourceDepotPath;
            m_actionListModel->add(entry);
        }
    }

    void showDetails(const RenameFileListModelEntry& entry)
    {
    }

protected:
    ui::ListViewPtr m_actionList;
    RefPtr<RenameFileListModel> m_actionListModel;

    ui::ProgressBarPtr m_progressBar;
    ui::TextLabelPtr m_progressText;

    ui::ButtonPtr m_closeButton;

    std::atomic<bool> m_canceFlag = false;
};*/

//--

ManagedItem* RenameItem(ui::IElement* owner, ManagedItem* file)
{
    auto& depot = GetEditor()->managedDepot().depot();

    GetEditor()->runLongAction(owner, nullptr, [](IProgressTracker& progress) {
            const auto size = 20;
            for (uint32_t i = 0; i < size; ++i) {
                if (progress.checkCancelation())
                    break;
                progress.reportProgress(i, size, TempString("Waiting {}/{}", i, size));
                base::Sleep(500);
            }
        }, "Waiting for rename...", false);

/*        base::depot::RenameConfiguration settings;
    settings.moveFilesNotCopy = true;

    // user input
    if (!ShowGenericRenameDialog(owner, file, settings))
        return nullptr;

    // scan files to rename
    Array<depot::FileRenameJob*> jobs;
    Array<StringBuf> failedFiles;
    GetEditor()->runLongAction(owner, nullptr, [&jobs, &failedFiles, &settings, &depot](IProgressTracker& progress) {
        CollectRenameJobs(depot, settings, progress, jobs, failedFiles);
        }, "Collecting files to rename", true);

    // nothing scanned
    if (jobs.empty())
    {
        ui::ShowMessageBox(owner, ui::MessageBoxSetup().title("Rename").message("There's nothing to rename"));
        return nullptr;
    }

    // confirm
    {
        auto dlg = RefNew<InspectRenameDialog>();
        dlg->addData(jobs, failedFiles);
        if (!dlg->runModal(owner))
            return nullptr;
    }


    // select the renamed element (only if we renamed as single one)
    if (!settings.elements.empty())
    {
        const auto& elem = settings.elements.back();
        if (elem.file)
            return GetEditor()->managedDepot().findManagedFile(elem.renamedDepotPath);
        else
            return GetEditor()->managedDepot().findPath(elem.renamedDepotPath);
    }*/

    return nullptr;
}

bool MoveItems(ui::IElement* owner, const Array<ManagedItem*>& items, ManagedDirectory* target)
{
    return false;
}

bool CopyItems(ui::IElement* owner, const Array<ManagedItem*>& items, ManagedDirectory* target)
{
    return false;
}

//--

END_BOOMER_NAMESPACE(ed)