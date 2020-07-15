/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base_editor_glue.inl"

using namespace base;

namespace ed
{
    namespace vsc
    {
        class IChangelist;
        typedef RefPtr<IChangelist> ChangelistPtr;

        class IVersionControl;
        typedef UniquePtr<IVersionControl> VersionControlPtr;

        struct FileState;
        struct Result;

    } // vsc

    class ConfigGroup;
    class ConfigRoot;

    class ManagedDepot;

    class ManagedItem;
    typedef RefPtr<ManagedItem> ManagedItemPtr;
    typedef RefWeakPtr<ManagedItem> ManagedIteamWeakPTr;

    class ManagedFile;
    typedef RefPtr<ManagedFile> ManagedFilePtr;
    typedef RefWeakPtr<ManagedFile> ManagedFileWeakPtr;

    class ManagedDirectory;
    typedef RefPtr<ManagedDirectory> ManagedDirectoryPtr;
    typedef RefWeakPtr<ManagedDirectory> ManagedDirectoryWeakPtr;

    class ManagedThumbnailEntry;
    typedef RefPtr<ManagedThumbnailEntry> ManagedThumbnailEntryPtr;
    typedef RefWeakPtr<ManagedThumbnailEntry> ManagedThumbnailEntryWeakPtr;

    class ManagedFileImportStatusCheck;
    typedef RefPtr<ManagedFileImportStatusCheck> ManagedFileImportStatusCheckPtr;
    typedef RefWeakPtr<ManagedFileImportStatusCheck> ManagedFileImportStatusCheckWeakPtr;

    class ManagedFileFormat;

    class IDataBox;
    class DataInspector;

    class MainWindow;

    enum class AssetBrowserContext : uint8_t
    {
        DirectoryTab,
    };
        
} // ed

using input::KeyCode;

#include "managedDepot.h"
#include "managedFile.h"
#include "editorConfig.h"
#include "editorWindow.h"