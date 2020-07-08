/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#pragma once

#include "versionControl.h"

#include "base/io/include/absolutePath.h"
#include "base/object/include/objectObserver.h"
#include "base/resource/include/resourceMountPoint.h"

namespace ed
{

    //---

    // item in depot directory
    class BASE_EDITOR_API ManagedItem : public IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ManagedItem, IObject);

    public:
        /// get the managed depot
        INLINE ManagedDepot* depot() const { return m_depot; }

        /// get the parent managed directory
        INLINE ManagedDirectory* parentDirectory() const { return m_directory; }

        /// get the file name with extension
        INLINE const StringBuf& name() const { return m_name; }

        /// has the file been marked as deleted ?
        INLINE bool isDeleted() const { return m_isDeleted; }

    public:
        ManagedItem(ManagedDepot* depot, ManagedDirectory* parentDir, StringView<char> name);
            
        /// get depot file path of this file
        /// NOTE: file path is constructed on demand (not a cheap call)
        virtual StringBuf depotPath() const;

        /// get the mounting point for this resource, returns the path to the nearest package mount
        /// ie, for engine/textures/checker_d.png this returns engine/ as this is where the package resides
        virtual res::ResourceMountPoint mountPoint() const;

        /// resolve absolute file path for this file, valid only for files coming from disk
        virtual io::AbsolutePath absolutePath() const;

        /// query thumbnail data, returns true if data changed
        virtual bool fetchThumbnailData(uint32_t& versionToken, image::ImageRef& outThumbnailImage, Array<StringBuf>& outComments) const = 0;

        /// get the per-type thumbnail, does not depend on the content of the item
        virtual const image::ImageRef& typeThumbnail() const = 0;

        //--

        // validate file name (note: no extension here)
        static bool ValidateName(StringView<char> txt);

    protected:
        ManagedDepot* m_depot; // depot
        ManagedDirectory* m_directory; // parent directory

        StringBuf m_name; // file name with extension

        const ManagedFileFormat* m_fileFormat; // file format description

        bool m_isDeleted:1; // we don't remove the file proxies, just mark them as deleted

        virtual ~ManagedItem();

        friend class ManagedDepot;
        friend class ManagedDirectory;
    };

    //---

} // ed

