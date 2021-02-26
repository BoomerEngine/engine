/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "replicationFieldPacking.h"

BEGIN_BOOMER_NAMESPACE_EX(replication)

///----

/// mark property as replicated
class CORE_REPLICATION_API SetupMetadata : public rtti::IMetadata
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

END_BOOMER_NAMESPACE_EX(replication)
