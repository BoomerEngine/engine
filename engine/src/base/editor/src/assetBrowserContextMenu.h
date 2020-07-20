/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"
#include "managedDirectory.h"
#include "managedItem.h"
#include "managedDepot.h"

namespace ed
{
    struct AssetItemList;

    //--

    /// show a modal window to delete files
    extern ui::EventFunctionBinder DeleteFiles(ui::IElement* owner, const AssetItemList& files);

    /// show a modal window to save files
    extern ui::EventFunctionBinder SaveFiles(ui::IElement* owner, const AssetItemList& files);

    //--

    class AssetBrowserTabFiles;

    /// build menu for given items
    extern void BuildDepotContextMenu(ui::MenuButtonContainer& menu, AssetBrowserTabFiles* tab, const Array<ManagedItem*>& items);

    /// build menu for given single item
    extern void BuildDepotContextMenu(ui::MenuButtonContainer& menu, AssetBrowserTabFiles* tab, ManagedItem* item);

    //--

} // ed

