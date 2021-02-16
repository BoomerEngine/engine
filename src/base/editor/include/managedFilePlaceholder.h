/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#pragma once

#include "managedItem.h"

namespace ed
{
    //---

    //---

    // placeholder for a new resource file created by user
    class BASE_EDITOR_API ManagedFilePlaceholder : public ManagedItem
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ManagedFilePlaceholder, ManagedItem);

    public:
        ManagedFilePlaceholder(ManagedDepot* depot, ManagedDirectory* parentDir, StringView initialFileName, const ManagedFileFormat* format);
        virtual ~ManagedFilePlaceholder();

        //--

        INLINE const ManagedFileFormat& format() const { return *m_fileFormat; }
        INLINE const StringBuf& shortName() const { return m_shortName; }

        //--

        /// Get type (resource type) thumbnail, can be used when file thumbnail is not loaded
        virtual const image::Image* typeThumbnail() const override;

        //---

        /// set name of the placeholder file
        void rename(StringView name);

    protected:
        const ManagedFileFormat* m_fileFormat; // file format description
        StringBuf m_shortName; // file name without extension
    };

    //---

} // editor

