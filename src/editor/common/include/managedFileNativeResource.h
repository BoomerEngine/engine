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

// file in the depot that can be DIRECTLY loaded to a resource of sorts (without cooking, etc)
class EDITOR_COMMON_API ManagedFileNativeResource : public ManagedFile
{
    RTTI_DECLARE_VIRTUAL_CLASS(ManagedFileNativeResource, ManagedFile);

public:
    /// resource class this file can be loaded into (shorthandl for fileFormat().nativeResoureClass())
    INLINE SpecificClassType<IResource> resourceClass() const { return m_resourceNativeClass; }

public:
    ManagedFileNativeResource(ManagedDepot* depot, ManagedDirectory* parentDir, StringView fileName);
    virtual ~ManagedFileNativeResource();

    //---

    // load resource metadata, valid only for serialized resources
    ResourceMetadataPtr loadMetadata() const;

    /// check if the file can be reimported
    bool canReimport() const;

    //---

    // load content for this file for edition
    // NOTE: this will return previously loaded content if resource is still opened somewhere
    ResourcePtr loadContent() const;

    // save resource back to file, resets the modified flag
    bool storeContent(const ResourcePtr& content);

    // discard loaded content, usually done when deleting file
    void discardContent();

protected:
    SpecificClassType<IResource> m_resourceNativeClass;

    GlobalEventTable m_fileEvents;
};

//---

END_BOOMER_NAMESPACE_EX(ed)

