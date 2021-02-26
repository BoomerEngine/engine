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
#include "resourceAsyncReferenceType.h"

DECLARE_MODULE(PROJECT_NAME)
{
    boomer::RTTI::GetInstance().registerDynamicTypeCreator(boomer::res::ResourceRefType::TypePrefix, &boomer::res::ResourceRefType::ParseType);
    boomer::RTTI::GetInstance().registerDynamicTypeCreator(boomer::res::ResourceAsyncRefType::TypePrefix, &boomer::res::ResourceAsyncRefType::ParseType);
    boomer::IObject::RegisterCloneFunction(&boomer::CloneObjectUntyped);
    boomer::IObject::RegisterSerializeFunction(&boomer::SaveObjectToBuffer);
    boomer::IObject::RegisterDeserializeFunction(&boomer::LoadObjectFromBuffer);
}
