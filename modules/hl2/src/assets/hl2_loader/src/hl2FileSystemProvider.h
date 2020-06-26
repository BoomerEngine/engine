/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
***/

#pragma once

#include "base/depot/include/depotFileSystem.h"

namespace hl2
{

    // provider of the HL2 file system mounting from installed VPKs
    struct FileSystemProviderVPK : public base::depot::IFileSystemProvider
    {
        RTTI_DECLARE_VIRTUAL_CLASS(FileSystemProviderVPK, base::depot::IFileSystemProvider);

    public:
        FileSystemProviderVPK();
        virtual base::UniquePtr<base::depot::IFileSystem> createFileSystem(base::depot::DepotStructure* owner) const override;
    };

} // hl2