/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base_depot_glue.inl"

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
} // base
