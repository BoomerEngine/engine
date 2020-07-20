/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base_resource_compiler_glue.inl"

// add events have StringBuf as data
DECLARE_GLOBAL_EVENT(EVENT_DEPOT_FILE_CHANGED)
DECLARE_GLOBAL_EVENT(EVENT_DEPOT_FILE_ADDED)
DECLARE_GLOBAL_EVENT(EVENT_DEPOT_FILE_REMOVED)
DECLARE_GLOBAL_EVENT(EVENT_DEPOT_FILE_RELOADED)
DECLARE_GLOBAL_EVENT(EVENT_DEPOT_DIRECTORY_ADDED)
DECLARE_GLOBAL_EVENT(EVENT_DEPOT_DIRECTORY_REMOVED)

namespace base
{

    namespace depot
    {

        //--

        class DepotStructure;

        class IFileSystem;
        class IFileSystemProvider;
        class IFileSystemNotifier;

        enum class DepotFileSystemType
        {
            Engine,
            Plugin,
            Project,
        };

        //--

    } // depot

    namespace res
    {

        class Cooker;

        class IBakingJob;
        typedef RefPtr<IBakingJob> BakingJobPtr;

        class ImportFileService;

        class IImportSink;

        class ISourceAsset;
        typedef RefPtr<ISourceAsset> SourceAssetPtr;

        class ISourceAssetFileSystem;
        typedef RefPtr<ISourceAssetFileSystem> SourceAssetFileSystemPtr;

        class SourceAssetRepository;

        class IResourceImporter;

        class ImportFileFingerprint;

        class ImportList;
        typedef RefPtr<ImportList> ImportListPtr;

        struct ImportQueueFileStatusChangeMessage;

        //--

        /// import status of file
        enum class ImportStatus : uint8_t
        {
            Pending, // pending status, no job done yet
            Checking, // status is being checked
            Canceled, // processing was canceled
            Processing, // file is being imported
            UpToDate, // resource is valid and up to date
            NotUpToDate, // resource is imported but it's not up to date
            NotSupported, // resource import is no longer supported (ie. imported SpeedTree but we are no longer linked with SpeedTree SDK, etc)
            NotImportable, // resource is not importable (ie. does not come from file)
            MissingAssets, // source files required to import resource are missing - we can't import again
            InvalidAssets, // assets are invalid (ie. error in the source file)
            NewAssetImported, // new asset was imported
        };

        /// result of asset file validation
        enum class SourceAssetStatus : uint8_t
        {
            Checking,
            ContentChanged,
            UpToDate,
            ReadFailure,
            Missing,
            Canceled,
        };

        //--

    } // res

} // base