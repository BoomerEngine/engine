/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\xml #]
***/

#pragma once

#include "base/object/include/serializationLoader.h"

namespace base
{
    namespace res
    {
        namespace xml
        {

            /// loader of serialized data from a xml format
            class BASE_RESOURCE_API XMLLoader : public stream::ILoader
            {
            public:
                XMLLoader();
                virtual ~XMLLoader();

                virtual CAN_YIELD bool loadObjects(stream::IBinaryReader& file, const stream::LoadingContext& context, stream::LoadingResult& result) override final;
                virtual bool extractLoadingDependencies(stream::IBinaryReader& file, bool includeAsync, Array<stream::LoadingDependency>& outDependencies) override final;
            };

        } // binary
    } // res
} // base
