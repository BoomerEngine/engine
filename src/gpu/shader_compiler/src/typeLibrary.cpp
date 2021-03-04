/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\types #]
***/

#include "build.h"

#include "typeLibrary.h"
#include "dataType.h"

#include "core/containers/include/stringBuilder.h"
#include "core/memory/include/linearAllocator.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::compiler)

//---

CompositeType::CompositeType(StringID name, CompositePackingRules packingRules, CompositeTypeHint hint)
    : m_name(name)
    , m_hint(hint)
    , m_typeHash(0)
    , m_scalarCount(0)
    , m_packing(packingRules)
    , m_linearSize(0)
    , m_layoutComputed(false)
{}

DataType CompositeType::memberType(const StringID name) const
{
    for (auto& member : m_members)
        if (member.name == name)
            return member.type;

    return DataType();
}

int CompositeType::memberIndex(const StringID name) const
{
    for (uint32_t i = 0; i < m_members.size(); ++i)
        if (m_members[i].name == name)
            return i;

    return -1;
}

DataType CompositeType::memberType(uint32_t index) const
{
    if (index < m_members.size())
        return m_members[index].type;

    return DataType();
}

StringID CompositeType::memberName(uint32_t index) const
{
    if (index < m_members.size())
        return m_members[index].name;

    return StringID();
}

void CompositeType::addMember(const parser::Location& loc, StringID memberName, DataType memberType, AttributeList&& attributes, const Array<parser::Token*>& initializationTokens)
{
    ASSERT(!memberName.empty());
    ASSERT(memberType.valid());
    ASSERT(!this->memberType(memberName).valid());

    Member info;
    info.location = loc;
    info.firstComponent = m_scalarCount;
    info.name = memberName;
    info.type = memberType;
    info.attributes = std::move(attributes);
    info.initalizationTokens = initializationTokens;
    m_members.pushBack(info);

    m_scalarCount += memberType.computeScalarComponentCount();

    m_typeHash << memberName.c_str();
    memberType.calcTypeHash(m_typeHash);
}

//


static ImageFormat FindPackedTypeForDataType(const DataType& dataType)
{
    if (dataType.isScalar())
    {
        if (dataType.baseType() == BaseType::Float)
            return ImageFormat::R32F;
        else if (dataType.baseType() == BaseType::Int)
            return ImageFormat::R32_INT;
        else if (dataType.baseType() == BaseType::Uint || dataType.baseType() == BaseType::Boolean || dataType.baseType() == BaseType::Name)
            return ImageFormat::R32_UINT;
        else
            return ImageFormat::UNKNOWN;
    }
    else if (dataType.isComposite() && dataType.composite().hint() == CompositeTypeHint::VectorType)
    {
        auto baseType = dataType.composite().memberType(0).baseType();
        if (baseType == BaseType::Float)
        {
            if (dataType.composite().scalarComponentCount() == 2)
                return ImageFormat::RG32F;
            else if (dataType.composite().scalarComponentCount() == 3)
                return ImageFormat::RGB32F;
            else if (dataType.composite().scalarComponentCount() == 4)
                return ImageFormat::RGBA32F;
        }
        else if (baseType == BaseType::Int)
        {
            if (dataType.composite().scalarComponentCount() == 2)
                return ImageFormat::RG32_INT;
            else if (dataType.composite().scalarComponentCount() == 3)
                return ImageFormat::RGB32_INT;
            else if (dataType.composite().scalarComponentCount() == 4)
                return ImageFormat::RGBA32_INT;
        }
        else if (baseType == BaseType::Uint)
        {
            if (dataType.composite().scalarComponentCount() == 2)
                return ImageFormat::RG32_UINT;
            else if (dataType.composite().scalarComponentCount() == 3)
                return ImageFormat::RGB32_UINT;
            else if (dataType.composite().scalarComponentCount() == 4)
                return ImageFormat::RGBA32_UINT;
        }
    }
    else if (dataType.isComposite() && dataType.composite().hint() == CompositeTypeHint::MatrixType)
    {
        if (dataType.composite().scalarComponentCount() == 4)
            return ImageFormat::MAT22F;
        if (dataType.composite().scalarComponentCount() == 6)
            return ImageFormat::MAT32F;
        if (dataType.composite().scalarComponentCount() == 8)
            return ImageFormat::MAT42F;
        if (dataType.composite().scalarComponentCount() == 9)
            return ImageFormat::MAT33F;
        else if (dataType.composite().scalarComponentCount() == 12)
            return ImageFormat::MAT43F;
        else if (dataType.composite().scalarComponentCount() == 16)
            return ImageFormat::MAT44F;
    }

    return ImageFormat::UNKNOWN;
}

bool CompositeType::packLayoutVertex(const CompositeType::Member& prop, uint32_t& inOutPackingOffset, MemberLayoutInfo& outLayout, parser::IErrorReporter& err) const
{
    // arrays are not allowed as members
    if (prop.type.isArray())
    {
        err.reportError(prop.location, TempString("Vertex data layout '{}' can't contain member '{}' with array type '{}'", m_name, prop.name, prop.type));
        return false;
    }

    // determine the packed data format for the member
    auto dataFormat = FindPackedTypeForDataType(prop.type);
    if (dataFormat == ImageFormat::UNKNOWN)
    {
        err.reportError(prop.location, TempString("Vertex data layout '{}' member '{}' with type '{}' that is not packable for vertex shader input", m_name, prop.name, prop.type));
        return false;
    }

    // use the custom packing format if it was specified
    if (const auto customFormatTxt = prop.attributes.value("format"_id))
    {
        auto orgFormat = dataFormat;
        if (!GetImageFormatByShaderName(customFormatTxt, dataFormat))
        {
            err.reportError(prop.location, TempString("Vertex data layout '{}' member '{}' with type '{}' wants to use custom format '{}' that is not recognized or supported", m_name, prop.name, prop.type, customFormatTxt));
            return false;
        }
    }

    // get the size and alignment
    uint32_t sizeToPack = GetImageFormatInfo(dataFormat).bitsPerPixel / 8;
    uint32_t alignment = 4; // always use alignment of 4 in packing vertex data

    // assign size and alignment
    outLayout.linearAlignment = alignment;
    outLayout.linearSize = sizeToPack;
    outLayout.dataFormat = dataFormat;

    // assign offset
    if (!prop.attributes.has("offset"_id))
    {
        outLayout.linearOffset = Align(inOutPackingOffset, alignment);
    }
    else
    {
        int offset = prop.attributes.valueAsIntOrDefault("offset"_id, -1);

        // offset must be positive
        if (offset < 0 || offset >= 65536)
        {
            err.reportError(prop.location, TempString("Vertex data layout '{}' contains member '{}' with user invalid offset {}", m_name, prop.name, offset));
            return false;

        }

        // the offset must be a multiply of the required alignment
        if (offset % alignment != 0)
        {
            err.reportError(prop.location, TempString("Vertex data layout '{}' contains member '{}' with user specified offset {} that is not a multiply of required alignment {}", m_name, prop.name, offset, alignment));
            return false;
        }

        // make sure we are not moving back in offsets
        if (offset < (int)inOutPackingOffset)
        {
            err.reportError(prop.location, TempString("Vertex data layout '{}' contains member '{}' with user specified offset {} that overlaps current data at offset {}", m_name, prop.name, offset, inOutPackingOffset));
            return false;
        }

        // use the offset
        outLayout.linearOffset = (uint16_t)offset;
        inOutPackingOffset = outLayout.linearOffset;
    }

    // advance
    inOutPackingOffset += sizeToPack;
    return true;
}

bool CompositeType::packLayoutStd140(const CompositeType::Member& prop, uint32_t& inOutPackingOffset, MemberLayoutInfo& outLayout, parser::IErrorReporter& err) const
{
    // if we are packing an array use the array inner type
    // NOTE: we only support native arrays here
    uint32_t minimalAlignment = 4;
    auto type = prop.type;
    if (type.isArray())
    {
        // get the array size for later and use the inner type for packing
        outLayout.linearArrayCount = range_cast<uint16_t>(prop.type.arrayCounts().elementCount()); // total element count
        minimalAlignment = 16;

        // use the inner type for packing
        type = type.removeArrayCounts();
    }
    else
    {
        outLayout.linearArrayCount = 0;
    }

    // custom packing format is not allowed in std140
    if (prop.attributes.has("layout"_id))
    {
        err.reportWarning(prop.location, TempString("Ignored packing layout for structure '{}' member '{}' as it is not supported here", m_name, prop.name));
    }
    if (prop.attributes.has("format"_id))
    {
        err.reportWarning(prop.location, TempString("Ignored packing format for structure '{}' member '{}' as it is not supported here", m_name, prop.name));
    }

    // pack composite layout
    uint32_t sizeToPack = 0;
    uint32_t alignment = 4;
    auto dataFormat = ImageFormat::UNKNOWN;
    if (type.isComposite() && type.composite().hint() == CompositeTypeHint::User)
    {
        // we need the type to be mapped
        if (!type.composite().m_layoutComputed)
            return false;

        // get required struct alignment and packing size from the mapped struct
        alignment = type.composite().m_linearAlignment;
        sizeToPack = type.composite().m_linearSize;
    }
    else
    {
        // determine the packed data format for the member
        dataFormat = FindPackedTypeForDataType(prop.type);
        if (dataFormat == ImageFormat::UNKNOWN)
        {
            err.reportError(prop.location, TempString("Structure layout '{}' member '{}' with type '{}' is not packable for std140", m_name, prop.name, type));
            return false;
        }

        // get the size and alignment
        sizeToPack = GetImageFormatInfo(dataFormat).bitsPerPixel / 8;
        alignment = std::min<uint32_t>(16, NextPow2(sizeToPack));
    }

    // array
    if (prop.type.isArray())
    {
        auto numElements = outLayout.linearArrayCount;
        sizeToPack = Align<uint32_t>(sizeToPack, minimalAlignment) * numElements;
    }

    // make sure minimal alignment is respected
    alignment = std::max<uint32_t>(alignment, minimalAlignment);

    // assign size and alignment
    outLayout.linearAlignment = range_cast<uint16_t>(alignment);
    outLayout.linearSize = range_cast<uint16_t>(sizeToPack);
    outLayout.dataFormat = dataFormat;

    // assign offset
    if (!prop.attributes.has("offset"_id))
    {
        outLayout.linearOffset = range_cast<uint16_t>(Align(inOutPackingOffset, alignment));
    }
    else
    {
        int offset = prop.attributes.valueAsIntOrDefault("offset"_id, -1);

        // the offset must be valid
        if (offset < 0 || offset >= 65536)
        {
            err.reportError(prop.location, TempString("Struct layout '{}' contains member '{}' with user specified offset {} that is not a multiply of required alignment {}",
                m_name, prop.name, offset, alignment));
            return false;
        }

        // the offset must be a multiply of the required alignment
        if (offset % alignment != 0)
        {
            err.reportError(prop.location, TempString("Struct layout '{}' contains member '{}' with user specified offset {} that is not a multiply of required alignment {}",
                                                            m_name, prop.name, offset, alignment));
            return false;
        }

        // make sure we are not moving back in offsets
        if (offset < (int)inOutPackingOffset)
        {
            err.reportError(prop.location, TempString("Struct layout '{}' contains member '{}' with user specified offset {} that overlaps current data at offset {}",
                                                            m_name, prop.name, offset, inOutPackingOffset));
            return false;
        }

        // use the offset
        outLayout.linearOffset = (uint16_t)offset;
        inOutPackingOffset = outLayout.linearOffset;
    }

    // advance
    inOutPackingOffset = outLayout.linearOffset + sizeToPack;

    // set array stride
    if (outLayout.linearArrayCount > 0)
        outLayout.linearArrayStride = Align<uint32_t>(outLayout.linearSize, minimalAlignment);

    // packed
    return true;
}

bool CompositeType::packLayoutStd430(const CompositeType::Member& prop, uint32_t& inOutPackingOffset, MemberLayoutInfo& outLayout, parser::IErrorReporter& err) const
{
    // TODO!
    return packLayoutStd140(prop, inOutPackingOffset, outLayout, err);
}

bool CompositeType::computeMemoryLayout(bool& outNeedsMorePasses, bool& outUpdated, parser::IErrorReporter& err)
{
    if (m_layoutComputed)
        return true;

    m_linearSize = 0;
    m_linearAlignment = 0;

    bool allPacked = true;
    uint32_t currentOffset = 0;
    for (auto& member : m_members)
    {
        // resources are not allowed in structures
        if (member.type.isResource())
        {
            err.reportError(member.location, TempString("Structure '{}' member '{}' is using a resource type that is only allowed in descriptor layouts not in general structures",
                                                                name(), member.name));
            return false;
        }

        // pack the data
        if (packingRules() == CompositePackingRules::Vertex)
        {
            if (!packLayoutVertex(member, currentOffset, member.layout, err))
            {
                allPacked = false;
                outNeedsMorePasses = true;
            }
        }
        else if (packingRules() == CompositePackingRules::Std140)
        {
            if (!packLayoutStd140(member, currentOffset, member.layout, err))
            {
                allPacked = false;
                outNeedsMorePasses = true;
            }
        }
        else if (packingRules() == CompositePackingRules::Std430)
        {
            if (!packLayoutStd430(member, currentOffset, member.layout, err))
            {
                allPacked = false;
                outNeedsMorePasses = true;
            }
        }
        else
        {
            err.reportError(member.location, TempString("Structure '{}' is using unknown packing mode", name()));
            return false;
        }
    }

    // update size
    if (allPacked)
    {
        if (hint() == CompositeTypeHint::VectorType || hint() == CompositeTypeHint::MatrixType)
        {
            m_linearAlignment = 4;
            m_linearSize = currentOffset; // may require padding
        }
        else
        {
            m_linearAlignment = 4;
            for (auto& elem : m_members)
                m_linearAlignment = std::max<uint32_t>(m_linearAlignment, elem.layout.linearAlignment);
            m_linearSize = currentOffset; // may require padding
        }

        //TRACE_INFO("Size of packed structure '{}' computed as {} with alignment {}", name(), linearSize, linearAlignment);
        m_layoutComputed = true;
        outUpdated = true;
    }

    return true;
}

void CompositeType::print(IFormatStream& f) const
{
    f.appendf("    composite {} ({} members)\n", m_name, m_members.size());

    for (uint32_t i=0; i<m_members.size(); ++i)
    {
        auto& m = m_members[i];

        f.appendf("      [{}]: ", i);
        f << m.type;
        f.appendf(" {}", m.name);

        /*if (m.customOffset != -1)
            f.appendf(", offset({})", m.customOffset);

        if (!m.customFormat.empty())
            f.appendf(", format({})", m.customFormat);*/

        f.appendf(", linearOffset={}, linearSize={}, linearArrayCount={}, linearArrayStride={}", m.layout.linearOffset, m.layout.linearSize, m.layout.linearArrayCount, m.layout.linearArrayStride);
        f.append("\n");
    }
}

//---

ResourceTableEntry::ResourceTableEntry()
{}

//---

ResourceTable::ResourceTable(StringID name, const AttributeList& attributes)
    : m_name(name)
    , m_attributes(attributes)
{}

int ResourceTable::memberIndex(const StringID name) const
{
    for (uint32_t i=0; i<m_members.size(); ++i)
        if (m_members[i].m_name == name)
            return i;

    return INDEX_NONE;
}

StringID ResourceTable::memberName(uint32_t index) const
{
    if (index < m_members.size())
        return m_members[index].m_name;

    return StringID();
}

void ResourceTable::addMember(const parser::Location& loc, const StringID name, const DataType& type, const AttributeList& attributes, char localSampler, const StaticSampler* staticSampler)
{
    ASSERT(name);
    ASSERT(memberIndex(name) == -1);

    auto& member = m_members.emplaceBack();
    member.m_location = loc;
    member.m_name = name;
    member.m_type = type;
    member.m_attributes = attributes;            
	member.m_localSampler = localSampler;
	member.m_staticSampler = staticSampler;
}

void ResourceTable::print(IFormatStream& f) const
{
    f.appendf("    descriptor {} ({} members)", m_name, m_members.size());

    f.append("\n");

    for (uint32_t i=0; i<m_members.size(); ++i)
    {
        auto& m = m_members[i];

        f.appendf("      [{}]: ", i);
        f << m.m_type;

        if (m.m_name.empty())
            f.appendf(" UNNAMED", m.m_name);
        else
            f.appendf(" {}", m.m_name);

        /*if (m.m_resourceFormat != ImageFormat::UNKNOWN)
            f.appendf(", format({})", m.m_resourceFormat);

        if (m.m_resourceLayout != nullptr)
            f.appendf(", layout({})", m.m_resourceLayout->name());*/
        f.append("\n");
    }
}       

//---

StaticSampler::StaticSampler(StringID name, const SamplerState& state)
	: m_name(name)
	, m_state(state)
{}

//---

StaticRenderStates::StaticRenderStates(StringID name, const GraphicsRenderStatesSetup& state)
	: m_name(name)
	, m_state(state)
{}

//---

TypeLibrary::TypeLibrary(LinearAllocator& allocator)
    : m_allocator(allocator)
{
    createDefaultTypes();
}

TypeLibrary::~TypeLibrary()
{
    for (auto ptr : m_resourceTables)
        ptr->~ResourceTable();

    m_resourceTableMap.clear();
    m_resourceTables.clear();

    for (auto ptr : m_staticSamplers)
        ptr->~StaticSampler();

	m_staticSamplerMap.clear();
	m_staticSamplers.clear();

	for (auto ptr : m_staticRenderStates)
		ptr->~StaticRenderStates();

	m_staticRenderStatesMap.clear();
	m_staticRenderStates.clear();

	for (auto ptr : m_compositeTypes)
		ptr->~CompositeType();

	m_compositeTypeMap.clear();
	m_compositeTypes.clear();
}

DataType TypeLibrary::compositeType(const StringID name) const
{
    const CompositeType* ret = nullptr;
    m_compositeTypeMap.find(name, ret);
    if (!ret)
        return DataType();

    return DataType(ret);
}

DataType TypeLibrary::booleanType(const uint32_t vectorSize /*= 1*/) const
{
    ASSERT(vectorSize > 0);
    ASSERT(vectorSize <= MAX_COMPONENTS);
    return m_boolTypes[vectorSize - 1];
}

DataType TypeLibrary::integerType(const uint32_t vectorSize /*= 1*/) const
{
    ASSERT(vectorSize > 0);
    ASSERT(vectorSize <= MAX_COMPONENTS);
    return m_intTypes[vectorSize - 1];
}

DataType TypeLibrary::unsignedType(const uint32_t vectorSize /*= 1*/) const
{
    ASSERT(vectorSize > 0);
    ASSERT(vectorSize <= MAX_COMPONENTS);
    return m_uintTypes[vectorSize - 1];
}

DataType TypeLibrary::floatType(const uint32_t vectorSize /*= 1*/, uint32_t matrixRows /*= 1*/) const
{
    ASSERT(vectorSize > 0);
    ASSERT(vectorSize <= MAX_COMPONENTS);
    ASSERT(matrixRows > 0);
    ASSERT(matrixRows <= MAX_COMPONENTS);
    return m_floatTypes[matrixRows-1][vectorSize - 1];
}

DataType TypeLibrary::simpleCompositeType(BaseType baseType, uint32_t vectorSize /*= 1*/, uint32_t matrixRows /*= 1*/) const
{
    if (vectorSize >= 1 && vectorSize <= 4)
    {
        if (matrixRows == 1)
        {
            if (baseType == BaseType::Boolean)
                return booleanType(vectorSize);
            else if (baseType == BaseType::Int)
                return integerType(vectorSize);
            else if (baseType == BaseType::Uint)
                return unsignedType(vectorSize);
            else if (baseType == BaseType::Float)
                return floatType(vectorSize);
        }
        else if (matrixRows > 1 && matrixRows <= 4)
        {
            if (baseType == BaseType::Float)
                return floatType(vectorSize, matrixRows);
        }
    }

    return DataType();
}

DataType TypeLibrary::packedFormatElementType(ImageFormat format) const
{
    // invalid format
    if (ImageFormat::UNKNOWN == format)
        return DataType();

    // create a simple composite type
    auto& info = GetImageFormatInfo(format);
    switch (info.formatClass)
    {
        case ImageFormatClass::FLOAT: return simpleCompositeType(BaseType::Float, info.numComponents);
        case ImageFormatClass::INT: return simpleCompositeType(BaseType::Int, info.numComponents);
        case ImageFormatClass::UINT: return simpleCompositeType(BaseType::Uint, info.numComponents);
        case ImageFormatClass::DEPTH: return simpleCompositeType(BaseType::Float, info.numComponents);
        case ImageFormatClass::SRGB: return simpleCompositeType(BaseType::Float, info.numComponents);
        case ImageFormatClass::UNORM: return simpleCompositeType(BaseType::Float, info.numComponents);
        case ImageFormatClass::SNORM: return simpleCompositeType(BaseType::Float, info.numComponents);
        default: return DataType();
    }
}

//--

DataType TypeLibrary::registerCompositeType(CompositeType* compositeType)
{
    ASSERT(compositeType != nullptr);
    ASSERT(!compositeType->name().empty());
    ASSERT(!m_compositeTypeMap.contains(compositeType->name()));

    m_compositeTypeMap.set(compositeType->name(), compositeType);
    m_compositeTypes.pushBack(compositeType);

    return DataType(compositeType);
}

const CompositeType* TypeLibrary::findCompositeType(StringID name) const
{
    const CompositeType* ret = nullptr;
    m_compositeTypeMap.find(name, ret);
    return ret;
}

//---

void TypeLibrary::registerResourceTable(ResourceTable* table)
{
    ASSERT(table != nullptr);
    ASSERT(!table->name().empty());
    ASSERT(!m_resourceTableMap.contains(table->name()));

    m_resourceTableMap.set(table->name(), table);
    m_resourceTables.pushBack(table);
}

const ResourceTable* TypeLibrary::findResourceTable(StringID name) const
{
    const ResourceTable* ret = nullptr;
    m_resourceTableMap.find(name, ret);
    return ret;
}

//---

void TypeLibrary::registerStaticSampler(StaticSampler* sampler)
{
	ASSERT(sampler != nullptr);
	ASSERT(!sampler->name().empty());
	ASSERT(!m_staticSamplerMap.contains(sampler->name()));

	m_staticSamplerMap.set(sampler->name(), sampler);
	m_staticSamplers.pushBack(sampler);
}

const StaticSampler* TypeLibrary::findStaticSampler(StringID name) const
{
	const StaticSampler* ret = nullptr;
	m_staticSamplerMap.find(name, ret);
	return ret;
}

///--

void TypeLibrary::registerStaticRenderStates(StaticRenderStates* states)
{
	ASSERT(states != nullptr);
	ASSERT(!states->name().empty());
	ASSERT(!m_staticRenderStatesMap.contains(states->name()));

	m_staticRenderStatesMap.set(states->name(), states);
	m_staticRenderStates.pushBack(states);
}

const StaticRenderStates* TypeLibrary::findStaticRenderStates(StringID name) const
{
	const StaticRenderStates* ret = nullptr;
	m_staticRenderStatesMap.find(name, ret);
	return ret;
}

//---

namespace helper
{
    // member names
    const StringID memberNames[4] =
    {
        "x"_id,
        "y"_id,
        "z"_id,
        "w"_id,
    };

    // create a type with N repeated components (up to 4)
    static CompositeType* CreateVectorType(LinearAllocator& mem, const char* core, DataType memberType, uint32_t numComponents, CompositeTypeHint typeHint)
    {
        StringBuf name = TempString("{}{}", core, numComponents);

        if (typeHint == CompositeTypeHint::MatrixType)
        {
            const auto numVectorComponents = memberType.composite().members().size();
            if (numVectorComponents == numComponents)
                name = TempString("mat{}", numComponents);
        }

        auto t = mem.createNoCleanup<CompositeType>(StringID(name.c_str()), CompositePackingRules::Std140, typeHint);

        for (uint32_t i = 0; i < numComponents; ++i)
            t->addMember(parser::Location(), memberNames[i], memberType, AttributeList());

        return t;
    }

}

void TypeLibrary::createDefaultTypes()
{
    // scalars
    m_boolTypes[0] = DataType::BoolScalarType();
    m_intTypes[0] = DataType::IntScalarType();
    m_uintTypes[0] = DataType::UnsignedScalarType();
    m_floatTypes[0][0] = DataType::FloatScalarType();

    // simple vectors
    for (uint32_t i = 1; i < MAX_COMPONENTS; ++i)
    {
        m_boolTypes[i] = registerCompositeType(helper::CreateVectorType(m_allocator, "bvec", DataType::BoolScalarType(), i + 1, CompositeTypeHint::VectorType));
        m_intTypes[i] = registerCompositeType(helper::CreateVectorType(m_allocator, "ivec", DataType::IntScalarType(), i + 1, CompositeTypeHint::VectorType));
        m_uintTypes[i] = registerCompositeType(helper::CreateVectorType(m_allocator, "uvec", DataType::UnsignedScalarType(), i + 1, CompositeTypeHint::VectorType));
        m_floatTypes[0][i] = registerCompositeType(helper::CreateVectorType(m_allocator, "vec", DataType::FloatScalarType(), i + 1, CompositeTypeHint::VectorType));
    }

    // float matrices
    for (uint32_t i = 1; i < MAX_COMPONENTS; ++i)
    {
        m_floatTypes[i][1] = registerCompositeType(helper::CreateVectorType(m_allocator, "mat2x", m_floatTypes[0][1], i + 1, CompositeTypeHint::MatrixType));
        m_floatTypes[i][2] = registerCompositeType(helper::CreateVectorType(m_allocator, "mat3x", m_floatTypes[0][2], i + 1, CompositeTypeHint::MatrixType));
        m_floatTypes[i][3] = registerCompositeType(helper::CreateVectorType(m_allocator, "mat4x", m_floatTypes[0][3], i + 1, CompositeTypeHint::MatrixType));
    }
}

bool TypeLibrary::calculateCompositeLayouts(parser::IErrorReporter& err)
{
    while (1)
    {
        bool somethingUpdated = false;
        bool needsPass = false;
        for (auto layout : m_compositeTypes)
        {
            // compute the layout, if this errors we have compilation problem
            if (!const_cast<CompositeType*>(layout)->computeMemoryLayout(needsPass, somethingUpdated, err))
                return false;
        }

        // if we couldn't resolve the types we may need another pass
        if (needsPass)
            continue;

        break;
    }

    return true;
}

static uint64_t CalcResourceTypeHash(StringID typeName, const AttributeList& attributes)
{
    CRC64 crc;
    crc << typeName;
    attributes.calcTypeHash(crc);
    return crc.crc();
}

static bool MutateTextureName(StringID& typeName, AttributeList& attributes)
{
    bool valid = true;

    if (typeName == "Texture2D"_id)
    {
        typeName = "Texture"_id;
        attributes.add("dim2D"_id);
        attributes.add("multisampled"_id);
    }
    else if (typeName == "Texture1D"_id)
    {
        typeName = "Texture"_id;
        attributes.add("dim1D"_id);
    }
    else if (typeName == "Texture3D"_id)
    {
        typeName = "Texture"_id;
        attributes.add("dim3D"_id);
    }
    else if (typeName == "TextureCube"_id)
    {
        typeName = "Texture"_id;
        attributes.add("dimCube"_id);
    }
    else if (typeName == "Texture1DArray"_id)
    {
        typeName = "Texture"_id;
        attributes.add("dim1D"_id);
        attributes.add("array"_id);
    }
    else if (typeName == "Texture2DArray"_id)
    {
        typeName = "Texture"_id;
        attributes.add("dim2D"_id);
        attributes.add("array"_id);
        attributes.add("multisampled"_id);
    }
    else if (typeName == "TextureCubeArray"_id)
    {
        typeName = "Texture"_id;
        attributes.add("dimCube"_id);
        attributes.add("array"_id);
    }
    else
    {
        valid = false;
    }

    return valid;
}

DataType TypeLibrary::resourceType(const CompositeType* constantBufferLayout, const AttributeList& attributes)
{
    auto* ret = m_allocator.createNoCleanup<ResourceType>();

    ret->attributes = attributes;
    ret->type = DeviceObjectViewType::ConstantBuffer;
    ret->resolvedLayout = constantBufferLayout;
            
    m_resourceTypes.pushBack(ret);
    return DataType(ret);
}
		
DataType TypeLibrary::resourceType(StringID typeName, const AttributeList& attributes, StringBuf& outError)
{
    const auto typeHash = CalcResourceTypeHash(typeName, attributes);
            
    const ResourceType* existingType = nullptr;
    if (m_resourceTypesMap.find(typeHash, existingType))
        return DataType(existingType);

    auto* ret = m_allocator.createNoCleanup<ResourceType>();
    m_resourceTypes.pushBack(ret);
    ret->attributes = attributes;

    if (typeName != "Texture"_id && typeName.view().beginsWith("Texture"))
    {
		if (!MutateTextureName(typeName, ret->attributes))
		{
			outError = TempString("Unable to infer attributes from unrecognized resource type '{}'", typeName);
			return DataType();
		}
    }

    if (typeName == "Texture"_id)
    {
        ret->depth = ret->attributes.has("depth"_id);
		ret->multisampled = ret->attributes.has("multisample"_id);
        ret->resolvedFormat = ImageFormat::RGBA32F;

        if (ret->attributes.has("unsigned"_id))
            ret->resolvedFormat = ImageFormat::RGBA32_UINT;
        else if (ret->attributes.has("signed"_id))
            ret->resolvedFormat = ImageFormat::RGBA32_INT;
                
        bool isArray = ret->attributes.has("array"_id);
        if (ret->attributes.has("dim1D"_id))
            ret->resolvedViewType = isArray ? ImageViewType::View1DArray : ImageViewType::View1D;
        else if (ret->attributes.has("dim2D"_id))
            ret->resolvedViewType = isArray ? ImageViewType::View2DArray : ImageViewType::View2D;
        else if (ret->attributes.has("dim3D"_id))
            ret->resolvedViewType = ImageViewType::View3D;
        else if (ret->attributes.has("dimCube"_id))
            ret->resolvedViewType = isArray ? ImageViewType::ViewCubeArray : ImageViewType::ViewCube;
        else
            return DataType();

		if (ret->resolvedViewType == ImageViewType::View3D && isArray)
		{
			outError = TempString("3D texture can't be an array");
			return DataType();
		}

		if (ret->attributes.has("uav"_id))
		{
			if (ret->attributes.has("sampler"_id))
			{
				outError = TempString("UAV bound textures can't have 'sampler' attribute");
				return DataType();
			}

			if (auto formatString = ret->attributes.value("format"_id))
			{
				if (!GetImageFormatByShaderName(formatString, ret->resolvedFormat))
				{
					outError = TempString("Unrecognized image format: '{}'", formatString);
					return DataType();
				}

				if (!IsFormatValidForView(ret->resolvedFormat))
				{
					outError = TempString("Format '{}' is not valid for UAV", formatString);
					return DataType();
				}

				// TODO: validate if format is a legal format for UAV write
			}
			else
			{
				outError = TempString("UAV view of an image must have the format specified, use 'format=rgba8', format='r32f' etc.");
				return DataType();
			}

			ret->type = DeviceObjectViewType::ImageWritable;
		}
		else if (ret->attributes.has("nosampler"_id))
		{
			if (ret->attributes.has("sampler"_id))
			{
				outError = TempString("'nosampler' and 'sampler' can't be specified at the same time");
				return DataType();
			}

			ret->type = DeviceObjectViewType::Image;

			if (auto formatString = ret->attributes.value("format"_id))
			{
				if (!GetImageFormatByShaderName(formatString, ret->resolvedFormat))
				{
					outError = TempString("Unrecognized image format: '{}'", formatString);
					return DataType();
				}

				if (!IsFormatValidForView(ret->resolvedFormat))
				{
					outError = TempString("Format '{}' is not valid for an shader bound image", formatString);
					return DataType();
				}
			}
			else
			{
				outError = TempString("No-sample views of texture must have the format specified, use 'format=rgba8', format='r32f' etc.");
				return DataType();
			}
		}
		else if (ret->attributes.has("sampler"_id))
		{
			if (auto formatString = ret->attributes.value("format"_id))
			{
				outError = TempString("Format specialization is not used for sampled images but you can use 'signed' and 'unsigned' attributes");
				return DataType();
			}

			if (ret->attributes.has("signed"_id))
				ret->sampledImageFlavor = BaseType::Int;
			else if (ret->attributes.has("unsigned"_id))
				ret->sampledImageFlavor = BaseType::Uint;
			else
				ret->sampledImageFlavor = BaseType::Float;

			ret->type = DeviceObjectViewType::SampledImage;					
		}
		else
		{
			outError = TempString("Texture resource resource 'sampler', 'nosampler' or 'uav' specialization");
			return DataType();
		}
    }
    else if (typeName == "Buffer"_id)
    {
		if (auto layoutString = ret->attributes.value("layout"_id))
		{
			const auto* layout = findCompositeType(layoutString);
			if (nullptr == layout)
				return DataType();

			ret->resolvedLayout = layout;

			if (ret->attributes.has("uav"_id))
				ret->type = DeviceObjectViewType::BufferStructuredWritable;
			else
				ret->type = DeviceObjectViewType::BufferStructured;
		}
		else
		{
			if (auto formatString = ret->attributes.value("format"_id))
			{
				if (!GetImageFormatByShaderName(formatString, ret->resolvedFormat))
					return DataType();
			}
			else
			{
				ret->resolvedFormat = ImageFormat::R32F;
			}

			if (ret->attributes.has("uav"_id))
				ret->type = DeviceObjectViewType::BufferWritable;
			else
				ret->type = DeviceObjectViewType::Buffer;
		}
    }
    else if (typeName == "ConstantBuffer"_id)
    {
		ret->type = DeviceObjectViewType::ConstantBuffer;
    }
	else if (typeName == "Sampler"_id)
	{
		ret->type = DeviceObjectViewType::Sampler;
	}
	else
	{
		outError = "Unknown resource type";
		return DataType();
	}

    m_resourceTypes.pushBack(ret);
    m_resourceTypesMap[typeHash] = ret;
    return DataType(ret);
}

//--

END_BOOMER_NAMESPACE_EX(gpu::compiler)
