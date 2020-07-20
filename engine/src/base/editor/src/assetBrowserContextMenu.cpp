/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"
#include "assetBrowser.h"
#include "assetBrowserContextMenu.h"
#include "assetBrowserTabFiles.h"
#include "assetFileListModel.h"

#include "editorService.h"
#include "editorWindow.h"
#include "managedDirectory.h"
#include "managedFile.h"
#include "managedFileFormat.h"

#include "base/ui/include/uiMessageBox.h"
#include "base/ui/include/uiInputBox.h"

#include "base/containers/include/clipboard.h"
#include "base/image/include/image.h"
#include "base/io/include/ioSystem.h"
#include "base/ui/include/uiMenuBar.h"
#include "base/ui/include/uiTextLabel.h"
#include "base/ui/include/uiButton.h"
#include "base/ui/include/uiListView.h"

namespace ed
{

    ///---

    struct DepotContext : public IReferencable
    {
        DepotContext(AssetBrowserTabFiles* tab, const Array<ManagedItem*>& items)
            : m_tab(tab)
            , m_items(items)
        {
            m_files.reserve(items.size());
            m_dirs.reserve(items.size());

            for (auto& item : items)
            {
                if (auto dir = rtti_cast<ManagedDirectory>(item))
                    m_dirs.emplaceBack(std::move(dir));
                else if (auto file = rtti_cast<ManagedFile>(item))
                    m_files.emplaceBack(std::move(file));
            }

            for (auto& file : m_files)
            {
                if (!file->absolutePath().empty())
                {
                    m_hasFilesWithAbsolutePath = true;
                    m_hasVersionedFiles = false; // TODO
                }
            }

            m_isPhysicalDirectory = tab && tab->directory() && !tab->directory()->absolutePath().empty();
            m_hasStuffToPaste = false;// m_fileClipboard && !m_fileClipboard->m_entries.empty();
        }

        RefWeakPtr<AssetBrowserTabFiles> m_tab;
        Array<ManagedItem*> m_items;
        Array<ManagedFile*> m_files;
        Array<ManagedDirectory*> m_dirs;

        bool m_hasFilesWithAbsolutePath = false;
        bool m_hasVersionedFiles = false;
        bool m_isPhysicalDirectory = false;
        bool m_hasStuffToPaste = false;

        void cmdAddDirectory()
        {}

        void cmdAddFile()
        {}

        void cmdOpen()
        {
            Array<ManagedItem*> failedToOpen;

            for (const auto& file : m_files)
                if (!GetService<Editor>()->openFile(file))
                    failedToOpen.pushBack(file);

            if (!failedToOpen.empty())
            {
                if (auto tab = m_tab.lock())
                {
                    StringBuilder txt;
                    txt << "No editor found capable of editing following";

                    if (failedToOpen.size() == 1)
                        txt << "file:\n";
                    else
                        txt << failedToOpen.size() << " files:\n";

                    uint32_t printCount = 10;
                    for (const auto& file : failedToOpen)
                    {
                        if (!printCount)
                        {
                            txt << "   And more...";
                            break;
                        }
                        txt << "   [img:page][i]" << file->depotPath() << "\n";
                        printCount -= 1;
                    }

                    ui::PostWindowMessage(tab, ui::MessageType::Error, "EditAsset"_id, TempString("Failed to open files:[br]{}", txt.c_str()));
                    //ui::ShowMessageBox(tab, ui::MessageBoxSetup().error().title("Edit asset").message(txt.c_str()).error());
                }
            }
        }

        void cmdDuplicate()
        {}

        void cmdCopy()
        {}

        void cmdCut()
        {}

        void cmdDelete()
        {
            if (auto tab = m_tab.lock())
            {
                AssetItemList assetItems;
                for (const auto& item : m_items)
                    assetItems.collect(item);

                DeleteFiles(tab, assetItems);
            }
        }

        void cmdPaste()
        {}

        void cmdSccCheckin()
        {}

        void cmdSccCheckout()
        {}

        void cmdSccGetLatest()
        {}

        void cmdSccShowHistory()
        {}

        void cmdSccRevert()
        {}

        void cmdShowInFiles()
        {
            if (!m_items.empty())
            {
                auto path = m_items.front()->absolutePath();
                IO::GetInstance().showFileExplorer(path);
            }
        }

        void cmdCopyDepotPath()
        {
            StringBuilder txt;

            for (auto& item : m_items)
            {
                auto path = item->depotPath();
                if (path.empty())
                    continue;

                if (!txt.empty()) txt.append("\n");
                txt.append(path.c_str());
            }

            if (!txt.empty())
            {
                ClipboardData data(txt.toString());
                //m_context->windowRenderer()->clipboardData(&data, 1);
            }
        }

        void cmdCopyAbsolutePath()
        {
            StringBuilder txt;

            for (auto& item : m_items)
            {
                auto path = item->absolutePath();
                if (path.empty())
                    continue;

                if (!txt.empty()) txt.append("\n");
                txt.append(path.ansi_str().c_str());
            }

            if (!txt.empty())
            {
                ClipboardData data(txt.toString());
                //m_context->windowRenderer()->clipboardData(&data, 1);
            }
        }
    };

    void BuildDepotContextMenu(ui::MenuButtonContainer& menu, AssetBrowserTabFiles* tab, const Array<ManagedItem*>& items)
    {
        auto editor = GetService<Editor>();
        auto menuContext = CreateSharedPtr<DepotContext>(tab, items);

        /*(if (menuContext->m_isPhysicalDirectory)
        {
            ret->addItem().caption("Add directory...").imageName("folder_add").OnClick = [menuContext](UI_CALLBACK) { menuContext->cmdAddDirectory(); };
            ret->addItem().
            ret->addItem().caption("Add directory...").imageName("folder_add").OnClick = [menuContext](UI_CALLBACK) { menuContext->cmdAddDirectory(); };
        }*/

        HashSet<const ManagedFileFormat*> fileFormats;
        for (const auto& file : menuContext->m_files)
            fileFormats.insert(&file->fileFormat());

        if (menuContext->m_files.size() >= 1)
        {
            bool canOpen = false;
            for (const auto* format : fileFormats)
            {
                if (editor->canOpenFile(*format))
                {
                    canOpen = true;
                    break;
                }
            }

            if (canOpen)
            {
                menu.createCallback("[b]Open...[/b]", "[img:open]", "Enter") = [menuContext]() { menuContext->cmdOpen(); };
                menu.createSeparator();
            }
        }

        if (!menuContext->m_items.empty() && tab)
        {
            menu.createCallback("Duplicate", "[img:file_copy]") = [menuContext]() { menuContext->cmdDuplicate(); };
            menu.createCallback("Delete", "[img:cross]") = [menuContext]() { menuContext->cmdDelete(); };
            menu.createSeparator();
        }

        if (!menuContext->m_items.empty())
        {
            if (menuContext->m_items.size() == 1 && menuContext->m_hasFilesWithAbsolutePath)
            {
                menu.createCallback("Show in files...", "[img:zoom]") = [menuContext]() { menuContext->cmdShowInFiles(); };
                menu.createSeparator();
            }

            menu.createCallback("Copy depot path") = [menuContext]() { menuContext->cmdCopyDepotPath(); };

            if (menuContext->m_hasFilesWithAbsolutePath)
                menu.createCallback("Copy absolute path(s)") = [menuContext]() { menuContext->cmdCopyAbsolutePath(); };
        }
    }

    void BuildDepotContextMenu(ui::MenuButtonContainer& menu, AssetBrowserTabFiles* tab, ManagedItem* item)
    {
        if (item)
        {
            InplaceArray<ManagedItem*, 1> items;
            items.pushBack(item);
            BuildDepotContextMenu(menu, tab, items);
        }
    }

    ///---

    /*
      <WindowTitleBar/>

  <TextLabel name="Prompt" text="Please confirm deletion of following files:" margin="5"/>
  <ListView name="FileList" initialSize="500;400" valign="expand" halign="expand"/>

  <Element layout="horizontal" halign="right" padding="5" >
    <Button style="PushButton" class="red" name="Delete" text="[img:delete] Delete" valign="middle"/>
    <Button style="PushButton" name="Cancel" text="Cancel" valign="middle"/>
  </Element>*/

    ui::EventFunctionBinder DeleteFiles(ui::IElement* owner, const AssetItemList& assetItems)
    {
        auto window = CreateSharedPtr<ui::Window>();
        window->layoutVertical();
        window->createChild<ui::WindowTitleBar>("Save files")->addStyleClass("mini"_id);

        {
            auto elem = window->createChild<ui::TextLabel>();

            StringBuilder txt;
            txt << "Following";
            if (assetItems.files.size())
                txt.appendf(" {} file{}", assetItems.files.size(), assetItems.files.size() == 1 ? "" : "s");
            if (assetItems.dirs.size() && assetItems.files.size())
                txt << " and";
            if (assetItems.dirs.size())
                txt.appendf(" {} director{}", assetItems.dirs.size(), assetItems.dirs.size() == 1 ? "y" : "ies");
            txt << " will be ";

            txt << "[b][color:#F33]";
            txt << "PERMAMENTLY DELETED!";
            txt << "[/color][/b]";

            elem->text(txt.view());
        }

        auto fileList = CreateSharedPtr<AssetItemsSimpleListModel>();
        for (auto* file : assetItems.files)
            fileList->addItem(file);
        for (auto* dir : assetItems.dirs)
            fileList->addItem(dir);

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
            auto button = buttons->createChildWithType<ui::Button>("PustButton"_id, "[img:delete] Delete");
            button->addStyleClass("red"_id);
            button->bind("OnClick"_id, window) = [fileList](ui::Window* window)
            {
                auto listToDelete = fileList->checked();

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
                            if (IO::GetInstance().deleteFile(file->absolutePath()))
                                deleted = true;
                            else
                                fileList->comment(item, "  [img:error] Failed to delete physical file");
                        }
                        else if (auto file = rtti_cast<ManagedDirectory>(item))
                        {
                            if (IO::GetInstance().deleteDir(file->absolutePath()))
                                deleted = true;
                            else
                                fileList->comment(item, "  [img:error] Failed to delete physical directory");
                        }
                    }

                    if (deleted)
                        fileList->removeItem(item);
                }

                if (fileList->size() == 0)
                {
                    window->requestClose();
                    window->call("OnResult"_id, 1);
                }
            };
        }

        {
            auto button = buttons->createChildWithType<ui::Button>("PustButton"_id, "Cancel");
            button->bind("OnClick"_id, window) = [](ui::Window* window)
            {
                window->call("OnResult"_id, 0);
                window->requestClose();
            };
        }

        window->bind("OnKeyDown"_id) = [fileList](ui::Window* window, ui::IElement*, input::KeyCode code)
        {
            if (code == input::KeyCode::KEY_UP || code == input::KeyCode::KEY_DOWN)
            {
                if (auto elem = window->findChildByName<ui::ListView>("FileList"_id))
                    elem->focus();
                return true;
            }
            else if (code == input::KeyCode::KEY_ESCAPE)
            {
                window->call("OnResult"_id, 0);
                window->requestClose();
                return true;
            }
            else if (code == input::KeyCode::KEY_SPACE)
            {
                if (auto elem = window->findChildByName<ui::ListView>("FileList"_id))
                {
                    for (const auto& index : elem->selection())
                        fileList->checked(index, !fileList->checked(index));
                }
                return true;
            }
            else if (code == input::KeyCode::KEY_RETURN)
            {
                if (auto elem = window->findChildByName<ui::Button>("Delete"_id))
                    return elem->call("OnClick"_id);
            }

            return false;
        };

        window->showModal(owner);
        return window->bind("OnResult"_id);
    }

    ///---

    /*
      <WindowTitleBar/>

  <TextLabel name="Prompt" text="Following files are [b][color:#F88]modified[/color][/b] and should be saved:" margin="5"/>
  <ListView name="FileList" initialSize="500;400" valign="expand" halign="expand"/>

  <Element layout="horizontal" halign="right" padding="5" >
    <Button style="PushButton" class="green" name="Save" text="[img:disk] Save" valign="middle"/>
    <Button style="PushButton" class="red" name="DontSave" text="[img:delete] Don't save" valign="middle"/>
    <Button style="PushButton" name="Cancel" text="Cancel" valign="middle"/>
  </Element>*/

    ui::EventFunctionBinder SaveFiles(ui::IElement* owner, const AssetItemList& assetItems)
    {
        auto window = CreateSharedPtr<ui::Window>();
        window->layoutVertical();
        window->createChild<ui::WindowTitleBar>("Save files")->addStyleClass("mini"_id);

        window->createChild<ui::TextLabel>("Following files are [b][color:#F88]modified[/color][/b] and should be saved:")->customMargins(5.0f);

        auto fileList = CreateSharedPtr<AssetItemsSimpleListModel>();
        for (auto* file : assetItems.files)
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
            auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "[img:disk] Save");
            button->addStyleClass("green"_id);
            button->bind("OnClick"_id, window) = [fileList](ui::Window* window)
            {
                auto listToSave = fileList->checked();
                auto& mainWindow = GetService<Editor>()->mainWindow();

                for (const auto& item : listToSave)
                {
                    bool saved = false;

                    if (auto file = rtti_cast<ManagedFile>(item))
                    {
                        if (!mainWindow.saveFile(file))
                        {
                            fileList->comment(item, "  [img:error] Unable to save");
                        }
                        else
                        {
                            saved = true;
                        }
                    }

                    if (saved)
                        fileList->removeItem(item);
                }

                if (fileList->size() == 0)
                {
                    window->requestClose();
                    window->call("OnResult"_id, 1);
                }
            };
        }

        {
            auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "[img:delete] Don't save");
            button->addStyleClass("red"_id);

            button->bind("OnClick"_id, window) = [](ui::Window* window)
            {
                window->call("OnResult"_id, 1);
                window->requestClose();
            };
        }

        {
            auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "Cancel");
            button->bind("OnClick"_id, window) = [](ui::Window* window)
            {
                window->call("OnResult"_id, 0);
                window->requestClose();
            };
        }

        window->bind("OnKeyDown"_id) = [fileList](ui::Window* window, ui::IElement*, input::KeyCode code)
        {
            if (code == input::KeyCode::KEY_UP || code == input::KeyCode::KEY_DOWN)
            {
                if (auto elem = window->findChildByName<ui::ListView>("FileList"_id))
                    elem->focus();
                return true;
            }
            else if (code == input::KeyCode::KEY_ESCAPE)
            {
                window->call("OnResult"_id, 0);
                window->requestClose();
                return true;
            }
            else if (code == input::KeyCode::KEY_SPACE)
            {
                if (auto elem = window->findChildByName<ui::ListView>("FileList"_id))
                {
                    for (const auto& index : elem->selection())
                        fileList->checked(index, !fileList->checked(index));
                }
                return true;
            }
            else if (code == input::KeyCode::KEY_RETURN)
            {
                if (auto elem = window->findChildByName<ui::Button>("Save"_id))
                    return elem->call("OnClick"_id);
            }

            return false;
        };

        window->showModal(owner);
        return window->bind("OnResult"_id);
    }

    ///---

} // ed

