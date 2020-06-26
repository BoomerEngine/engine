/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\data #]
***/

#pragma once

#include "renderingShaderDataType.h"
#include "base/parser/include/textToken.h"
#include "base/parser/include/textErrorReporter.h"
#include "base/containers/include/hashMap.h"
#include "base/reflection/include/variantTable.h"

namespace rendering
{
    namespace compiler
    {

        struct ResolvedDescriptorInfo;

        /// get composite kind, encodes some special cases for composite types that can be swizzled
        enum class CompositeTypeHint : uint8_t
        {
            User, // this is a user type
            VectorType, // N-component vector
            MatrixType, // NxM-component matrix
        };

        /// packing rules for composite type
        enum class CompositePackingRules : uint8_t
        {
            Vertex, // pack data for vertex shader input assembler, this is the most strict and strange
            Std140, // strict shader rules (old shaders, array elements are forced to 16 bytes alignment, float3 takes 16 bytes, etc)
            Std430, // relaxed shader rules (new shaders, array elements are using 4 bytes alignment), default for structures
        };

        /// composite type (structure)
        /// NOTE: we don't care about offsets here, the placement is abstract
        class RENDERING_COMPILER_API CompositeType : public base::NoCopy
        {
        public:
            CompositeType(base::StringID name, CompositePackingRules packingRules, CompositeTypeHint hint = CompositeTypeHint::User);

            struct MemberLayoutInfo
            {
                uint32_t linearAlignment = 0; // member alignment that was used (or is needed)
                uint32_t linearOffset = 0; // memory offset for this member int the parent structure
                uint32_t linearSize = 0; // physical data size
                uint32_t linearArrayCount = 0; // for arrays this is the number of array elements
                uint32_t linearArrayStride = 0; // for arrays this is the stride of the array

                ImageFormat dataFormat = ImageFormat::UNKNOWN; // specified format, if known
            };

            struct Member
            {
                base::parser::Location location; // location of the member definition
                base::StringID name; // name of the member
                uint32_t firstComponent = 0; // first component in the scalar representation for this member
                DataType type; // type of the member

                AttributeList attributes; // all parsed attributes

                base::Array<base::parser::Token*> initalizationTokens; // initialization tokens (can be a full expression)
                CodeNode* initializerCode = nullptr; // parsed initialization code for the value

                //--

                MemberLayoutInfo layout; // computed layout
            };

            // get the type hint :) allows for easy implementation of GLSL/HLSL style type casts that would be otherwise impossible
            INLINE CompositeTypeHint hint() const { return m_hint; }

            // get the packing rules for the type
            INLINE CompositePackingRules packingRules() const { return m_packing; }

            // get all structure members
            typedef base::Array<Member> TMembers;
            INLINE const TMembers& members() const { return m_members; }

            // get name of the composite type
            INLINE base::StringID name() const { return m_name; }

            // get number of scalar components needed to represent the type
            INLINE uint32_t scalarComponentCount() const { return m_scalarCount; }

            // get type unique hash that describes the type
            INLINE uint64_t typeHash() const { return m_typeHash.crc(); }

            // get linear memory size for this composite type
            INLINE uint32_t linearSize() const { return m_linearSize; }

            // get linear memory alignment for this composite type
            INLINE uint32_t linearAlignment() const { return m_linearAlignment; }

            // find member by name, returns type of the member
            DataType memberType(const base::StringID name) const;

            // get index of member, returns -1 if not found
            int memberIndex(const base::StringID name) const;

            // get member by index
            DataType memberType(uint32_t index) const;

            // get name of member by index
            base::StringID memberName(uint32_t index) const;

            //--

            // add member, fails if member with that name already exists
            void addMember(const base::parser::Location& loc, const base::StringID memberName, DataType memberType, AttributeList&& attributes, const base::Array<base::parser::Token*>& initializationTokens = base::Array<base::parser::Token*>());

            //--

            // dump type data to debug stream
            void print(base::IFormatStream& f) const;

            //--

            // compute data layout for the structure
            bool computeMemoryLayout(bool& outNeedsMorePasses, bool& outUpdated, base::parser::IErrorReporter& err);

        private:
            base::StringID m_name; // name of the type
            TMembers m_members; // members and their types
            base::CRC64 m_typeHash; // type CRC, unique identifier for the type structure, allows to detect types with the same structure
            uint32_t m_scalarCount; // number of scalar components needed to represent the type
            CompositeTypeHint m_hint; // get the internal hint (used for GLSL/HLSL swizzles and mask)
            CompositePackingRules m_packing; // packing rules for the type

            uint32_t m_linearSize; // total size in the memory of this composite type
            uint32_t m_linearAlignment; // required structure alignment
            bool m_layoutComputed; // layout for this member was computed

            //--

            bool packLayoutVertex(const CompositeType::Member& prop, uint32_t& inOutPackingOffset, MemberLayoutInfo& outLayout, base::parser::IErrorReporter& err) const;
            bool packLayoutStd140(const CompositeType::Member& prop, uint32_t& inOutPackingOffset, MemberLayoutInfo& outLayout, base::parser::IErrorReporter& err) const;
            bool packLayoutStd430(const CompositeType::Member& prop, uint32_t& inOutPackingOffset, MemberLayoutInfo& outLayout, base::parser::IErrorReporter& err) const;
        };

        ///---

        struct RENDERING_COMPILER_API ResourceTableEntry
        {
            base::parser::Location m_location; // location of the member definition
            base::StringID m_name; // name of the member
            base::StringID m_mergedName; // descriptor + field, this way it's unique
            DataType m_type; // type of the entry
            AttributeList m_attributes; // used provided attributes

            //ImageFormat m_resourceFormat; // format for the formated buffer
            //ImageType m_resourceImageType; // type for images (textures)
            //const CompositeType* m_resourceLayout; // layout of the constant buffer or structured buffer

            ResourceTableEntry();
        };

        ///---

        /// resource table (descriptor)
        class RENDERING_COMPILER_API ResourceTable : public base::NoCopy
        {
        public:
            ResourceTable(base::StringID name, const AttributeList& attributes);

            // get all structure members
            typedef base::Array<ResourceTableEntry> TMembers;
            INLINE const TMembers& members() const { return m_members; }

            // get name of the composite type
            INLINE base::StringID name() const { return m_name; }

            // assigned attributes
            INLINE const AttributeList& attributes() const { return m_attributes; }

            //--

            // add member, fails if member with that name already exists
            void addMember(const base::parser::Location& loc, const base::StringID name, const DataType& type, const AttributeList& attributes);

            // get index of member, returns -1 if not found
            int memberIndex(const base::StringID name) const;

            // get name of member by index
            base::StringID memberName(uint32_t index) const;

            //--

            // dump type data to debug stream
            void print(base::IFormatStream& f) const;

        private:
            base::StringID m_name; // name of the type
            TMembers m_members; // members and their types
            AttributeList m_attributes; // is thia a material resource layout
        };

        ///---

        /// a type library (describes known types)
        /// owned by shader library as all fragments and functions share types
        /// sharing common type library is simpler when validating fragments working together
        class RENDERING_COMPILER_API TypeLibrary : public base::NoCopy
        {
        public:
            TypeLibrary(base::mem::LinearAllocator& allocator);
            ~TypeLibrary();

            //--

            /// find composite type
            DataType compositeType(const base::StringID name) const;

            // get a boolean type, with optional vector size
            DataType booleanType(uint32_t vectorSize = 1) const;

            // get a signed integer type with optional vector size
            DataType integerType(uint32_t vectorSize = 1) const;

            // get a unsigned type with optional vector size
            DataType unsignedType(uint32_t vectorSize = 1) const;

            // get a floating point type with optional vector size
            DataType floatType(uint32_t vectorSize = 1, uint32_t matrixRows = 1) const;

            // get composite type
            DataType simpleCompositeType(BaseType baseType, uint32_t vectorSize = 1, uint32_t matrixRows = 1) const;

            // get resource type by name (image/sampler/texture etc)
            DataType resourceType(base::StringID typeName, const AttributeList& attributes);

            // get resource type for a constant buffer of given layout
            DataType resourceType(const CompositeType* constantBufferLayout, const AttributeList& attributes);

            // get the element type for a given packed data format, returns float3 for RGB32F, float4 for RGBA8 etc
            DataType packedFormatElementType(ImageFormat type) const;

            //--

            /// declare a composite type
            /// NOTE: fails is the type is already registered 
            DataType registerCompositeType(CompositeType* compositeType);

            /// find composite type by name
            const CompositeType* findCompositeType(base::StringID name) const;

            //--

            /// declare a resource table
            void registerResourceTable(ResourceTable* table);

            /// find resource table by name
            const ResourceTable* findResourceTable(base::StringID name) const;

            //--

            /// get all registered composite types (NOTE: includes the vector types)
            typedef base::Array<const CompositeType*> TCompositeTypes;
            INLINE const TCompositeTypes& allCompositeTypes() const { return m_compositeTypes; }

            /// get all registered resource tables types (NOTE: includes the vector types)
            typedef base::Array<const ResourceTable*> TResourceTables;
            INLINE const TResourceTables& allResourceTables() const { return m_resourceTables; }

            //--

            /// compute sizes of composite types
            bool calculateCompositeLayouts(base::parser::IErrorReporter& err);

        private:
            static const uint32_t MAX_COMPONENTS = 4;

            DataType m_boolTypes[MAX_COMPONENTS];
            DataType m_intTypes[MAX_COMPONENTS];
            DataType m_uintTypes[MAX_COMPONENTS];
            DataType m_floatTypes[MAX_COMPONENTS][MAX_COMPONENTS];

            typedef base::HashMap<base::StringID, const CompositeType*> TCompositeTypeMap;

            TCompositeTypeMap m_compositeTypeMap;
            TCompositeTypes m_compositeTypes;

            typedef base::HashMap<base::StringID, const ResourceTable*> TResourceTableMap;

            TResourceTableMap m_resourceTableMap;
            TResourceTables m_resourceTables;

            typedef base::HashMap<uint64_t, const ResourceType*> TResourceTypeMap;
            TResourceTypeMap m_resourceTypesMap;
            base::Array<ResourceType*> m_resourceTypes;

            base::mem::LinearAllocator& m_allocator;

            //--

            void createDefaultTypes();
        };

    } // shader
} // rendering