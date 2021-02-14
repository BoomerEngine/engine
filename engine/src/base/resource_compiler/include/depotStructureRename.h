/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: depot #]
***/

#pragma once

#include "base/containers/include/hashMap.h"
#include "base/resource/include/resourceFileTables.h"
#include "base/resource/include/resourceFileTablesBuilder.h"

namespace base
{
    namespace depot
    {
        //--

        /// settings for the rename operation
        struct BASE_RESOURCE_COMPILER_API RenameConfiguration
        {
            struct RenameElement
            {
                bool file = false;
                StringBuf sourceDepotPath;
                StringBuf renamedDepotPath;
            };

            Array<RenameElement> elements;

            StringBuf fixupReferencesRootDir;
            bool moveFilesNotCopy = false;
            bool createThumbstones = true;
            bool applyLinks = true;
            bool fixupExternalReferences = true;
            bool fixupInternalReferences = true;
            bool removeEmptyDirectories = true;

            RenameConfiguration();
        };

        /// rename error/progress handler (it's so complex we need one)
        class BASE_RESOURCE_COMPILER_API IRenameProgressTracker : public IProgressTracker
        {
        public:
            virtual ~IRenameProgressTracker();

            /// report status of a file
            virtual void reportFileStatus(StringView depotPath, bool success) = 0;
        };

        //--

        struct FileRenameJob
        {
            RTTI_DECLARE_POOL(POOL_DEPOT);

        public:
            StringBuf sourceDepotPath;
            StringBuf targetDepotPath;

            bool deleteOriginalFile = false;

            res::FileTables::Header writeHeader;
            res::FileTablesBuilder writeTables;

            struct PathChange
            {
                StringBuf oldPath;
                StringBuf newPath;
            };

            Array<PathChange> pathChanges;
        };

        //--

        /// collect rename jobs, in general can fail if we have something wrong with the files
        extern BASE_RESOURCE_COMPILER_API bool CollectRenameJobs(DepotStructure& depot, const RenameConfiguration& config, IProgressTracker& progress, Array<FileRenameJob*>& outJobs, Array<StringBuf>& outFailedFiles);

        /// rename depot elements (including folders)
        extern BASE_RESOURCE_COMPILER_API void ApplyRenameJobs(DepotStructure& depot, const RenameConfiguration& config, IRenameProgressTracker& progress, const Array<FileRenameJob*>& jobs);

        //--

    } // depot
} // base

