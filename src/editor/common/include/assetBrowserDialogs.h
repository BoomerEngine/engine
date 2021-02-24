/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(ed)

//--

/// show a modal window to delete files
extern void DeleteDepotFiles(ui::IElement* owner, const ManagedFileCollection& files);

/// show a modal window to delete depot items, directories will be expanded to include all sub directories and files
extern void DeleteDepotItems(ui::IElement* owner, const Array<ManagedItem*>& items);

/// show a modal window to save files, returns false if user pressed "cancel" otherwise returns true
extern bool SaveDepotFiles(ui::IElement* owner, const ManagedFileCollection& files);

/// show a list of opened files, allows to switch to editor, close windows etc
extern void ShowOpenedFilesList(ui::IElement* owner, ManagedFile* focusFile);

/// ask about file name in given directory (used for "Save As Prefab..." etc)
extern bool ShowSaveAsFileDialog(ui::IElement* owner, ManagedDirectory* specificDirectory, ClassType resourceClass, StringView message, StringView initialFileName, StringBuf& outDepotPath);

//--

END_BOOMER_NAMESPACE(ed)

