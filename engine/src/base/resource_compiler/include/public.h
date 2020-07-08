/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base_resource_compiler_glue.inl"

namespace base
{

    namespace depot
    {

        //--

        class DepotStructure;

        class IFileSystem;
        class IFileSystemProvider;
        class IFileSystemNotifier;

        class PackageManifest;
        typedef RefPtr<PackageManifest> PackageManifestPtr;

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

        //--

        /// import status of file
        enum class ImportStatus : uint8_t
        {
            UpToDate, // resource is valid and up to date
            NotUpToDate, // resource is imported but it's not up to date
            NotSupported, // resource import is no longer supported (ie. imported SpeedTree but we are no longer linked with SpeedTree SDK, etc)
            MissingAssets, // source files required to import resource are missing - we can't import again
            InvalidAssets, // assets are invalid (ie. error in the source file)
            NewAssetImported, // new asset was imported
        };

        //--

    } // res

} // base