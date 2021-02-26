/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#pragma once

#include "managedItem.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

// placeholder for a new directory created by user
class EDITOR_COMMON_API ManagedDirectoryPlaceholder : public ManagedItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(ManagedDirectoryPlaceholder, ManagedItem);

public:
    ManagedDirectoryPlaceholder(ManagedDepot* depot, ManagedDirectory* parentDir, StringView initialFileName);
    virtual ~ManagedDirectoryPlaceholder();

    //--

    /// Get type (resource type) thumbnail, can be used when file thumbnail is not loaded
    virtual const image::Image* typeThumbnail() const override;

    //---

    /// set name of the placeholder file
    void rename(StringView name);

private:
    image::ImagePtr m_directoryIcon;
};

//---

END_BOOMER_NAMESPACE_EX(ed)

