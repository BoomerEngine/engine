/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "loader.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(LoadingService);
RTTI_END_TYPE();

LoadingService::LoadingService()
{}

//--

ResourcePtr LoadResource(StringView path, ClassType expectedClass)
{
    return GetService<LoadingService>()->loadResource(path, expectedClass);
}

ResourcePtr LoadResource(const ResourceID& id, ClassType expectedClass)
{
    return GetService<LoadingService>()->loadResource(id, expectedClass);
}

//--

END_BOOMER_NAMESPACE()