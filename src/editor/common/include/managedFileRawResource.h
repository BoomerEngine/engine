/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#pragma once

#include "managedFile.h"
#include "core/object/include/globalEventKey.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

// file in the depot that is not directly loaded by the engine, usually has a text format
class EDITOR_COMMON_API ManagedFileRawResource : public ManagedFile
{
    RTTI_DECLARE_VIRTUAL_CLASS(ManagedFileRawResource, ManagedFile);

public:
    ManagedFileRawResource(ManagedDepot* depot, ManagedDirectory* parentDir, StringView fileName);
    virtual ~ManagedFileRawResource();

    //--

    // load content for this file for edition, loads it as untyped buffer
    // NOTE: this function will always load a new content
    Buffer loadContent();

    // save new content of the file (clears the modified flag as well)
    bool storeContent(const Buffer& content);
};

//---

END_BOOMER_NAMESPACE_EX(ed)

