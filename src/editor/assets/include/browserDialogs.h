/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

/// show a modal window to delete depot items, directories will be expanded to include all sub directories and files
extern EDITOR_ASSETS_API void DeleteDepotItems(ui::IElement* owner, const Array<StringBuf>& depotPaths);

/// show a modal window to save files, returns false if user pressed "cancel" otherwise returns true
extern EDITOR_ASSETS_API bool SaveEditors(ui::IElement* owner, const Array<ResourceEditorPtr>& editors);

/// show a list of opened files, allows to switch to editor, close windows etc
extern EDITOR_ASSETS_API void ShowOpenedEditorsList(ui::IElement* owner, const ResourceEditor* editor = nullptr);

//--

/// ask about file name in given directory (used for "Save As Prefab..." etc)
extern EDITOR_ASSETS_API bool ShowSaveAsFileDialog(ui::IElement* owner, StringView specificDirectory, StringView message, StringView initialFileName, StringBuf& outDepotPath, StringView extension="");

//--

/// show a generic rename dialog, does not do any renaming
extern EDITOR_ASSETS_API bool ShowGenericRenameDialog(ui::IElement* owner, StringView depotPath, StringBuf& outNewPath);

//--

/// rename a single file
extern EDITOR_ASSETS_API bool RenameItem(ui::IElement* owner, StringView path, StringBuf& outRenamedPath);

/// move items(s) to new location
extern EDITOR_ASSETS_API bool MoveItems(ui::IElement* owner, const Array<StringBuf>& items, StringView targetDepotPath);

/// copy items(s) to new location
extern EDITOR_ASSETS_API bool CopyItems(ui::IElement* owner, const Array<StringBuf>& items, StringView targetDepotPath);

//--

END_BOOMER_NAMESPACE_EX(ed)

