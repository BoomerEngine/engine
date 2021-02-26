/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# dependency: base_system, base_memory, base_containers, base_io #]
* [# dependency: base_fibers, base_test, base_xml #]
***/

#include "build.h"

#include "rttiMetadata.h"
#include "object.h"
#include "object.h"

DECLARE_MODULE(PROJECT_NAME)
{
    boomer::rtti::IMetadata::GetStaticClass();
    boomer::IObject::GetStaticClass();
}
