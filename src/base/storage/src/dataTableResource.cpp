/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: config #]
***/

#include "build.h"
#include "dataTable.h"
#include "dataTableBuilder.h"
#include "dataTableResource.h"
#include "base/resource/include/resourceTags.h"

namespace base
{
    namespace table
    {

        RTTI_BEGIN_TYPE_CLASS(TableResource);
            RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4db");
            RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("Cooked Data Table");
        RTTI_END_TYPE();

        TableResource::TableResource()
        {}

        /*void TableResource::content(Table&& tableToConsume)
        {
            if (data)
                data->extract(std::move(tableToConsume));
            else
                data = base::RefNew<Table>(std::move(tableToConsume));
        }*/

    } // table
} // base


