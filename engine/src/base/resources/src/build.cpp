/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# dependency: base_system, base_memory, base_containers, base_io #]
* [# dependency: base_object, base_reflection, base_fibers, base_xml #]
* [# dependency: base_config, base_math, base_app, base_parser #]
***/

#include "build.h"
#include "reflection.inl"
#include "static_init.inl"

#include "resourceReferenceType.h"

DECLARE_MODULE(PROJECT_NAME)
{
    RTTI::GetInstance().registerDynamicTypeCreator(base::res::ResourceRefType::TypePrefix, &base::res::ResourceRefType::ParseType);
    base::IObject::RegisterCloneFunction(&base::CloneObjectUntyped);
    base::IObject::RegisterSerializeFunction(&base::SaveObjectToBuffer);
    base::IObject::RegisterDeserializeFunction(&base::LoadObjectFromBuffer);
}
