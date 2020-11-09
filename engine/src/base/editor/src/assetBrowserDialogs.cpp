/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"
#include "managedFile.h"
#include "managedDirectory.h"
#include "managedItemCollection.h"

#include "resourceEditor.h"
#include "assetBrowserDialogs.h"
#include "assetFileListSimpleModel.h"

#include "base/ui/include/uiTextLabel.h"
#include "base/ui/include/uiListView.h"
#include "base/ui/include/uiButton.h"
#include "base/io/include/ioSystem.h"

namespace ed
{

    //---

    bool SaveDepotFiles(ui::IElement* owner, const ManagedFileCollection& files)
    {
        DEBUG_CHECK_RETURN_V(owner, false);

        if (files.empty())
            return true;

        auto window = CreateSharedPtr<ui::Window>(ui::WindowFeatureFlagBit::DEFAULT_DIALOG, "Save files");
        auto windowRef = window.get();

        auto message = window->createChild<ui::TextLabel>("Following files are [b][color:#F88]modified[/color][/b] and should be saved:");
        message->customMargins(5.0f);

        auto fileList = CreateSharedPtr<AssetItemsSimpleListModel>();
        for (auto* file : files.files())
            fileList->addItem(file);

        auto listView = window->createChild<ui::ListView>();
        listView->columnCount(1);
        listView->model(fileList);
        listView->expand();
        listView->customInitialSize(500, 400);

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
                            if (auto editor = file->editor())
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
        auto window = CreateSharedPtr<ui::Window>(ui::WindowFeatureFlagBit::DEFAULT_DIALOG, "Delete files");
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

        auto fileList = CreateSharedPtr<AssetItemsSimpleListModel>();
        for (auto* file : filesToDelete)
        {
            auto canDelete = !file->inUse() && !file->editor();
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
                            if (file->inUse())
                            {
                                fileList->comment(item, "  [img:error] File is still in use");
                            }
                            else if (file->editor())
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

        window->runModal(owner);
    }

    //--

} // ed