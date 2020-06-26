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
#include "base/object/include/streamBinaryWriter.h"
#include "base/object/include/streamTextWriter.h"
#include "base/object/include/streamBinaryReader.h"
#include "base/object/include/streamTextReader.h"

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
                data = base::CreateSharedPtr<Table>(std::move(tableToConsume));
        }*/

        bool TableResource::onReadBinary(stream::IBinaryReader& reader)
        {
            if (!TBaseClass::onReadBinary(reader))
                return false;

/*            data = base::CreateSharedPtr<Table>();

            uint8_t hasTable = 0;
            reader >> hasTable;

            if (hasTable)
            {
                base::rtti::TypeSerializationContext classContext;
                classContext.classContext = cls();

                if (!data->readBinary(classContext, reader))
                    return false;
            }*/

            return true;
        }

        bool TableResource::onWriteBinary(stream::IBinaryWriter& writer) const
        {
            if (!TBaseClass::onWriteBinary(writer))
                return false;
/*
            uint8_t hasTable = (data != nullptr);
            writer << hasTable;

            if (hasTable)
            {
                base::rtti::TypeSerializationContext classContext;
                classContext.classContext = cls();

                if (!data->writeBinary(classContext, writer))
                    return false;
            }*/

            return true;
        }

    } // table
} // base


