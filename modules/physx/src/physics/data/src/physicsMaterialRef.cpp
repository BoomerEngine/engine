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

namespace physics
{
    namespace data
    {

        RTTI_BEGIN_CUSTOM_TYPE(MaterialReference);
            RTTI_TYPE_TRAIT().zeroInitializationValid().noDestructor().fastCopyCompare();
        RTTI_END_TYPE();

        //--

        static MaterialReference theDefaultMaterialName("Default"_id);

        //--

        MaterialReference::MaterialReference()
            : m_name("Default"_id)
        {}

        MaterialReference::MaterialReference(base::StringID materialName)
            : m_name(materialName)
        {}

        const MaterialReference& MaterialReference::DEFAULT()
        {
            return theDefaultMaterialName;
        }

        const MaterialDefinition& MaterialReference::GetDefinition() const
        {
            return *(const MaterialDefinition*)nullptr;
        }

        void MaterialReference::writeBinary(base::rtti::TypeSerializationContext& typeContext, base::stream::OpcodeWriter& stream) const
        {
            stream.writeStringID(m_name);
        }

        void MaterialReference::readBinary(base::rtti::TypeSerializationContext& typeContext, base::stream::OpcodeReader& stream)
        {
            m_name = stream.readStringID();
        }

        void MaterialReference::calcHash(base::CRC64& crc) const
        {
            crc << m_name;
        }

        //--

    } // data
} // physics


