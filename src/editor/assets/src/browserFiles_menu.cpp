/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"

#include "browserService.h"

#include "browserFiles.h"
#include "browserFilesModel.h"

#include "importPrepare.h"

#include "editor/common/include/service.h"
#include "core/resource/include/metadata.h"
#include "engine/ui/include/uiMenuBar.h"
#include "engine/ui/include/uiRenderer.h"
#include "core/resource/include/depot.h"
#include "core/resource_compiler/include/sourceAssetService.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

void AssetBrowserTabFiles::buildContextMenu(ui::MenuButtonContainer& menu)
{
    if (!buildFilesContextMenu(menu))
        buildGenericContextMenu(menu);

    // TODO: shared options

}

bool AssetBrowserTabFiles::buildFilesContextMenu(ui::MenuButtonContainer& menu)
{
    Array<StringBuf> dirs, files;
    
    m_files->selection().visit<IAssetBrowserDepotVisItem>([&dirs, &files](IAssetBrowserDepotVisItem* vis)
        {
            if (vis->is<AssetBrowserFileVis>())
                files.pushBack(vis->depotPath());
            else if (vis->is<AssetBrowserDirectoryVis>())
                dirs.pushBack(vis->depotPath());
        });

    if (dirs.empty() && files.empty())
        return false;

    if (files.size() && !dirs.size())
    {
        if (files.size() == 1)
            buildSingleFileContextMenu(menu, files[0]);

        buildManyFilesContextMenu(menu, files);
    }
    else if (files.size() == 0 && dirs.size() == 1)
    {
        buildSingleDirContextMenu(menu, dirs[0]);
    }
    
    buildMixedItemsContextMenu(menu);
    
    return true;
}

void AssetBrowserTabFiles::buildSingleFileContextMenu(ui::MenuButtonContainer& menu, const StringBuf& depotPath)
{
    menu.createCallback("[b]Open[/b]", "[img:file_empty_edit]", "Enter") = [this]()
    {
        if (auto file = selectedFile())
            GetService<AssetBrowserService>()->openEditor(this, file, true);
    };

    menu.createSeparator();

    menu.createCallback("Show asset file...", "[img:zoom]", "F3") = [depotPath, this]()
    {
        if (auto file = selectedFile())
        {
            StringBuf absolutePath;
            if (GetService<DepotService>()->queryFileAbsolutePath(depotPath, absolutePath))
                ShowFileExplorer(absolutePath);
        }
    };

    if (auto metadata = GetService<DepotService>()->loadFileToXMLObject<ResourceMetadata>(depotPath))
    {
        if (!metadata->importDependencies.empty())
        {
            StringBuf absolutePath;

            const auto sourcePath = metadata->importDependencies[0].importPath;
            if (GetService<SourceAssetService>()->resolveContextPath(sourcePath, absolutePath))
            {
                menu.createCallback("Show source file...", "[img:zoom]", "Shift+F3") = [absolutePath, this]()
                {
                    ShowFileExplorer(absolutePath);
                };
            }
        }
    }

    menu.createSeparator();
}

void AssetBrowserTabFiles::buildSingleDirContextMenu(ui::MenuButtonContainer& menu, const StringBuf& depotPath)
{
    {
        auto bookmarked = GetService<AssetBrowserService>()->checkDirectoryBookmark(depotPath);
        auto icon = bookmarked ? "[img:star]" : "[img:star_gray]";
        auto text = bookmarked ? "Remove bookmark" : "Bookmark";
        menu.createCallback(text, icon) = [bookmarked, depotPath]()
        {
            GetService<AssetBrowserService>()->toggleDirectoryBookmark(depotPath, !bookmarked);
        };

        menu.createSeparator();
    }
}

void AssetBrowserTabFiles::buildManyFilesContextMenu(ui::MenuButtonContainer& menu, const Array<StringBuf>& depotPaths)
{
    bool anyImported = false;

    for (const auto& path : depotPaths)
    {
        if (auto metadata = GetService<DepotService>()->loadFileToXMLObject<ResourceMetadata>(path))
        {
            if (!metadata->importDependencies.empty())
            {
                anyImported = true;
                break;
            }
        }
    }

    //--

    if (anyImported)
    {
        menu.createCallback("Reimport...", "[img:import]") = [this]()
        {
            bool anythingToReimport = false;

            auto dlg = RefNew<AssetImportPrepareDialog>();
            for (const auto& path : selectedFiles())
                anythingToReimport |= dlg->reimportFile(path);

            if (anythingToReimport)
                dlg->runModal(this);
            else
                ui::PostWindowMessage(this, ui::MessageType::Warning, "Reimport"_id, "None of the selected files can be reimported");
        };

        menu.createSeparator();
    }
}

void AssetBrowserTabFiles::buildMixedItemsContextMenu(ui::MenuButtonContainer& menu)
{
    menu.createSeparator();

    menu.createCallback("Mark for copy", "[img:copy]") = [this]()
    {
        cmdCopySelection();
    };

    menu.createCallback("Mark for move", "[img:cut]") = [this]()
    {
        cmdCopySelection();
    };

    menu.createSeparator();


    menu.createSeparator();

    if (m_setup.allowFileActions)
    {
        menu.createCallback("[b][color:#FAA]Delete[/b][/color] from depot...", "[img:cross]") = [this]()
        {
            //DeleteDepotItems(this, selectedItems());
        };

        menu.createSeparator();
    }

    menu.createCallback("Copy depot paths", "[img:copy]") = [this]()
    {
        StringBuilder txt;
        for (const auto& path : selectedItems())
        {
            if (!txt.empty()) txt << "\n";
            txt << path;
        }

        clipboard().storeText(txt.view());
    };

    menu.createCallback("Copy absolute paths", "[img:copy]") = [this]()
    {
        StringBuilder txt;
        for (const auto& path : selectedItems())
        {
            if (!txt.empty()) txt << "\n";

            StringBuf absolutePath;
            if (GetService<DepotService>()->queryFileAbsolutePath(path, absolutePath))
                txt << absolutePath;
        }

        clipboard().storeText(txt.view());
    };

    menu.createSeparator();
}

void AssetBrowserTabFiles::buildGenericContextMenu(ui::MenuButtonContainer& menu)
{
    // new directory
    if (m_depotPath)
    {
        // new asset sub menu
        {
            auto newAssetSubMenu = RefNew<ui::MenuButtonContainer>();
            buildNewAssetMenu(newAssetSubMenu);
            menu.createSubMenu(newAssetSubMenu->convertToPopup(), "New", "[img:file_add]");
        }

        // create asset sub menu
        {
            auto createAssetSubMenu = RefNew<ui::MenuButtonContainer>();
            buildImportAssetMenu(createAssetSubMenu);
            menu.createSubMenu(createAssetSubMenu->convertToPopup(), "Import", "[img:file_go]");
        }

        menu.createSeparator();

        {
            auto bookmarked = GetService<AssetBrowserService>()->checkDirectoryBookmark(m_depotPath);
            auto icon = bookmarked ? "[img:star]" : "[img:star_gray]";
            auto text = bookmarked ? "Remove bookmark" : "Bookmark";
            menu.createCallback(text, icon) = [this]()
            {
                cmdBookmarkTab();
            };
        }

        menu.createSeparator();

        menu.createCallback("Show in files...", "[img:zoom]", "F3") = [this]()
        {
            StringBuf absolutePath;
            if (GetService<DepotService>()->queryFileAbsolutePath(m_depotPath, absolutePath))
                ShowFileExplorer(absolutePath);
        };

        menu.createSeparator();
    }
}

//--

END_BOOMER_NAMESPACE_EX(ed)
