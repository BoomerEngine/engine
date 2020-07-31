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

    // placeholder for a new directory created by user
    class BASE_EDITOR_API ManagedDirectoryPlaceholder : public ManagedItem
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ManagedDirectoryPlaceholder, ManagedItem);

    public:
        ManagedDirectoryPlaceholder(ManagedDepot* depot, ManagedDirectory* parentDir, StringView<char> initialFileName);
        virtual ~ManagedDirectoryPlaceholder();

        //--

        /// Get type (resource type) thumbnail, can be used when file thumbnail is not loaded
        virtual const image::ImageRef& typeThumbnail() const override;

        //---

        /// set name of the placeholder file
        void rename(StringView<char> name);

    private:
        image::ImageRef m_directoryIcon;
    };

    //---

} // editor

