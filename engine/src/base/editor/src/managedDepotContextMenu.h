/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#pragma once

namespace ed
{
    //--

    class AssetBrowserTabFiles;

    struct DepotMenuContext
    {
        AssetBrowserTabFiles* tab = nullptr; // only if we are inside the tab
        ManagedDirectory* contextDirectory = nullptr;
    };

    /// build menu for given single item
    extern BASE_EDITOR_API void BuildDepotContextMenu(ui::IElement* owner, ui::MenuButtonContainer& menu, const DepotMenuContext& context, ManagedItem* item);

    /// build menu for given items
    extern BASE_EDITOR_API void BuildDepotContextMenu(ui::IElement* owner, ui::MenuButtonContainer& menu, const DepotMenuContext& context, const Array<ManagedItem*>& items);

    //--

    /// open all files in list
    extern BASE_EDITOR_API void OpenDepotFiles(ui::IElement* owner, const Array<ManagedFile*>& files);

    //---

} // ed

