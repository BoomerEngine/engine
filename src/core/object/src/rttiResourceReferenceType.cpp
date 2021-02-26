/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types #]
***/

#include "build.h"
#include "rttiResourceReferenceType.h"

BEGIN_BOOMER_NAMESPACE_EX(rtti)

//--

IResourceReferenceType::IResourceReferenceType(StringID name)
    : IType(name)
{}

IResourceReferenceType::~IResourceReferenceType()
{}

//--

END_BOOMER_NAMESPACE_EX(rtti)
