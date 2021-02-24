/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#pragma once

#include "base/resource/include/resource.h"

BEGIN_BOOMER_NAMESPACE(base::res)

//--

// file entry in the import list, describes source, target and configuration of the import
struct BASE_RESOURCE_COMPILER_API ImportFileEntry
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(ImportFileEntry);

    bool enabled = true;
    StringBuf depotPath;
    StringBuf assetPath;
    ResourceConfigurationPtr userConfiguration;
};

//--

// import list, can be saved as a standalone resource, also importable/exportable to XML for interop
class BASE_RESOURCE_COMPILER_API ImportList : public IResource
{
    RTTI_DECLARE_VIRTUAL_CLASS(ImportList, IResource);

public:
    ImportList();
    ImportList(Array<ImportFileEntry>&& files);
    ImportList(const ImportFileEntry& file);

    /// list of files
    INLINE const Array<ImportFileEntry>& files() const { return m_files; }

    ///---

private:
    Array<ImportFileEntry> m_files;
};

//--

END_BOOMER_NAMESPACE(base::res)
