/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

class AssetBrowserTabFiles;
class IAssetBrowserDepotVisItem;

/// build menu for given single item
//extern EDITOR_COMMON_API void BuildDepotContextMenuForSingleFile(ui::IElement* owner, ui::MenuButtonContainer& menu, const DepotMenuContext& context, StringView fileDepotPath);

/// build menu for given items
extern EDITOR_COMMON_API void BuildDepotContextMenu(AssetBrowserTabFiles* tab, ui::MenuButtonContainer& menu, const Array<RefPtr<IAssetBrowserDepotVisItem>>& items);

//---

END_BOOMER_NAMESPACE_EX(ed)

