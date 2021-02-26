/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\types #]
***/

#pragma once

#include "renderingShaderDataType.h"
#include "core/parser/include/textToken.h"
#include "core/parser/include/textErrorReporter.h"
#include "core/containers/include/hashMap.h"
#include "core/reflection/include/variantTable.h"
#include "gpu/device/include/renderingSamplerState.h"
#include "gpu/device/include/renderingGraphicsStates.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::compiler)

class StaticSampler;
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
class GPU_SHADER_COMPILER_API CompositeType : public NoCopy
{
public:
    CompositeType(StringID name, CompositePackingRules packingRules, CompositeTypeHint hint = CompositeTypeHint::User);

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
        parser::Location location; // location of the member definition
        StringID name; // name of the member
        uint32_t firstComponent = 0; // first component in the scalar representation for this member
        DataType type; // type of the member

        AttributeList attributes; // all parsed attributes

        Array<parser::Token*> initalizationTokens; // initialization tokens (can be a full expression)
        CodeNode* initializerCode = nullptr; // parsed initialization code for the value

        //--

        MemberLayoutInfo layout; // computed layout
    };

    // get the type hint :) allows for easy implementation of GLSL/HLSL style type casts that would be otherwise impossible
    INLINE CompositeTypeHint hint() const { return m_hint; }

    // get the packing rules for the type
    INLINE CompositePackingRules packingRules() const { return m_packing; }

    // get all structure members
    typedef Array<Member> TMembers;
    INLINE const TMembers& members() const { return m_members; }

    // get name of the composite type
    INLINE StringID name() const { return m_name; }

    // get number of scalar components needed to represent the type
    INLINE uint32_t scalarComponentCount() const { return m_scalarCount; }

    // get type unique hash that describes the type
    INLINE uint64_t typeHash() const { return m_typeHash.crc(); }

    // get linear memory size for this composite type
    INLINE uint32_t linearSize() const { return m_linearSize; }

    // get linear memory alignment for this composite type
    INLINE uint32_t linearAlignment() const { return m_linearAlignment; }

    // find member by name, returns type of the member
    DataType memberType(const StringID name) const;

    // get index of member, returns -1 if not found
    int memberIndex(const StringID name) const;

    // get member by index
    DataType memberType(uint32_t index) const;

    // get name of member by index
    StringID memberName(uint32_t index) const;

    //--

    // add member, fails if member with that name already exists
    void addMember(const parser::Location& loc, const StringID memberName, DataType memberType, AttributeList&& attributes, const Array<parser::Token*>& initializationTokens = Array<parser::Token*>());

    //--

    // dump type data to debug stream
    void print(IFormatStream& f) const;

    //--

    // compute data layout for the structure
    bool computeMemoryLayout(bool& outNeedsMorePasses, bool& outUpdated, parser::IErrorReporter& err);

private:
    StringID m_name; // name of the type
    TMembers m_members; // members and their types
    CRC64 m_typeHash; // type CRC, unique identifier for the type structure, allows to detect types with the same structure
    uint32_t m_scalarCount; // number of scalar components needed to represent the type
    CompositeTypeHint m_hint; // get the internal hint (used for GLSL/HLSL swizzles and mask)
    CompositePackingRules m_packing; // packing rules for the type

    uint32_t m_linearSize; // total size in the memory of this composite type
    uint32_t m_linearAlignment; // required structure alignment
    bool m_layoutComputed; // layout for this member was computed

    //--

    bool packLayoutVertex(const CompositeType::Member& prop, uint32_t& inOutPackingOffset, MemberLayoutInfo& outLayout, parser::IErrorReporter& err) const;
    bool packLayoutStd140(const CompositeType::Member& prop, uint32_t& inOutPackingOffset, MemberLayoutInfo& outLayout, parser::IErrorReporter& err) const;
    bool packLayoutStd430(const CompositeType::Member& prop, uint32_t& inOutPackingOffset, MemberLayoutInfo& outLayout, parser::IErrorReporter& err) const;
};

///---

struct GPU_SHADER_COMPILER_API ResourceTableEntry
{
    parser::Location m_location; // location of the member definition
    StringID m_name; // name of the member
    DataType m_type; // type of the entry
    AttributeList m_attributes; // used provided attributes

	char m_localSampler = -1;
	const StaticSampler* m_staticSampler = nullptr;

    ResourceTableEntry();
};

///---

/// resource table (descriptor)
class GPU_SHADER_COMPILER_API ResourceTable : public NoCopy
{
public:
    ResourceTable(StringID name, const AttributeList& attributes);

    // get all structure members
    typedef Array<ResourceTableEntry> TMembers;
    INLINE const TMembers& members() const { return m_members; }

    // get name of the composite type
    INLINE StringID name() const { return m_name; }

    // assigned attributes
    INLINE const AttributeList& attributes() const { return m_attributes; }

    //--

    // add member, fails if member with that name already exists
	void addMember(const parser::Location& loc, const StringID name, const DataType& type, const AttributeList& attributes, char localSampler = -1, const StaticSampler* staticSampler = nullptr);


    // get index of member, returns -1 if not found
    int memberIndex(const StringID name) const;

    // get name of member by index
    StringID memberName(uint32_t index) const;

    //--

    // dump type data to debug stream
    void print(IFormatStream& f) const;

private:
    StringID m_name; // name of the type
    TMembers m_members; // members and their types
    AttributeList m_attributes; // is thia a material resource layout
};

///---

/// static sampler
class GPU_SHADER_COMPILER_API StaticSampler : public NoCopy
{
public:
	StaticSampler(StringID name, const SamplerState& state);

	// name of the samples
	INLINE StringID name() const { return m_name; }

	// sampler state
	INLINE const SamplerState& state() const { return m_state; }

private:
	StringID m_name;
	SamplerState m_state;
};

///---

/// static rendering states... DX12/Vulkan BS
class GPU_SHADER_COMPILER_API StaticRenderStates : public NoCopy
{
public:
	StaticRenderStates(StringID name, const GraphicsRenderStatesSetup& state);

	// name of the samples
	INLINE StringID name() const { return m_name; }

	// sampler state
	INLINE const GraphicsRenderStatesSetup& state() const { return m_state; }

private:
	StringID m_name;
	GraphicsRenderStatesSetup m_state;
};

///---

/// a type library (describes known types)
/// owned by shader library as all fragments and functions share types
/// sharing common type library is simpler when validating fragments working together
class GPU_SHADER_COMPILER_API TypeLibrary : public NoCopy
{
public:
    TypeLibrary(mem::LinearAllocator& allocator);
    ~TypeLibrary();

    //--

    /// find composite type
    DataType compositeType(const StringID name) const;

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
    DataType resourceType(StringID typeName, const AttributeList& attributes, StringBuf& outError);

    // get resource type for a constant buffer of given layout
    DataType resourceType(const CompositeType* constantBufferLayout, const AttributeList& attributes);

    // get the element type for a given packed data format, returns float3 for RGB32F, float4 for RGBA8 etc
    DataType packedFormatElementType(ImageFormat type) const;

    //--

    /// declare a composite type
    /// NOTE: fails is the type is already registered 
    DataType registerCompositeType(CompositeType* compositeType);

    /// find composite type by name
    const CompositeType* findCompositeType(StringID name) const;

    //--

    /// declare a resource table
    void registerResourceTable(ResourceTable* table);

    /// find resource table by name
    const ResourceTable* findResourceTable(StringID name) const;

	///--

	/// declare static sampler
	void registerStaticSampler(StaticSampler* sampler);

	/// find static sampler by name
	const StaticSampler* findStaticSampler(StringID name) const;

	///--

	/// declare render states
	void registerStaticRenderStates(StaticRenderStates* states);

	/// find static sampler by name
	const StaticRenderStates* findStaticRenderStates(StringID name) const;

    //--

    /// get all registered composite types (NOTE: includes the vector types)
    typedef Array<const CompositeType*> TCompositeTypes;
    INLINE const TCompositeTypes& allCompositeTypes() const { return m_compositeTypes; }

    /// get all registered resource tables types
    typedef Array<const ResourceTable*> TResourceTables;
    INLINE const TResourceTables& allResourceTables() const { return m_resourceTables; }

	/// get all registered static samplers
	typedef Array<const StaticSampler*> TStaticSamplers;
	INLINE const TStaticSamplers& allStaticSamplers() const { return m_staticSamplers; }

	/// get all registered static render states
	typedef Array<const StaticRenderStates*> TStaticRenderStates;
	INLINE const TStaticRenderStates& allStaticRenderStates() const { return m_staticRenderStates; }

    //--

    /// compute sizes of composite types
    bool calculateCompositeLayouts(parser::IErrorReporter& err);

private:
    static const uint32_t MAX_COMPONENTS = 4;

    DataType m_boolTypes[MAX_COMPONENTS];
    DataType m_intTypes[MAX_COMPONENTS];
    DataType m_uintTypes[MAX_COMPONENTS];
    DataType m_floatTypes[MAX_COMPONENTS][MAX_COMPONENTS];

    typedef HashMap<StringID, const CompositeType*> TCompositeTypeMap;
    TCompositeTypeMap m_compositeTypeMap;
    TCompositeTypes m_compositeTypes;

    typedef HashMap<StringID, const ResourceTable*> TResourceTableMap;
    TResourceTableMap m_resourceTableMap;
    TResourceTables m_resourceTables;

	typedef HashMap<StringID, const StaticSampler*> TStaticSamplerMap;
	TStaticSamplerMap m_staticSamplerMap;
	TStaticSamplers m_staticSamplers;

	typedef HashMap<StringID, const StaticRenderStates*> TStaticRenderStatesMap;
	TStaticRenderStatesMap m_staticRenderStatesMap;
	TStaticRenderStates m_staticRenderStates;

    typedef HashMap<uint64_t, const ResourceType*> TResourceTypeMap;
    TResourceTypeMap m_resourceTypesMap;
    Array<ResourceType*> m_resourceTypes;

	mem::LinearAllocator& m_allocator;

    //--

    void createDefaultTypes();
};

END_BOOMER_NAMESPACE_EX(gpu::compiler)
