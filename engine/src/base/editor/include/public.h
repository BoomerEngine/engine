/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base_editor_glue.inl"

using namespace base;

// event to observe files in depot, all events have "ManagedFilePtr" as data
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_FILE_CREATED);
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_FILE_DELETED);
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_FILE_OPENED); // in asset browser
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_FILE_CLOSED); // in asset browser
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_FILE_MODIFIED);
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_FILE_UNMODIFIED);
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_FILE_SAVED);
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_FILE_RELOADED);

// event to observe directories in depot, all events have "ManagedDirectoryPtr" as data
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_DIRECTORY_OPENED); // in asset browser
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_DIRECTORY_CLOSED); // in asset browser
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_DIRECTORY_CREATED);
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_DIRECTORY_DELETED);
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_DIRECTORY_BOOKMARKED);
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_DIRECTORY_UNBOOKMARKED);

// event to observer on specific file, no data
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_FILE_MODIFIED);
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_FILE_UNMODIFIED);
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_FILE_SAVED);
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_FILE_RELOADED);
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_FILE_CHANGED); // file on disk hash changed
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_FILE_DELETED);
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_FILE_CREATED); // called also when we are undeleted

// event to observer on specific directory, no data
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DIRECTORY_DELETED);
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DIRECTORY_CREATED); // called also when we are undeleted
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DIRECTORY_BOOKMARKED);
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DIRECTORY_UNBOOKMARKED);

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

    class IBackgroundCommand;
    typedef RefPtr<IBackgroundCommand> BackgroundCommandPtr;
    typedef RefWeakPtr<IBackgroundCommand> BackgroundCommandWeakPtr;

    class IBackgroundJob;
    typedef RefPtr<IBackgroundJob> BackgroundJobPtr;

    class BackgroundJobUnclaimedConnection;
    typedef RefPtr<BackgroundJobUnclaimedConnection> BackgroundJobUnclaimedConnectionPtr;

    class ConfigGroup;
    class ConfigRoot;

    class ManagedDepot;

    class ManagedItem;
    typedef RefPtr<ManagedItem> ManagedItemPtr;
    typedef RefWeakPtr<ManagedItem> ManagedIteamWeakPTr;

    class ManagedFile;
    typedef RefPtr<ManagedFile> ManagedFilePtr;
    typedef RefWeakPtr<ManagedFile> ManagedFileWeakPtr;

    class ManagedFileNativeResource;
    typedef RefPtr<ManagedFileNativeResource> ManagedFileNativeResourcePtr;
    typedef RefWeakPtr<ManagedFileNativeResource> ManagedFileNativeResourceWeakPtr;

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
    class ManagedFilePlaceholder;

    //--

    class IResourceEditorAspect;
    typedef RefPtr<IResourceEditorAspect> ResourceEditorAspectPtr;

    //--

    class MainWindow;

    class AssetBrowser;
    class AssetImportPrepareTab;
    class AssetImportMainTab;

    enum class AssetBrowserContext : uint8_t
    {
        DirectoryTab,
        EditorTabHeader,
    };

    //--
        
} // ed

using input::KeyCode;

#include "managedDepot.h"
#include "managedFile.h"
#include "editorConfig.h"
#include "editorWindow.h"