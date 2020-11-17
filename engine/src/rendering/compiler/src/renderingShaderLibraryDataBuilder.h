/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
***/

#pragma once

#include "rendering/driver/include/renderingShaderLibrary.h"
#include "base/containers/include/pagedBuffer.h"
#include "renderingShaderTypeLibrary.h"

namespace rendering
{
    namespace compiler
    {
        /// helper class that maps render states to PipelineIndices and helps to build the vast tables of PipelineLibrary
        class ShaderLibraryBuilder : public base::NoCopy
        {
        public:
            ShaderLibraryBuilder();

            //--

            /// export library data - structures
            base::Buffer extractStructureData() const;

            /// export library data - shaders
            base::Buffer extractShaderData() const;

            //--

            /// map a raw string
            PipelineStringIndex mapString(base::StringView txt);

            /// map a name
            PipelineIndex mapName(base::StringView txt);

            /// map structure for use with rendering, scans the elements and builds similar layout
            /// reuses previously mapped one if found
            PipelineIndex mapDataLayout(const CompositeType* structType);

            /// map vertex binding state (collection of vertex layouts bound together)
            PipelineIndex mapVertexInputState(const VertexInputState& state);

            /// map vertex binding state (collection of vertex layouts bound together)
            PipelineIndex mapVertexInputLayout(const VertexInputLayout& state);

            /// map a resource table layout
            PipelineIndex mapParameterLayout(const ResourceTable *resourceTable);

            /// map a parameter binding state (which layouts in which sets), reuses previously mapped one if found
            PipelineIndex mapParameterBindingState(const ParameterBindingState& state);

            // map shader binary data, reuses existing binary data if possible
            PipelineIndex mapShaderDataBlob(ShaderType shaderType, const void* data, uint32_t dataSize);

            // map group of shaders (shader bundle)
            PipelineIndex mapShaderBundle(const ShaderBundle& state);

            /// map range of indirect indices, returns index to the first one mapped
            PipelineIndex mapIndirectIndices(const PipelineIndex* indices, uint32_t count = 1);

            //---

            // compute structure key
            void computeDataElementKey(base::CRC64& crc, PipelineIndex pi) const;
            void computeDataStructureKey(base::CRC64& crc, PipelineIndex pi) const;
            void computeResourceTableKey(base::CRC64& crc, const ParameterResourceLayoutTable& state) const;
            void computeResourceBindingKey(base::CRC64& crc, const ParameterBindingState& state) const;
            void computeShaderBundleKey(base::CRC64& crc, const ShaderBundle& state) const;
            void computeVertexInputStateKey(base::CRC64& crc, const VertexInputState& state) const;

            uint64_t computeVertexInputStateKey(const VertexInputState& state) const;
            uint64_t computeResourceTableKey(const ParameterResourceLayoutTable& state) const;
            uint64_t computeResourceBindingKey(const ParameterBindingState& state) const;
            uint64_t computeShaderBundleKey(const ShaderBundle& state) const;
            

            //---

            INLINE base::StringID name(PipelineIndex index) const { return base::StringID(m_stringTable.typedData() + m_nameTable[index]); }

            INLINE const base::Array<char>& stringTable() const { return m_stringTable; };
            INLINE const base::Array<PipelineStringIndex>& nameTable() const { return m_nameTable; };

            INLINE const base::Array<DataLayoutElement>& dataLayoutElements() const { return m_dataLayoutElements; }
            INLINE const base::Array<DataLayoutStructure>& dataLayoutStructures() const { return m_dataLayoutStructures; }
            INLINE const base::Array<VertexInputLayout>& vertexInputLayouts() const { return m_vertexInputLayouts; }
            INLINE const base::Array<VertexInputState>& vertexInputStates() const { return m_vertexInputStates; }
            INLINE const base::Array<ParameterResourceLayoutElement>& parameterResourceLayoutElements() const { return m_parameterResourceLayoutElements; }
            INLINE const base::Array<ParameterResourceLayoutTable>& parameterResourceLayoutTables() const { m_parameterResourceLayoutTables; }
            INLINE const base::Array<ParameterBindingState>& parameterBindingStates() const { return m_parameterBindingStates; }
            INLINE const base::Array<ShaderBlob>& shaderBlobs() const { return m_shaderBlobs; }
            INLINE const base::Array<ShaderBundle>& shaderBundles() const { return m_shaderBundles; }
            INLINE const base::Array<PipelineIndex>& indirectIndices() const { return m_indirectIndices; }

        public:
            base::Array<char> m_stringTable;
            base::HashMap<base::StringBuf, PipelineStringIndex> m_stringMap;

            base::Array<PipelineStringIndex> m_nameTable;
            base::HashMap<base::StringBuf, PipelineIndex> m_nameMap;

            base::Array<DataLayoutElement> m_dataLayoutElements;
            base::HashMap<DataLayoutElement, PipelineIndex> m_dataLayoutElementMap;

            base::Array<DataLayoutStructure> m_dataLayoutStructures;
            base::HashMap<DataLayoutStructure, PipelineIndex> m_dataLayoutStructureMap;

            base::Array<VertexInputLayout> m_vertexInputLayouts;
            base::HashMap<VertexInputLayout, PipelineIndex> m_vertexInputLayoutMap;

            base::Array<VertexInputState> m_vertexInputStates;
            base::HashMap<VertexInputState, PipelineIndex> m_vertexInputStateMap;

            base::Array<ParameterResourceLayoutElement> m_parameterResourceLayoutElements;
            base::HashMap<ParameterResourceLayoutElement, PipelineIndex> m_parameterResourceLayoutElementMap;

            base::Array<ParameterResourceLayoutTable> m_parameterResourceLayoutTables;
            base::HashMap<ParameterResourceLayoutTable, PipelineIndex> m_parameterResourceLayoutTableMap;

            base::Array<ParameterBindingState> m_parameterBindingStates;
            base::HashMap<ParameterBindingState, PipelineIndex> m_parameterBindingStateMap;

            base::Array<ShaderBlob> m_shaderBlobs;
            base::HashMap<uint64_t, PipelineIndex> m_shaderBlobMap; // key is the content CRC64

            base::Array<ShaderBundle> m_shaderBundles;
            base::HashMap<ShaderBundle, PipelineIndex> m_shaderBundleMap;

            base::Array<PipelineIndex> m_indirectIndices;

            ///--

            base::HashMap<const CompositeType*, PipelineIndex> m_compositeTypeMap;
            base::HashMap<const ResourceTable*, PipelineIndex> m_resourceTableMap;

            base::PagedBuffer<uint8_t> m_shaderData;

            ///---
        };

    } // compiler
} // rendering
