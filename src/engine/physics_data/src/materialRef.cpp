/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: materials #]
***/

#include "build.h"
#include "materialRef.h"

#include "core/object/include/streamOpcodeWriter.h"
#include "core/object/include/streamOpcodeReader.h"

BEGIN_BOOMER_NAMESPACE()

RTTI_BEGIN_CUSTOM_TYPE(PhysicsMaterialReference);
    RTTI_TYPE_TRAIT().zeroInitializationValid().noDestructor().fastCopyCompare();
RTTI_END_TYPE();

//--

static PhysicsMaterialReference theDefaultMaterialName("Default"_id);

//--

PhysicsMaterialReference::PhysicsMaterialReference()
    : m_name("Default"_id)
{}

PhysicsMaterialReference::PhysicsMaterialReference(StringID materialName)
    : m_name(materialName)
{}

const PhysicsMaterialReference& PhysicsMaterialReference::DEFAULT()
{
    return theDefaultMaterialName;
}

const PhysicsMaterialDefinition& PhysicsMaterialReference::GetDefinition() const
{
    return *(const PhysicsMaterialDefinition*)nullptr;
}

void PhysicsMaterialReference::writeBinary(TypeSerializationContext& typeContext, stream::OpcodeWriter& stream) const
{
    stream.writeStringID(m_name);
}

void PhysicsMaterialReference::readBinary(TypeSerializationContext& typeContext, stream::OpcodeReader& stream)
{
    m_name = stream.readStringID();
}

void PhysicsMaterialReference::calcHash(CRC64& crc) const
{
    crc << m_name;
}

//--

END_BOOMER_NAMESPACE()



