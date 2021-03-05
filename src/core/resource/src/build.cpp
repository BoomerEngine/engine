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
#include "static_init.inl"

#include "id.h"
#include "referenceType.h"
#include "asyncReferenceType.h"

DECLARE_MODULE(PROJECT_NAME)
{
    boomer::RTTI::GetInstance().registerDynamicTypeCreator(boomer::ResourceRefType::TypePrefix, &boomer::ResourceRefType::ParseType);
    boomer::RTTI::GetInstance().registerDynamicTypeCreator(boomer::ResourceAsyncRefType::TypePrefix, &boomer::ResourceAsyncRefType::ParseType);
    boomer::IObject::RegisterCloneFunction(&boomer::CloneObjectUntyped);
    boomer::IObject::RegisterSerializeFunction(&boomer::SaveObjectToBuffer);
    boomer::IObject::RegisterDeserializeFunction(&boomer::LoadObjectFromBuffer);

    auto id = boomer::ResourceID::Create();
    auto txt = boomer::StringBuf(boomer::TempString("{}", id));

    boomer::ResourceID id2;
    boomer::ResourceID::Parse(txt, id2);

    TRACE_WARNING("{} == {}", id, id2);
}
