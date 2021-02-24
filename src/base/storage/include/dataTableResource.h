/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: config #]
***/

#pragma once

#include "base/resource/include/resource.h"

BEGIN_BOOMER_NAMESPACE(base::storage)

/// resource that contains cooked data table, can be built from various sources, even scripts
class BASE_STORAGE_API TableResource : public res::IResource
{
    RTTI_DECLARE_VIRTUAL_CLASS(TableResource, res::IResource);

public:
    TableResource();

    // get the data table container
    //INLINE const TablePtr& table() const { return data; }

    //--

    // set new content
    // NOTE: will gut existing data and place new data inside
    //void content(Table&& tableToConsume);

private:

    //TablePtr data;
};

END_BOOMER_NAMESPACE(base::storage)
