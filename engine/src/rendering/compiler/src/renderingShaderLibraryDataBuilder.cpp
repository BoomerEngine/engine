/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
*/

#include "build.h"
#include "renderingShaderTypeLibrary.h"
#include "renderingShaderLibraryDataBuilder.h"

#include "base/object/include/rttiArrayType.h"
#include "base/containers/include/inplaceArray.h"
#include "renderingShaderFunction.h"
#include "renderingShaderDataType.h"

namespace rendering
{
    namespace compiler
    {
        //---

        ShaderLibraryBuilder::ShaderLibraryBuilder()
            : m_shaderData(POOL_COMPILED_SHADER_DATA)
        {
            m_stringMap.reserve(256);
            m_stringTable.reserve(4096);
            m_stringTable.pushBack(0);
            m_stringMap[base::StringBuf::EMPTY()] = 0;

            m_nameTable.reserve(256);
            m_nameTable.pushBack(0);
            m_nameMap[base::StringBuf::EMPTY()] = 0;

            //m_dataStructureMap.reserve(256);
            //m_parameterLayoutMap.reserve(256);

            m_dataLayoutElements.reserve(256);
            m_dataLayoutElementMap.reserve(256);
            m_dataLayoutStructures.reserve(256);
            m_dataLayoutStructureMap.reserve(256);
            m_vertexInputLayouts.reserve(256);
            m_vertexInputLayoutMap.reserve(256);
            m_vertexInputStates.reserve(256);
            m_vertexInputStateMap.reserve(256);
            m_parameterResourceLayoutElements.reserve(256);
            m_parameterResourceLayoutElementMap.reserve(256);
            m_parameterResourceLayoutTables.reserve(256);
            m_parameterResourceLayoutTableMap.reserve(256);
            m_parameterBindingStates.reserve(256);
            m_parameterBindingStateMap.reserve(256);
            m_shaderBlobs.reserve(256);
            m_shaderBlobMap.reserve(256);
            m_shaderBundles.reserve(256);
            m_shaderBundleMap.reserve(256);
            m_indirectIndices.reserve(1024);
        }

        PipelineStringIndex ShaderLibraryBuilder::mapString(base::StringView txt)
        {
            if (!txt)
                return 0;

            PipelineStringIndex ret = 0;
            if (m_stringMap.find(txt, ret))
                return ret;

            auto stringIndex = m_stringTable.size();
            auto ptr = m_stringTable.allocateUninitialized(txt.length() + 1);
            memcpy(ptr, txt.data(), txt.length());
            ptr[txt.length()] = 0; // always zero terminate the string in storage

            m_stringMap[base::StringBuf(txt)] = stringIndex;
            return stringIndex;
        }

        PipelineIndex ShaderLibraryBuilder::mapName(base::StringView txt)
        {
            if (!txt)
                return 0;

            PipelineIndex ret = 0;
            if (m_nameMap.find(txt, ret))
                return ret;

            auto nameIndex = m_nameTable.size();
            m_nameTable.pushBack(mapString(txt));
            m_nameMap[base::StringBuf(txt)] = nameIndex;

            return nameIndex;
        }

        PipelineIndex ShaderLibraryBuilder::mapDataLayout(const CompositeType* structType)
        {
            if (!structType)
                return INVALID_PIPELINE_INDEX;

            PipelineIndex structureIndex = INVALID_PIPELINE_INDEX;
            if (m_compositeTypeMap.find(structType, structureIndex))
                return structureIndex;

            base::InplaceArray<PipelineIndex, 16> elements;
            for (auto& member : structType->members())
            {
                ASSERT(!member.type.isResource());

                // determine packing offset since the base of the structure
                DataLayoutElement element;
                element.format = member.layout.dataFormat;
                element.structureIndex = INVALID_PIPELINE_INDEX;
                element.offset = range_cast<uint16_t>(member.layout.linearOffset);
                element.size = range_cast<uint16_t>(member.layout.linearSize);
                element.alignment = range_cast<uint16_t>(member.layout.linearAlignment);
                element.arrayCount = range_cast<uint16_t>(member.layout.linearArrayCount);
                element.arrayStride = range_cast<uint16_t>(member.layout.linearArrayStride);
                element.name = mapName(member.name.view());

                // try to reuse index
                PipelineIndex elementIndex = INVALID_PIPELINE_INDEX;
                if (!m_dataLayoutElementMap.find(element, elementIndex))
                {
                    elementIndex = m_dataLayoutElements.size();
                    m_dataLayoutElements.pushBack(element);
                    m_dataLayoutElementMap[element] = elementIndex;
                }

                elements.pushBack(elementIndex);
            }

            // map the range of elements, can map to nothing if we have no elements
            const auto indirectIndex = mapIndirectIndices(elements.typedData(), elements.size());
            DEBUG_CHECK_EX(indirectIndex != INVALID_PIPELINE_INDEX, "Failed to map structure elements");
                
            // emit structure
            DataLayoutStructure structInfo;
            structInfo.alignment = range_cast<uint16_t>(structType->linearAlignment());
			structInfo.size = range_cast<uint16_t>(structType->linearSize());
            structInfo.name = mapString(structType->name().c_str());
            structInfo.firstElementIndex = indirectIndex;
            structInfo.numElements = range_cast<uint16_t>(elements.size());

            // funny thing is that we may already have it cached
            if (!m_dataLayoutStructureMap.find(structInfo, structureIndex))
            {
                structureIndex = m_dataLayoutStructures.size();
                m_dataLayoutStructures.pushBack(structInfo);
                m_dataLayoutStructureMap[structInfo] = structureIndex;
            }

            // remember this type as well
            DEBUG_CHECK(structureIndex != INVALID_PIPELINE_INDEX);
            m_compositeTypeMap[structType] = structureIndex;
            return structureIndex;
        }

        PipelineIndex ShaderLibraryBuilder::mapVertexInputState(const VertexInputState& state)
        {
            // funny thing is that we may already have it cached
            PipelineIndex stateIndex = INVALID_PIPELINE_INDEX;;
            if (!m_vertexInputStateMap.find(state, stateIndex))
            {
                stateIndex = m_vertexInputStates.size();
                m_vertexInputStates.pushBack(state);
                m_vertexInputStateMap[state] = stateIndex;
            }

            // remember this type as well
            DEBUG_CHECK(stateIndex != INVALID_PIPELINE_INDEX);
            return stateIndex;
        }

        PipelineIndex ShaderLibraryBuilder::mapVertexInputLayout(const VertexInputLayout& state)
        {
            // funny thing is that we may already have it cached
            PipelineIndex stateIndex = INVALID_PIPELINE_INDEX;;
            if (!m_vertexInputLayoutMap.find(state, stateIndex))
            {
                stateIndex = m_vertexInputLayouts.size();
                m_vertexInputLayouts.pushBack(state);
                m_vertexInputLayoutMap[state] = stateIndex;
            }

            // remember this type as well
            DEBUG_CHECK(stateIndex != INVALID_PIPELINE_INDEX);
            return stateIndex;
        }

        static void TranslateResourceType(const DataType& t, rendering::ResourceType& outType, rendering::ResourceAccess& outAccess)
        {
            outAccess = rendering::ResourceAccess::ReadOnly;

            const auto& info = t.resource();
            if (info.type == "Texture")
                outType = rendering::ResourceType::Texture;
            else if (info.type == "Buffer")
                outType = rendering::ResourceType::Buffer;
            else if (info.type == "ConstantBuffer")
                outType = rendering::ResourceType::Constants;

            if (info.uav)
            {
                outAccess = rendering::ResourceAccess::UAVReadWrite;

                if (info.attributes.has("writeonly"_id))
                    outAccess = rendering::ResourceAccess::UAVWriteOnly;
                else if (info.attributes.has("readonly"_id))
                    outAccess = rendering::ResourceAccess::UAVReadOnly;
            }            
        }

        PipelineIndex ShaderLibraryBuilder::mapParameterLayout(const ResourceTable* resourceTable)
        {
            // invalid
            if (!resourceTable)
                return INVALID_PIPELINE_INDEX;

            // maybe we are already mapped
            {
                PipelineIndex index = 0;
                if (m_resourceTableMap.find(resourceTable, index))
                    return index;
            }

            // collect used resource properties
            base::InplaceArray<PipelineIndex, 32> elements;
            for (auto& member : resourceTable->members())
            {
                // TODO: add array support

                // prepare the entry
                ParameterResourceLayoutElement resElement;
                resElement.name = mapName(member.m_name.view());
                resElement.format = member.m_type.resource().resolvedFormat;
                resElement.layout = mapDataLayout(member.m_type.resource().resolvedLayout);
                resElement.maxArrayCount = 0; // TODO

                // translate compiler side type info something we can carry in other parts of rendering
                TranslateResourceType(member.m_type, resElement.type, resElement.access);
                DEBUG_CHECK(resElement.type != rendering::ResourceType::None);

                // map to index
                PipelineIndex resourceElementIndex = INVALID_PIPELINE_INDEX;
                if (!m_parameterResourceLayoutElementMap.find(resElement, resourceElementIndex))
                {
                    resourceElementIndex = m_parameterResourceLayoutElements.size();
                    m_parameterResourceLayoutElements.pushBack(resElement);
                    m_parameterResourceLayoutElementMap[resElement] = resourceElementIndex;
                }

                elements.pushBack(resourceElementIndex);
            }

            // map the whole element range
            const auto indirectIndex = mapIndirectIndices(elements.typedData(), elements.size());
            DEBUG_CHECK_EX(elements.empty() || indirectIndex != INVALID_PIPELINE_INDEX, "Failed to map layout elements");

            // create element
            ParameterResourceLayoutTable table;
            table.name = mapName(resourceTable->name().view());
            table.firstElementIndex = indirectIndex;
            table.numElements = elements.size();
            table.structureKey = computeResourceTableKey(table);

            // map
            PipelineIndex tableIndex = INVALID_PIPELINE_INDEX;;
            if (!m_parameterResourceLayoutTableMap.find(table, tableIndex))
            {
                tableIndex = m_parameterResourceLayoutTables.size();
                m_parameterResourceLayoutTables.pushBack(table);
                m_parameterResourceLayoutTableMap[table] = tableIndex;
            }

            // remember this type as well
            DEBUG_CHECK(tableIndex != INVALID_PIPELINE_INDEX);
            m_parameterResourceLayoutTableMap[table] = tableIndex;
            m_resourceTableMap[resourceTable] = tableIndex; 
            return tableIndex;
        }

        PipelineIndex ShaderLibraryBuilder::mapParameterBindingState(const ParameterBindingState& state)
        {
            PipelineIndex index = INVALID_PIPELINE_INDEX;
            if (!m_parameterBindingStateMap.find(state, index))
            {
                index = m_parameterBindingStates.size();
                m_parameterBindingStates.pushBack(state);
                m_parameterBindingStateMap[state] = index;
            }
            return index;
        }

        PipelineIndex ShaderLibraryBuilder::mapShaderDataBlob(ShaderType shaderType, const void* data, uint32_t dataSize)
        {
            // we cannot map empty blob
            if (!dataSize || !data)
                return INVALID_PIPELINE_INDEX;

            // compute data hash, that's the key into the hashmap
            auto dataHash = base::CRC64().append(data, dataSize).crc();

            // find existing blob
            PipelineIndex blobIndex = INVALID_PIPELINE_INDEX;
            if (m_shaderBlobMap.find(dataHash, blobIndex))
            {
                // TODO: validate content?
                return blobIndex;
            }


            ShaderBlob blob;
            blob.type = shaderType;
            blob.dataHash = dataHash;
            blob.unpackedSize = dataSize;
            blob.offset = m_shaderData.size();

            // compress blob data 
            const auto cutoffSize = dataSize * 9 / 10; // 90% of original size
            auto compressedData = base::Compress(base::CompressionType::LZ4HC, data, dataSize, POOL_COMPILED_SHADER_DATA);
            if (compressedData && compressedData.size() <= cutoffSize)
            {
                // store compressed data
                m_shaderData.write(compressedData.data(), compressedData.size());
                blob.packedSize = compressedData.size();
            }
            else
            {
                // compression did not help much, store uncompressed
                m_shaderData.write((const uint8_t*)data, dataSize);
                blob.packedSize = dataSize;
            }

            // store blob
            blobIndex = m_shaderBlobs.size();
            m_shaderBlobs.pushBack(blob);
            m_shaderBlobMap[dataHash] = blobIndex;
            return blobIndex;
        }

        PipelineIndex ShaderLibraryBuilder::mapShaderBundle(const ShaderBundle& state)
        {
            PipelineIndex index = INVALID_PIPELINE_INDEX;
            if (!m_shaderBundleMap.find(state, index))
            {
                index = m_shaderBundles.size();
                m_shaderBundles.pushBack(state);
                m_shaderBundleMap[state] = index;
            }
            return index;
        }

        PipelineIndex ShaderLibraryBuilder::mapIndirectIndices(const PipelineIndex* indices, uint32_t count/* = 1*/)
        {
            if (count == 0)
                return INVALID_PIPELINE_INDEX;

            /*if (count == 1)
                return indices[0];*/

            PipelineIndex index = m_indirectIndices.size();
            auto* writePtr = m_indirectIndices.allocateUninitialized(count);
            memcpy(writePtr, indices, sizeof(PipelineIndex) * count);
            return index;
        }

        //--

        struct MemoryWriterHelper
        {
            MemoryWriterHelper(base::Buffer ret)
                : buffer(ret)
                , writePtr(ret.data())
                , endWritePtr(ret.data() + ret.size())
            {}

            template< typename T >
            T& alloc()
            {
                DEBUG_CHECK(writePtr + sizeof(T) <= endWritePtr);
                auto* ptr = writePtr;
                memset(ptr, 0, sizeof(T));
                writePtr += sizeof(T);
                return *(T*)ptr;
            }

            uint8_t* alloc(uint32_t size)
            {
                DEBUG_CHECK(writePtr + size <= endWritePtr);
                auto* ptr = writePtr;
                writePtr += size;
                return ptr;
            }

            void write(const void* data, uint64_t size)
            {
                DEBUG_CHECK(writePtr + size <= endWritePtr);
                memcpy(writePtr, data, size);
                writePtr += size;
            }

            template< typename T >
            void writeChunkData(uint32_t& offset, uint32_t& count, const base::Array<T>& data)
            {
                offset = writePtr - buffer.data();
                count = data.size();
                write(data.data(), data.dataSize());
            }


            base::Buffer buffer;

            uint8_t* writePtr;
            uint8_t* endWritePtr;
        };

        base::Buffer ShaderLibraryBuilder::extractStructureData() const
        {
            uint64_t totalSizeNeeded = 0;
            totalSizeNeeded += sizeof(ShaderLibraryData::Header);
            totalSizeNeeded += m_stringTable.dataSize();
            totalSizeNeeded += m_nameTable.dataSize();
            totalSizeNeeded += m_dataLayoutElements.dataSize();
            totalSizeNeeded += m_dataLayoutStructures.dataSize();
            totalSizeNeeded += m_vertexInputLayouts.dataSize();
            totalSizeNeeded += m_vertexInputStates.dataSize();
            totalSizeNeeded += m_parameterResourceLayoutElements.dataSize();
            totalSizeNeeded += m_parameterResourceLayoutTables.dataSize();
            totalSizeNeeded += m_parameterBindingStates.dataSize();
            totalSizeNeeded += m_shaderBlobs.dataSize();
            totalSizeNeeded += m_shaderBundles.dataSize();
            totalSizeNeeded += m_indirectIndices.dataSize();

            auto ret = base::Buffer::Create(POOL_COMPILED_SHADER_STRUCTURES, totalSizeNeeded, 16);
            DEBUG_CHECK_EX(ret, "Out of memory when allocating data for compiled shader structs");
            if (!ret)
                return nullptr;

            MemoryWriterHelper writer(ret);

#define WRITE_CHUNK(type_, table_) writer.writeChunkData(header.chunks[ShaderLibraryData::##type_].offset, header.chunks[ShaderLibraryData::##type_].count, table_)
            auto& header = writer.alloc<ShaderLibraryData::Header>();
            WRITE_CHUNK(Chunk_StringTable, m_stringTable);
            WRITE_CHUNK(Chunk_Names, m_nameTable);
            WRITE_CHUNK(Chunk_IndirectIndices, m_indirectIndices);
            WRITE_CHUNK(Chunk_DataElements, m_dataLayoutElements);
            WRITE_CHUNK(Chunk_DataStructures, m_dataLayoutStructures);
            WRITE_CHUNK(Chunk_VertexInputLayouts, m_vertexInputLayouts);
            WRITE_CHUNK(Chunk_VertexInputStates,  m_vertexInputStates);
            WRITE_CHUNK(Chunk_ParameterResourceElements, m_parameterResourceLayoutElements);
            WRITE_CHUNK(Chunk_ParameterResourceTables, m_parameterResourceLayoutTables);
            WRITE_CHUNK(Chunk_ParameterBindingStates, m_parameterBindingStates);
            WRITE_CHUNK(Chunk_ShaderBlobs, m_shaderBlobs);
            WRITE_CHUNK(Chunk_ShaderBundles, m_shaderBundles);
#undef WRITE_CHUNK

            return ret;
        }

        base::Buffer ShaderLibraryBuilder::extractShaderData() const
        {
            return m_shaderData.toBuffer(POOL_COMPILED_SHADER_DATA);
        }

        //--

        void ShaderLibraryBuilder::computeDataElementKey(base::CRC64& crc, PipelineIndex pi) const
        {
            const auto& elem = m_dataLayoutElements[pi];
            crc << elem.alignment;
            crc << elem.arrayCount;
            crc << elem.arrayStride;
            crc << (uint16_t)elem.format;
            crc << elem.offset;
            crc << elem.size;
        }

        void ShaderLibraryBuilder::computeDataStructureKey(base::CRC64& crc, PipelineIndex pi) const
        {
            const auto& elem = m_dataLayoutStructures[pi];

            crc << elem.size;
            crc << elem.alignment;
            crc << elem.numElements;

            for (uint32_t i = 0; i < elem.numElements; ++i)
            {
                auto elementIndex = m_indirectIndices[elem.firstElementIndex + i];
                computeDataElementKey(crc, elementIndex);
            }
        }

        void ShaderLibraryBuilder::computeVertexInputStateKey(base::CRC64& structureKey, const VertexInputState& state) const
        {
            structureKey << state.numStreamLayouts;

            for (uint32_t i = 0; i < state.numStreamLayouts; ++i)
            {
                const auto& elem = m_vertexInputLayouts[m_indirectIndices[i + state.firstStreamLayout]];

                structureKey << elem.instanced;
                structureKey << elem.customStride;

                computeDataStructureKey(structureKey, elem.structureIndex);
            }
        }

        uint64_t ShaderLibraryBuilder::computeVertexInputStateKey(const VertexInputState& state) const
        {
            base::CRC64 structureKey;
            computeVertexInputStateKey(structureKey, state);
            return structureKey;
        }

        void ShaderLibraryBuilder::computeResourceTableKey(base::CRC64& structureKey, const ParameterResourceLayoutTable& state) const
        {
            structureKey << state.numElements;

            for (uint32_t i = 0; i < state.numElements; ++i)
            {
                const auto& elem = m_parameterResourceLayoutElements[m_indirectIndices[i + state.firstElementIndex]];

                structureKey << (uint8_t)elem.type;
                structureKey << (uint16_t)elem.format;
                structureKey << (uint16_t)elem.access;
                structureKey << (uint16_t)elem.maxArrayCount;

                if (elem.layout != INVALID_PIPELINE_INDEX)
                    computeDataStructureKey(structureKey, elem.layout);
            }
        }

        uint64_t ShaderLibraryBuilder::computeResourceTableKey(const ParameterResourceLayoutTable& state) const
        {
            base::CRC64 structureKey;
            computeResourceTableKey(structureKey, state);
            return structureKey;
        }

        void ShaderLibraryBuilder::computeResourceBindingKey(base::CRC64& structureKey, const ParameterBindingState& state) const
        {
            structureKey << state.numParameterLayoutIndices;
            structureKey << state.numStaticSamplers;
            structureKey << state.numDirectConstants;

            for (uint32_t i = 0; i < state.numParameterLayoutIndices; ++i)
            {
                const auto& elem = m_parameterResourceLayoutTables[m_indirectIndices[i + state.firstParameterLayoutIndex]];
                computeResourceTableKey(structureKey, elem);
            }
        }

        uint64_t ShaderLibraryBuilder::computeResourceBindingKey(const ParameterBindingState& state) const
        {
            base::CRC64 structureKey;
            computeResourceBindingKey(structureKey, state);
            return structureKey;
        }

        void ShaderLibraryBuilder::computeShaderBundleKey(base::CRC64& structureKey, const ShaderBundle& state) const
        {
            if (state.parameterBindingState != INVALID_PIPELINE_INDEX)
            {
                const auto& bindings = m_parameterBindingStates[state.parameterBindingState];
                computeResourceBindingKey(structureKey, bindings);
            }

            if (state.vertexBindingState != INVALID_PIPELINE_INDEX)
            {
                const auto& bindings = m_vertexInputStates[state.vertexBindingState];
                computeVertexInputStateKey(structureKey, bindings);
            }

            structureKey << state.numShaders;

            for (uint32_t i = 0; i < state.numShaders; ++i)
            {
                const auto& blob = m_shaderBlobs[m_indirectIndices[i + state.firstShaderIndex]];
                structureKey << (uint8_t)blob.type;
                structureKey << blob.unpackedSize;
                structureKey << blob.dataHash;
            }
        }

        uint64_t ShaderLibraryBuilder::computeShaderBundleKey(const ShaderBundle& state) const
        {
            base::CRC64 structureKey;
            computeShaderBundleKey(structureKey, state);
            return structureKey;
        }

    } // compiler
} // rendering