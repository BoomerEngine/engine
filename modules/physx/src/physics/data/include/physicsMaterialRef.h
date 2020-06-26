/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: materials #]
***/

#pragma once

namespace physics
{
    namespace data
    {

        class MaterialDefinition;

        /// reference to physical material, just a name but can contain a twist
        class PHYSICS_DATA_API MaterialReference
        {
        public:
            MaterialReference(); // empty material, if used directly works as default material, normally used to signal "no material override"
            MaterialReference(const MaterialReference& material) = default;
            MaterialReference(MaterialReference&& material) = default;
            MaterialReference(base::StringID materialName);

            MaterialReference& operator=(const MaterialReference& material) = default;
            MaterialReference& operator=(MaterialReference&& material) = default;

            // is this an default (empty) material ?
            INLINE bool empty() const { return m_name.empty(); }

            // get material name
            INLINE base::StringID name() const { return m_name; }

            // ordering operator for maps
            INLINE bool operator<(const MaterialReference& other) const { return m_name < other.m_name; }
            INLINE bool operator==(const MaterialReference& other) const { return m_name == other.m_name; }
            INLINE bool operator!=(const MaterialReference& other) const { return m_name != other.m_name; }

            // get a "Default" physical material that can be used when everything else fails
            static const MaterialReference& DEFAULT();

            // get definition of material referenced by this name, uses the material library to resolve the material
            // NOTE: returns default material in case of any problems
            const MaterialDefinition& GetDefinition() const;

            //--

            // Custom type implementation requirements
            bool writeBinary(const base::rtti::TypeSerializationContext& typeContext, base::stream::IBinaryWriter& stream) const;
            bool writeText(const base::rtti::TypeSerializationContext& typeContext, base::stream::ITextWriter& stream) const;
            bool readBinary(const base::rtti::TypeSerializationContext& typeContext, base::stream::IBinaryReader& stream);
            bool readText(const base::rtti::TypeSerializationContext& typeContext, base::stream::ITextReader& stream);

            // Calculate hash of the data
            void calcHash(base::CRC64& crc) const;

            //--

        private:
            base::StringID m_name; // name of the referenced material
        };

    } // data
} // physics

