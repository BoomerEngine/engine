/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#pragma once
#include "base/containers/include/hashMap.h"

namespace base
{
    namespace res
    {
        //--
        
        /// semi-cachable source asset that is used in resource importing
        /// NOTE: the source assets MAY be cached (if allowed) to speed up reimport but the usual scenario is that they are kept in the memory for some time to prevent the need of reparsing the file
        class BASE_RESOURCE_COMPILER_API ISourceAsset : public IObject
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ISourceAsset, IObject);

        public:
            ISourceAsset();
            virtual ~ISourceAsset();

            ///---

            /// is the asset file-cachable ?
            virtual bool shouldCacheInFile() const { return false; }

            /// is the asset memory-cachable ?
            virtual bool shouldCacheInMemory() const { return true; }

            /// calculate estimated memory size of the asset
            virtual uint64_t calcMemoryUsage() const = 0;

            /// load the asset from given memory buffer 
            /// NOTE: it's legal for the asset to take ownership of the buffer
            virtual bool loadFromMemory(Buffer data) const = 0;

            ///---
        };

        //--

    } // res
} // base