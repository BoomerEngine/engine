/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "replicationFieldPacking.h"
#include "replicationRttiExtensions.h"

BEGIN_BOOMER_NAMESPACE(base::replication)

//--

RTTI_BEGIN_TYPE_CLASS(SetupMetadata);
RTTI_END_TYPE();

SetupMetadata::SetupMetadata()
{}

SetupMetadata::SetupMetadata(const char* packingString)
    : m_packing(packingString)
{}

SetupMetadata::SetupMetadata(const FieldPacking& packing)
    : m_packing(packing)
{}

//--

END_BOOMER_NAMESPACE(base::replication)
