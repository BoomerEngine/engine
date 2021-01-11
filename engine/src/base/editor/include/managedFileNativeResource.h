/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#pragma once

#include "managedFile.h"
#include "base/object/include/globalEventKey.h"

namespace ed
{

    //---

    // file in the depot that can be DIRECTLY loaded to a resource of sorts (without cooking, etc)
    class BASE_EDITOR_API ManagedFileNativeResource : public ManagedFile
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ManagedFileNativeResource, ManagedFile);

    public:
        /// resource class this file can be loaded into (shorthandl for fileFormat().nativeResoureClass())
        INLINE SpecificClassType<res::IResource> resourceClass() const { return m_resourceNativeClass; }

    public:
        ManagedFileNativeResource(ManagedDepot* depot, ManagedDirectory* parentDir, StringView fileName);
        virtual ~ManagedFileNativeResource();

        //---

        // load resource metadata, valid only for serialized resources
        res::MetadataPtr loadMetadata() const;

        /// check if the file can be reimported
        bool canReimport() const;

        //---

        // load content for this file for edition
        // NOTE: this will return previously loaded content if resource is still opened somewhere
        res::ResourcePtr loadContent() const;

        // save resource back to file, resets the modified flag
        bool storeContent(const res::ResourcePtr& content);

        // discard loaded content, usually done when deleting file
        void discardContent();

        //--

        // is the file in use
        virtual bool inUse() const override;

    protected:
        SpecificClassType<res::IResource> m_resourceNativeClass;

        GlobalEventTable m_fileEvents;
    };

    //---

} // editor

