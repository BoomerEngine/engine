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

/// show a modal window to delete files
extern EDITOR_COMMON_API void DeleteDepotFiles(ui::IElement* owner, const ManagedFileCollection& files);

/// show a modal window to delete depot items, directories will be expanded to include all sub directories and files
extern EDITOR_COMMON_API void DeleteDepotItems(ui::IElement* owner, const Array<ManagedItem*>& items);

/// show a modal window to save files, returns false if user pressed "cancel" otherwise returns true
extern EDITOR_COMMON_API bool SaveDepotFiles(ui::IElement* owner, const ManagedFileCollection& files);

/// show a list of opened files, allows to switch to editor, close windows etc
extern EDITOR_COMMON_API void ShowOpenedFilesList(ui::IElement* owner, ManagedFile* focusFile);

//--

/// ask about file name in given directory (used for "Save As Prefab..." etc)
extern EDITOR_COMMON_API bool ShowSaveAsFileDialog(ui::IElement* owner, ManagedDirectory* specificDirectory, ClassType resourceClass, StringView message, StringView initialFileName, StringBuf& outDepotPath);

//--

/// show a generic rename dialog, does not do any renaming
extern EDITOR_COMMON_API bool ShowGenericRenameDialog(ui::IElement* owner, const ManagedItem* item, RenameConfiguration& outSettings);

//--

/// rename a single file
extern EDITOR_COMMON_API ManagedItem* RenameItem(ui::IElement* owner, ManagedItem* file);

/// move items(s) to new location
extern EDITOR_COMMON_API bool MoveItems(ui::IElement* owner, const Array<ManagedItem*>& items, ManagedDirectory* target);

/// copy items(s) to new location
extern EDITOR_COMMON_API bool CopyItems(ui::IElement* owner, const Array<ManagedItem*>& items, ManagedDirectory* target);

//--

END_BOOMER_NAMESPACE_EX(ed)

