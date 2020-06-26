/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: materials #]
***/

#include "build.h"
#include "physicsMaterialRef.h"

#include "base/object/include/streamTextWriter.h"
#include "base/object/include/streamTextReader.h"
#include "base/object/include/streamBinaryWriter.h"
#include "base/object/include/streamBinaryReader.h"

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

        bool MaterialReference::writeBinary(const base::rtti::TypeSerializationContext& typeContext, base::stream::IBinaryWriter& stream) const
        {
            stream.writeName(m_name);
            return true;
        }

        bool MaterialReference::writeText(const base::rtti::TypeSerializationContext& typeContext, base::stream::ITextWriter& stream) const
        {
            stream.writeValue(m_name.view());
            return true;
        }

        bool MaterialReference::readBinary(const base::rtti::TypeSerializationContext& typeContext, base::stream::IBinaryReader& stream)
        {
            m_name = stream.readName();
            return true;
        }

        bool MaterialReference::readText(const base::rtti::TypeSerializationContext& typeContext, base::stream::ITextReader& stream)
        {
            base::StringView<char> txt;
            if (!stream.readValue(txt))
            {
                TRACE_WARNING("Unable to load string data when string data was expected");
                return true; // let's try to continue
            }

            m_name = base::StringID(txt);
            return true;
        }

        void MaterialReference::calcHash(base::CRC64& crc) const
        {
            crc << m_name;
        }

        //--

    } // data
} // physics


