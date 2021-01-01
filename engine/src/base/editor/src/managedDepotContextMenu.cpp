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

#include "base/ui/include/uiMenuBar.h"
#include "base/io/include/ioSystem.h"

namespace ed
{

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
        Array<ManagedFile*> failedFiles;
        for (auto* file : files)
            if (!file->open())
                failedFiles.pushBack(file);
    }

    void SaveDepotFiles(ui::IElement* owner, const Array<ManagedFile*>& files)
    {
        Array<ManagedFile*> failedFiles;
        for (auto* file : files)
            if (!file->save())
                failedFiles.pushBack(file);
    }

    void CloseDepotFiles(ui::IElement* owner, const Array<ManagedFile*>& files)
    {
        Array<ManagedFile*> failedFiles;
        for (auto* file : files)
            if (!file->close())
                failedFiles.pushBack(file);
    }

    void ReimportDepotFiles(ui::IElement* owner, const Array<ManagedFile*>& files)
    {
        base::Array<ManagedFileNativeResource*> filesToReimport;
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

        base::GetService<Editor>()->mainWindow().addReimportFiles(filesToReimport);
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
                if (file->canOpen() && !file->opened())
                    openableFiles.pushBack(file);
                //if (file->opened())
                  //  closableFiles.pushBack(file);
                if (file->opened() && file->isModified())
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
                    if (!file->inUse() && !file->editor())
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
                        menu.createCallback("Show in files...", "[img:zoom]") = [absolutePath]() { base::io::ShowFileExplorer(absolutePath); };
                        menu.createSeparator();
                    }
                }
                else
                {
                    menu.createCallback("Show in depot", "[img:zoom]") = [file]() { base::GetService<Editor>()->mainWindow().selectFile(file); };
                    menu.createSeparator();
                }
            }

            menu.createCallback("Copy depot path") = [files]()
            {
                StringBuilder txt;

                for (auto* file : files)
                    if (auto path = file->depotPath())
                        txt.appendf("{}\n", path);

                // TODO: copy to clipboard
            };

            if (context.contextDirectory)
            {
                menu.createCallback("Copy absolute path(s)") = [files]() {
                    StringBuilder txt;

                    for (auto* file : files)
                        if (auto path = file->absolutePath())
                            txt.appendf("{}\n", path);

                    // TODO: copy to clipboard
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

} // ed

