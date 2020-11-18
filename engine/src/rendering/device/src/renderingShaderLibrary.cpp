/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shaders #]
*/

#include "build.h"
#include "renderingShaderLibrary.h"
#include "renderingDeviceApi.h"

#include "base/containers/include/stringBuilder.h"
#include "base/resource/include/resourceTags.h"
#include "renderingDeviceService.h"

namespace rendering
{

    //---

    uint32_t DataLayoutElement::CalcHash(const DataLayoutElement& key)
    {
        base::CRC32 crc;
        crc << key.name;
        crc << key.offset;
        crc << key.size;
        crc << key.alignment;
        crc << key.arrayCount;
        crc << key.arrayStride;
        crc << key.structureIndex;
        crc << (uint8_t)key.format;
        return crc.crc();
    }

    bool DataLayoutElement::operator==(const DataLayoutElement& other) const
    {
        return (name == other.name) && (offset == other.offset) && (size == other.size)
            && (alignment == other.alignment) && (arrayStride == other.arrayStride) && (arrayCount == other.arrayCount) && (structureIndex == other.structureIndex) && (format == other.format);
    }

    bool DataLayoutElement::operator!=(const DataLayoutElement& other) const
    {
        return !operator==(other);
    }

    //---

    uint32_t DataLayoutStructure::CalcHash(const DataLayoutStructure& key)
    {
        base::CRC32 crc;
        crc << key.name;
        crc << key.alignment;
        crc << key.size;
        crc << key.firstElementIndex;
        crc << key.numElements;
        return crc.crc();
    }

    bool DataLayoutStructure::operator==(const DataLayoutStructure& other) const
    {
        return (name == other.name) && (alignment == other.alignment) && (size == other.size) && (firstElementIndex == other.firstElementIndex) && (numElements == other.numElements);
    }

    bool DataLayoutStructure::operator!=(const DataLayoutStructure& other) const
    {
        return !operator==(other);
    }

    //---

    uint32_t VertexInputLayout::CalcHash(const VertexInputLayout& key)
    {
        base::CRC32 crc;
        crc << key.name;
        crc << key.structureIndex;
        crc << key.customStride;
        crc << key.instanced;
        return crc.crc();
    }

    bool VertexInputLayout::operator==(const VertexInputLayout& other) const
    {
        return (name == other.name) && (structureIndex == other.structureIndex) && (customStride == other.customStride) && (instanced == other.instanced);
    }

    bool VertexInputLayout::operator!=(const VertexInputLayout& other) const
    {
        return !operator==(other);
    }
    //---

    uint32_t VertexInputState::CalcHash(const VertexInputState& key)
    {
        base::CRC32 crc;
        crc << key.firstStreamLayout;
        crc << key.numStreamLayouts;
        crc << key.structureKey;
        return crc.crc();
    }

    bool VertexInputState::operator==(const VertexInputState& other) const
    {
        return (structureKey == other.structureKey) && (firstStreamLayout == other.firstStreamLayout) && (numStreamLayouts == other.numStreamLayouts);
    }

    bool VertexInputState::operator!=(const VertexInputState& other) const
    {
        return !operator==(other);
    }

    //--

    uint32_t ParameterResourceLayoutElement::CalcHash(const ParameterResourceLayoutElement& key)
    {
        base::CRC32 crc;
        crc << key.name;
        crc << (uint8_t)key.type;
        crc << key.maxArrayCount;
        crc << key.layout;
        crc << (uint8_t)key.format;
        return crc.crc();
    }

    bool ParameterResourceLayoutElement::operator==(const ParameterResourceLayoutElement& other) const
    {
        return (name == other.name) && (type == other.type) && (maxArrayCount == other.maxArrayCount) && (layout == other.layout) && (format == other.format);
    }

    bool ParameterResourceLayoutElement::operator!=(const ParameterResourceLayoutElement& other) const
    {
        return !operator==(other);
    }

    //--

    uint32_t ParameterResourceLayoutTable::CalcHash(const ParameterResourceLayoutTable& key)
    {
        base::CRC32 crc;
        crc << key.name;
        crc << key.firstElementIndex;
        crc << key.numElements;
        crc << key.structureKey;
        return crc.crc();
    }

    bool ParameterResourceLayoutTable::operator==(const ParameterResourceLayoutTable& other) const
    {
        return (structureKey == other.structureKey) && (name == other.name) && (firstElementIndex == other.firstElementIndex) && (numElements == other.numElements);
    }

    bool ParameterResourceLayoutTable::operator!=(const ParameterResourceLayoutTable& other) const
    {
        return !operator==(other);
    }

    //--


    //--

    uint32_t ParameterBindingState::CalcHash(const ParameterBindingState& key)
    {
        base::CRC32 crc;
        crc << key.firstParameterLayoutIndex;
        crc << key.numParameterLayoutIndices;
        crc << key.firstStaticSampler;
        crc << key.numStaticSamplers;
        crc << key.firstDirectConstant;
        crc << key.numDirectConstants;
        crc << key.structureKey;
        return crc.crc();
    }

    bool ParameterBindingState::operator==(const ParameterBindingState& other) const
    {
        return (structureKey == other.structureKey) && (firstParameterLayoutIndex == other.firstParameterLayoutIndex) && (numParameterLayoutIndices == other.numParameterLayoutIndices) &&
            (firstStaticSampler == other.firstStaticSampler) && (numStaticSamplers == other.numStaticSamplers) &&
            (firstDirectConstant == other.firstDirectConstant) && (numDirectConstants == other.numDirectConstants);
    }

    bool ParameterBindingState::operator!=(const ParameterBindingState& other) const
    {
        return !operator==(other);
    }

    //--

    uint32_t ShaderBlob::CalcHash(const ShaderBlob& key)
    {
        base::CRC32 crc;
        crc << (uint8_t)key.type;
        crc << key.offset;
        crc << key.packedSize;
        crc << key.unpackedSize;
        crc << key.dataHash;
        return crc.crc();
    }

    bool ShaderBlob::operator==(const ShaderBlob& other) const
    {
        return (type == other.type) && (offset == other.offset) && (packedSize == other.dataHash) && (unpackedSize == other.unpackedSize) && (dataHash == other.dataHash);
    }

    bool ShaderBlob::operator!=(const ShaderBlob& other) const
    {
        return !operator==(other);
    }

    //--

    uint32_t ShaderBundle::CalcHash(const ShaderBundle& key)
    {
        base::CRC32 crc;
        crc << key.vertexBindingState;
        crc << key.parameterBindingState;
        crc << key.firstShaderIndex;
        crc << key.numShaders;
        return crc.crc();
    }

    bool ShaderBundle::operator==(const ShaderBundle& other) const
    {
        return (parameterBindingState == other.parameterBindingState)
            && (vertexBindingState == other.vertexBindingState)
            && (firstShaderIndex == other.firstShaderIndex) && (numShaders == other.numShaders);
    }

    //--

    RTTI_BEGIN_TYPE_CLASS(ShaderLibraryData);
        RTTI_PROPERTY(m_structureData);
        RTTI_PROPERTY(m_shaderData);
    RTTI_END_TYPE();

    ShaderLibraryData::ShaderLibraryData()
    {}

    ShaderLibraryData::ShaderLibraryData(base::Buffer structureData, base::Buffer shadersData)
        : m_structureData(structureData)
        , m_shaderData(shadersData)
    {
        buildMaps();
    }

    void ShaderLibraryData::onPostLoad()
    {
        TBaseClass::onPostLoad();
        buildMaps();
    }

    uint64_t ShaderLibraryData::dataSize() const
    {
        uint64_t ret = 0;
        ret += m_shaderData.size();
        ret += m_structureData.size();
        return ret;
    }
    
    void ShaderLibraryData::buildMaps()
    {
        // build name table
        {
            const auto numNames = header().chunks[Chunk_Names].count;

            m_names.reset();
            m_names.reserve(numNames);

            const auto* stringTable = (const char*)chunkData(Chunk_StringTable);
            const auto* stringIndex = (const PipelineStringIndex*)chunkData(Chunk_Names);
            for (uint32_t i = 0; i < numNames; ++i)
                m_names.pushBack(base::StringID(stringTable + *stringIndex++));
        }
    }

    //--

    RTTI_BEGIN_TYPE_CLASS(ShaderLibrary);
        RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4shader");
        RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("Compiled Shader");
        RTTI_PROPERTY(m_data);
    RTTI_END_TYPE();

    ShaderLibrary::ShaderLibrary()
        : m_state(ShaderState::Loading)
    {
    }

    ShaderLibrary::ShaderLibrary(const ShaderLibraryDataPtr& existingData)
        : m_data(existingData)
        , m_state(ShaderState::Loaded)
    {
        m_data->parent(this);
        createDeviceResources_NoLock();
    }

    ShaderLibrary::~ShaderLibrary()
    {
        destroyDeviceResources_NoLock();
    }

    void ShaderLibrary::onPostLoad()
    {
        TBaseClass::onPostLoad();
        createDeviceResources_NoLock();
    }

    bool ShaderLibrary::resolve(ObjectID& outObject, PipelineIndex& outIndex, ShaderLibraryDataPtr* outData /*= nullptr*/) const
    {
        auto lock = base::CreateLock(m_lock);
        if (m_object && m_object->id())
        {
            if (outData)
                *outData = m_object->data();

            outObject = m_object->id();
            outIndex = m_index;
            return true;
        }

        return false;
    }

    void ShaderLibrary::bind(const ShaderObjectPtr& obj, PipelineIndex index)
    {
        auto lock = base::CreateLock(m_lock);

        DEBUG_CHECK(m_state == ShaderState::Loading);

        m_index = index;

        m_object = obj;
        m_data = m_object ? m_object->data() : nullptr;
        m_state = m_object ? ShaderState::Loaded : ShaderState::Failed;
    }

    void ShaderLibrary::createDeviceResources_NoLock()
    {
        DEBUG_CHECK_RETURN(m_data);

        if (auto service = base::GetService<rendering::DeviceService>())
            if (auto device = service->device())
                m_object = device->createShaders(m_data);
    }

    void ShaderLibrary::destroyDeviceResources_NoLock()
    {
        m_object.reset();
    }

    //--

    ShaderObject::ShaderObject(ObjectID id, const ShaderLibraryDataPtr& data, IDeviceObjectHandler* impl)
        : IDeviceObject(id, impl)
        , m_data(data)
    {
        DEBUG_CHECK(m_data);
    }

    //--

} // rendering
