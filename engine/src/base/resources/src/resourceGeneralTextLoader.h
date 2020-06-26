/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\text #]
***/

#pragma once

#include "base/object/include/streamTextReader.h"
#include "base/object/include/serializationLoader.h"
#include "base/object/include/streamTextReader.h"
#include "base/xml/include/public.h"
#include "base/containers/include/hashMap.h"
#include "base/containers/include/array.h"

namespace base
{
    namespace res
    {
        namespace text
        {

            /// loader of serialized data from a xml format
            class BASE_RESOURCES_API TextLoader : public stream::ILoader
            {
            public:
                TextLoader();
                virtual ~TextLoader();

                virtual CAN_YIELD bool loadObjects(stream::IBinaryReader& file, const stream::LoadingContext& context, stream::LoadingResult& result) override final;
                virtual bool extractLoadingDependencies(stream::IBinaryReader& file, bool includeAsync, Array<stream::LoadingDependency>& outDependencies) override final;
            };

        } // text
    } // res
} // base
