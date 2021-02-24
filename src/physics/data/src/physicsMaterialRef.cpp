/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: materials #]
***/

#include "build.h"
#include "physicsMaterialRef.h"

#include "base/object/include/streamOpcodeWriter.h"
#include "base/object/include/streamOpcodeReader.h"

BEGIN_BOOMER_NAMESPACE(boomer)

RTTI_BEGIN_CUSTOM_TYPE(PhysicsMaterialReference);
    RTTI_TYPE_TRAIT().zeroInitializationValid().noDestructor().fastCopyCompare();
RTTI_END_TYPE();

//--

static PhysicsMaterialReference theDefaultMaterialName("Default"_id);

//--

PhysicsMaterialReference::PhysicsMaterialReference()
    : m_name("Default"_id)
{}

PhysicsMaterialReference::PhysicsMaterialReference(base::StringID materialName)
    : m_name(materialName)
{}

const PhysicsMaterialReference& PhysicsMaterialReference::DEFAULT()
{
    return theDefaultMaterialName;
}

const PhysicsMaterialDefinition& PhysicsMaterialReference::GetDefinition() const
{
    return *(const MaterialDefinition*)nullptr;
}

void PhysicsMaterialReference::writeBinary(base::rtti::TypeSerializationContext& typeContext, base::stream::OpcodeWriter& stream) const
{
    stream.writeStringID(m_name);
}

void PhysicsMaterialReference::readBinary(base::rtti::TypeSerializationContext& typeContext, base::stream::OpcodeReader& stream)
{
    m_name = stream.readStringID();
}

void PhysicsMaterialReference::calcHash(base::CRC64& crc) const
{
    crc << m_name;
}

//--

END_BOOMER_NAMESPACE(boomer)



