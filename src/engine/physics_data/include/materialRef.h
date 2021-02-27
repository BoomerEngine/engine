/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: materials #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

class PhysicsMaterialDefinition;

/// reference to physical material, just a name but can contain a twist
class ENGINE_PHYSICS_DATA_API PhysicsMaterialReference
{
public:
    PhysicsMaterialReference(); // empty material, if used directly works as default material, normally used to signal "no material override"
    PhysicsMaterialReference(const PhysicsMaterialReference& material) = default;
    PhysicsMaterialReference(PhysicsMaterialReference&& material) = default;
    PhysicsMaterialReference(StringID materialName);

    PhysicsMaterialReference& operator=(const PhysicsMaterialReference& material) = default;
    PhysicsMaterialReference& operator=(PhysicsMaterialReference&& material) = default;

    // is this an default (empty) material ?
    INLINE bool empty() const { return m_name.empty(); }

    // get material name
    INLINE StringID name() const { return m_name; }

    // ordering operator for maps
    INLINE bool operator<(const PhysicsMaterialReference& other) const { return m_name < other.m_name; }
    INLINE bool operator==(const PhysicsMaterialReference& other) const { return m_name == other.m_name; }
    INLINE bool operator!=(const PhysicsMaterialReference& other) const { return m_name != other.m_name; }

    // get a "Default" physical material that can be used when everything else fails
    static const PhysicsMaterialReference& DEFAULT();

    // get definition of material referenced by this name, uses the material library to resolve the material
    // NOTE: returns default material in case of any problems
    const PhysicsMaterialDefinition& GetDefinition() const;

    //--

    // Custom type implementation requirements
    void writeBinary(rtti::TypeSerializationContext& typeContext, stream::OpcodeWriter& stream) const;
    void readBinary(rtti::TypeSerializationContext& typeContext, stream::OpcodeReader& stream);

    // Calculate hash of the data
    void calcHash(CRC64& crc) const;

    //--

private:
    StringID m_name; // name of the referenced material
};

END_BOOMER_NAMESPACE()
