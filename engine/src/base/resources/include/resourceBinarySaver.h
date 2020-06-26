/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\binary #]
***/

#pragma once

#include "base/object/include/serializationSaver.h"

namespace base
{
    namespace res
    {
        namespace binary
        {
            class StructureMapper;
            class FileTablesBuilder;

            /// Binary saver
            class BASE_RESOURCES_API BinarySaver : public stream::ISaver
            {
            public:
                BinarySaver();

                // ISaver
                virtual bool saveObjects(stream::IBinaryWriter& file, const stream::SavingContext& context) override final;

            private:
                static void SetupNames(StructureMapper& mapper, class FileTablesBuilder& builder);
                static void SetupImports(StructureMapper& mapper, class FileTablesBuilder& builder);
                static void SetupExports(StructureMapper& mapper, class FileTablesBuilder& builder);
                static void SetupBuffers(StructureMapper& mapper, class FileTablesBuilder& builder);
                static void SetupProperties(StructureMapper& mapper, class FileTablesBuilder& builder);

                bool writeExports(stream::IBinaryWriter& file, const stream::SavingContext& context, const StructureMapper& mapper, FileTablesBuilder& tables, uint64_t headerOffset);
                bool writeBuffers(stream::IBinaryWriter& file, const stream::SavingContext& context, const StructureMapper& mapper, FileTablesBuilder& tables, uint64_t headerOffset);
            };

        } // binary
    } // res
} // base
