/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\binary #]
***/

#pragma once

#include "base/object/include/streamBinaryReader.h"
#include "base/object/include/serializationLoader.h"

namespace base
{
    namespace res
    {
        namespace binary
        {

            class FileTables;
            class RuntimeTables;

            /// loader of serialized data from a binary format
            class BASE_RESOURCE_API BinaryLoader : public stream::ILoader
            {
            public:
                BinaryLoader();
                virtual ~BinaryLoader();

                virtual CAN_YIELD bool loadObjects(stream::IBinaryReader& file, const stream::LoadingContext& context, stream::LoadingResult& result) override final;
                virtual bool extractLoadingDependencies(stream::IBinaryReader& file, bool includeAsync, Array<stream::LoadingDependency>& outDependencies) override final;

            private:
                // validate data tables (CRC)
                static bool ValidateTables(uint64_t baseOffset, stream::IBinaryReader& file, const FileTables& fileTables);
            };

        } // binary
    } // res
} // base
