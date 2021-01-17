/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base_editor_glue.inl"

using namespace base;

// event to observe files in depot, all events have "ManagedFilePtr" as data
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_FILE_CREATED, ManagedFilePtr);
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_FILE_DELETED, ManagedFilePtr);
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_FILE_OPENED, ManagedFilePtr); // in asset browser
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_FILE_CLOSED, ManagedFilePtr); // in asset browser
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_FILE_MODIFIED, ManagedFilePtr);
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_FILE_UNMODIFIED, ManagedFilePtr);
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_FILE_SAVED, ManagedFilePtr);
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_FILE_RELOADED, ManagedFilePtr);

// event to observe directories in depot, all events have "ManagedDirectoryPtr" as data
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_DIRECTORY_OPENED, ManagedDirectoryPtr); // in asset browser
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_DIRECTORY_CLOSED, ManagedDirectoryPtr); // in asset browser
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_DIRECTORY_CREATED, ManagedDirectoryPtr);
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_DIRECTORY_DELETED, ManagedDirectoryPtr);
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_DIRECTORY_BOOKMARKED, ManagedDirectoryPtr);
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_DIRECTORY_UNBOOKMARKED, ManagedDirectoryPtr);

// event to indicate that placeholder items were discarded/accepted, take ManagedItemPtr
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_PLACEHOLDER_DISCARDED, ManagedItemPtr);
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_DEPOT_PLACEHOLDER_ACCEPTED, ManagedItemPtr);

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

// event to indicate that placeholder items were discarded/accepted
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_PLACEHOLDER_DISCARDED);
DECLARE_GLOBAL_EVENT(EVENT_MANAGED_PLACEHOLDER_ACCEPTED);


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

    class ManagedDepot;
    class ManagedFileCollection;

    class ManagedItem;
    typedef RefPtr<ManagedItem> ManagedItemPtr;
    typedef RefWeakPtr<ManagedItem> ManagedIteamWeakPTr;

    class ManagedFile;
    typedef RefPtr<ManagedFile> ManagedFilePtr;
    typedef RefWeakPtr<ManagedFile> ManagedFileWeakPtr;

    class ManagedFileNativeResource;
    typedef RefPtr<ManagedFileNativeResource> ManagedFileNativeResourcePtr;
    typedef RefWeakPtr<ManagedFileNativeResource> ManagedFileNativeResourceWeakPtr;

    class ManagedFilePlaceholder;
    typedef RefPtr<ManagedFilePlaceholder> ManagedFilePlaceholderPtr;
    typedef RefWeakPtr<ManagedFilePlaceholder> ManagedFilePlaceholderWeakPtr;

    class ManagedDirectoryPlaceholder;
    typedef RefPtr<ManagedDirectoryPlaceholder> ManagedDirectoryPlaceholderPtr;
    typedef RefWeakPtr<ManagedDirectoryPlaceholder> ManagedDirectoryPlaceholderWeakPtr;

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

    class ResourceEditor;
    typedef RefPtr<ResourceEditor> ResourceEditorPtr;

    class IResourceEditorAspect;
    typedef RefPtr<IResourceEditorAspect> ResourceEditorAspectPtr;

    //--

    class Editor;

    class IBaseResourceContainerWindow;
    class MainWindow;

    class AssetBrowser;
    class AssetImportPrepareTab;
    class AssetImportMainTab;

    typedef SpecificClassType<res::IResource> TImportClass;

    enum class AssetBrowserContext : uint8_t
    {
        DirectoryTab,
        EditorTabHeader,
    };

    //--
        
} // ed

using input::KeyCode;
