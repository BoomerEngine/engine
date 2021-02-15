/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "replicationFieldPacking.h"

namespace base
{
    namespace replication
    {

        ///----

        /// mark property as replicated
        class BASE_REPLICATION_API SetupMetadata : public rtti::IMetadata
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SetupMetadata, rtti::IMetadata);

        public:
            SetupMetadata();
            SetupMetadata(const char* packingString);
            SetupMetadata(const FieldPacking& packing);

            INLINE const FieldPacking& packing() const
            {
                return m_packing;
            }

        private:
            FieldPacking m_packing;
        };

        ///----

    } // replication
} // base
