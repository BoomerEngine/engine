/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#include "build.h"

#include "editorService.h"

#include "managedDirectory.h"
#include "managedFile.h"
#include "managedFileFormat.h"
#include "managedItemCollection.h"
#include "managedDepotContextMenu.h"
#include "managedFileNativeResource.h"

#include "assetBrowserDialogs.h"
#include "assetBrowserTabFiles.h"

#include "engine/ui/include/uiMenuBar.h"
#include "engine/ui/include/uiElement.h"
#include "engine/ui/include/uiRenderer.h"
#include "core/io/include/io.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

///---


static void SplitList(const Array<ManagedItem*>& items, Array<ManagedFile*>& outFiles, Array<ManagedDirectory*>& outDirs)
{
    // TODO: reserve space in array

    for (auto* item : items)
    {
        if (auto file = rtti_cast<ManagedFile>(item))
        {
            outFiles.pushBack(file);
        }
        else if (auto dir = rtti_cast<ManagedDirectory>(item))
        {
            outDirs.pushBack(dir);
        }
    }
}

void OpenDepotFiles(ui::IElement* owner, const Array<ManagedFile*>& files)
{
    for (auto* file : files)
        GetEditor()->openFileEditor(file);
}

void SaveDepotFiles(ui::IElement* owner, const Array<ManagedFile*>& files)
{
    for (auto* file : files)
        GetEditor()->saveFileEditor(file);
}

void CloseDepotFiles(ui::IElement* owner, const Array<ManagedFile*>& files)
{
    for (auto* file : files)
        GetEditor()->closeFileEditor(file);
}

void ReimportDepotFiles(ui::IElement* owner, const Array<ManagedFile*>& files)
{
    Array<ManagedFileNativeResource*> filesToReimport;
    for (auto* file : files)
    {
        if (auto* nativeFile = rtti_cast<ManagedFileNativeResource>(file))
        {
            if (nativeFile->canReimport())
            {
                filesToReimport.pushBack(nativeFile);
            }
        }
    }

    GetEditor()->reimportFiles(filesToReimport);
}

void CopyDepotItems(ui::IElement* owner, const Array<ManagedItem*>& item)
{

}

void CutDepotItems(ui::IElement* owner, const Array<ManagedItem*>& item)
{

}

void BuildDepotContextMenu(ui::IElement* owner, ui::MenuButtonContainer& menu, const DepotMenuContext& context, const Array<ManagedItem*>& items)
{
    Array<ManagedFile*> files;
    Array<ManagedDirectory*> dirs;
    SplitList(items, files, dirs);

    // Save/Open/Close
    {
        Array<ManagedFile*> openableFiles;
        //Array<ManagedFile*> closableFiles;
        Array<ManagedFile*> saveableFiles;

        for (auto* file : files)
        {
            if (GetEditor()->canOpenFile(file) && !GetEditor()->findFileEditor(file))
                openableFiles.pushBack(file);
            if (file->isModified() && GetEditor()->findFileEditor(file))
                saveableFiles.pushBack(file);
        }

        if (!openableFiles.empty())
            menu.createCallback("[b]Open...[/b]", "[img:open]", "Enter") = [openableFiles, owner]() { OpenDepotFiles(owner, openableFiles); };
            
        if (!saveableFiles.empty())
            menu.createCallback("Save", "[img:save]", "Shift+Ctrl+S") = [saveableFiles, owner]() { SaveDepotFiles(owner, saveableFiles); };

        //if (!closableFiles.empty())
            //  menu.createCallback("Close", "[img:cross]", "Ctrl+F4") = [closableFiles, owner]() { CloseDepotFiles(owner, closableFiles); };

        menu.createSeparator();
    }

    // if we have any content we can delete it
    if (!dirs.empty() || !files.empty())
    {
        {
            bool canReimport = false;
            for (auto* file : files)
            {
                if (file->fileFormat().canUserImport())
                {
                    canReimport = true;
                    break;
                }
            }

            if (canReimport)
            {
                menu.createCallback("Reimport", "[img:import]") = [files, owner]() { ReimportDepotFiles(owner, files); };
                menu.createSeparator();
            }
        }

        if (dirs.empty() && files.size() == 1 && context.tab)
        {
            auto tab = context.tab;
            auto file = files[0];

            menu.createCallback("Duplicate...", "[img:page_arrange]") = [owner, file, tab]() {
                tab->duplicateFile(file);
            };
            menu.createSeparator();
        }

        {
            bool canDelete = false;
            for (auto* file : files)
            {
                if (!GetEditor()->findFileEditor(file))
                {
                    canDelete = true;
                    break;
                }
            }

            for (auto* dir : dirs)
            {
                if (!dir->isBookmarked())
                {
                    canDelete = true;
                    break;
                }
            }

            bool canCopy = false;
            for (auto* file : files)
            {
                if (!file->isDeleted())
                {
                    canCopy = true;
                    break;
                }
            }

            for (auto* dir : dirs)
            {
                if (!dir->isDeleted())
                {
                    canCopy = true;
                    break;
                }
            }

            if (canDelete || canCopy)
            {
                if (canDelete)
                menu.createCallback("Delete", "[img:delete]") = [items, owner]() { DeleteDepotItems(owner, items); };

                if (canCopy)
                    menu.createCallback("Copy", "[img:copy]") = [items, owner]() { CopyDepotItems(owner, items); };

                if (canDelete && canCopy)
                    menu.createCallback("Cut", "[img:cut]") = [items, owner]() { CutDepotItems(owner, items); };

                menu.createSeparator();
            }
        }
    }

    // rename
    if (files.size() == 1 && dirs.empty())
    {
        auto file = files[0];
        auto tab = context.tab;

        menu.createCallback("Rename...", "[img:rename]") = [file, owner, tab]() { 
            if (auto* renamedFile = RenameItem(owner, file))
            {
                if (tab && renamedFile->parentDirectory())
                    tab->directory(renamedFile->parentDirectory(), renamedFile);
            }
        };
        menu.createSeparator();
    }
    else if (dirs.size() == 1 && files.empty())
    {
        auto dir = dirs[0];
        auto tab = context.tab;

        menu.createCallback("Rename...", "[img:rename]") = [dir, owner, tab]() {
            if (auto* renamedDir = RenameItem(owner, dir))
            {
                if (tab && renamedDir->parentDirectory())
                    tab->directory(renamedDir->parentDirectory(), renamedDir);
            }
        };

        menu.createSeparator();
    }

    // file specific items
    if (!files.empty())
    {
        if (files.size() == 1)
        {
            auto* file = files[0];

            if (context.contextDirectory)
            {
                auto absolutePath = file->absolutePath();
                if (!absolutePath.empty())
                {
                    menu.createCallback("Show in files...", "[img:zoom]") = [absolutePath]() { ShowFileExplorer(absolutePath); };
                    menu.createSeparator();
                }
            }
            else
            {
                menu.createCallback("Show in depot", "[img:zoom]") = [file]() { GetEditor()->showFile(file); };
                menu.createSeparator();
            }
        }

        menu.createCallback("Copy depot path") = [files, owner]()
        {
            StringBuilder txt;

            for (auto* file : files)
            {
                if (auto path = file->depotPath())
                {
                    if (txt.empty())
                        txt << "\n";
                    txt << path;
                }
            }

            owner->renderer()->storeTextToClipboard(txt.view());
        };

        if (context.contextDirectory)
        {
            menu.createCallback("Copy absolute path(s)") = [files, owner]() {
                StringBuilder txt;

                for (auto* file : files)
                {
                    if (auto path = file->absolutePath())
                    {
                        if (txt.empty())
                            txt << "\n";
                        txt << path;
                    }
                }

                owner->renderer()->storeTextToClipboard(txt.view());
            };
        }
    }
}

void BuildDepotContextMenu(ui::IElement* owner, ui::MenuButtonContainer& menu, const DepotMenuContext& context, ManagedItem* item)
{
    if (item)
    {
        InplaceArray<ManagedItem*, 1> items;
        items.pushBack(item);
        BuildDepotContextMenu(owner, menu, context, items);
    }
}

///---

END_BOOMER_NAMESPACE_EX(ed)

