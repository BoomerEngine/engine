/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shaders #]
***/

#pragma once

#include "renderingDeviceObject.h"

namespace rendering
{

    namespace compiler
    {
        class ShaderLibraryBuilder;
    } // compiler
        
#pragma pack(push)
#pragma pack(1)

    //----

    /// data layout element (structure filed)
    struct RENDERING_DEVICE_API DataLayoutElement
    {
        PipelineStringIndex name = INVALID_PIPELINE_INDEX; // name of the element

        uint16_t offset = 0; // offset since the base offset of the containing structure
        uint16_t size = 0; // size of the data element
        uint16_t alignment = 0; // alignment required by this member
        uint16_t arrayCount = 0; // number of elements in array, 0 if it's a scalar
        uint16_t arrayStride = 0; // stride of single array element, 0 if it's a scalar

        PipelineIndex structureIndex = INVALID_PIPELINE_INDEX; // index to the structure if this is a structure or 0 if it's not a structure
        ImageFormat format = ImageFormat::UNKNOWN; // format of the stored data (FLOAT, FLOAT2, etc..), UNKNOWN for the structures

        //--

        static uint32_t CalcHash(const DataLayoutElement& key);

        bool operator==(const DataLayoutElement & other) const;
        bool operator!=(const DataLayoutElement & other) const;
    };

    /// data layout structure (collection of elements)
    struct RENDERING_DEVICE_API DataLayoutStructure
    {
        PipelineIndex name = INVALID_PIPELINE_INDEX; // name of the structure

        uint16_t alignment = 0; // alignment required by this data (in bytes, usually 16)
        uint16_t size = 0; // size of the data (not padded)

        PipelineIndex firstElementIndex = INVALID_PIPELINE_INDEX; // first layout element
        uint16_t numElements = 0; // number of layout elements

        //--

        static uint32_t CalcHash(const DataLayoutStructure& key);

        bool operator==(const DataLayoutStructure& other) const;
        bool operator!=(const DataLayoutStructure& other) const;
    };

    ///---

    /// vertex layout - group of elements in the same buffer
    struct RENDERING_DEVICE_API VertexInputLayout
    {
        PipelineIndex name = INVALID_PIPELINE_INDEX; // name of the binding point
        PipelineIndex structureIndex = INVALID_PIPELINE_INDEX; // layout structure
        uint32_t customStride = 0; // custom data stride (if specified)
        uint8_t instanced = 0; // is this data for instancing ?

        //--

        static uint32_t CalcHash(const VertexInputLayout& key);

        bool operator==(const VertexInputLayout& other) const;
        bool operator!=(const VertexInputLayout& other) const;
    };

    ///---

    /// vertex input configuration - configuration of selected vertex layouts and streams
    struct RENDERING_DEVICE_API VertexInputState
    {
        PipelineIndex firstStreamLayout = INVALID_PIPELINE_INDEX;
        uint8_t numStreamLayouts = 0;
        uint64_t structureKey = 0; // unique to layout, not name or other data

        //--

        static uint32_t CalcHash(const VertexInputState& key);

        bool operator==(const VertexInputState& other) const;
        bool operator!=(const VertexInputState& other) const;
    };

    //----

    struct RENDERING_DEVICE_API ParameterResourceLayoutElement
    {
        PipelineIndex name = INVALID_PIPELINE_INDEX; 

        ResourceType type = ResourceType::None;
        ResourceAccess access = ResourceAccess::ReadOnly;
        PipelineIndex layout = INVALID_PIPELINE_INDEX; // StructuredBuffers/ConstantBuffers
        ImageFormat format = ImageFormat::UNKNOWN; // Textures/RWTextures/Buffer/RWBuffer

        uint16_t maxArrayCount = 0;

        //--

        static uint32_t CalcHash(const ParameterResourceLayoutElement& key);

        bool operator==(const ParameterResourceLayoutElement& other) const;
        bool operator!=(const ParameterResourceLayoutElement& other) const;
    };

    struct RENDERING_DEVICE_API ParameterResourceLayoutTable
    {
        PipelineIndex name = INVALID_PIPELINE_INDEX;

        PipelineIndex firstElementIndex = INVALID_PIPELINE_INDEX;
        uint8_t numElements = 0;

        uint64_t structureKey = 0; // unique to layout, not name or other data

        //--

        static uint32_t CalcHash(const ParameterResourceLayoutTable& key);

        bool operator==(const ParameterResourceLayoutTable& other) const;
        bool operator!=(const ParameterResourceLayoutTable& other) const;
    };

    //----

    struct RENDERING_DEVICE_API ParameterBindingState
    {
        PipelineIndex firstParameterLayoutIndex = 0;
        uint8_t numParameterLayoutIndices = 0;

        PipelineIndex firstStaticSampler = 0;
        uint8_t numStaticSamplers = 0;

        PipelineIndex firstDirectConstant = 0;
        uint8_t numDirectConstants = 0;

        uint64_t structureKey = 0; // unique to layout, not name or other data

        //--

        static uint32_t CalcHash(const ParameterBindingState& key);

        bool operator==(const ParameterBindingState& other) const;
        bool operator!=(const ParameterBindingState& other) const;
    };

    //----

    /// shader bundle - set of shaders with common root signature, vertex bindings, etc
    struct RENDERING_DEVICE_API ShaderBundle
    {
        PipelineIndex vertexBindingState = INVALID_PIPELINE_INDEX; // only if we have vertex shader

        PipelineIndex parameterBindingState = INVALID_PIPELINE_INDEX;

        PipelineIndex firstShaderIndex = INVALID_PIPELINE_INDEX;
        uint8_t numShaders = 0;

        uint64_t bundleKey = 0; // identifies the unique combination of shaders and pipeline states, can be used as "pipeline object" key

        //--

        static uint32_t CalcHash(const ShaderBundle& key);

        bool operator==(const ShaderBundle& other) const;
        bool operator!=(const ShaderBundle& other) const;
    };

    //---

    /// compiled shader data
    struct RENDERING_DEVICE_API ShaderBlob
    {
        ShaderType type = ShaderType::Pixel; // shader type (pixel, vertex, etc)
        uint32_t offset = 0; // offset in the fat buffer to the shader data
        uint32_t packedSize = 0; // size of the shader data when compressed
        uint32_t unpackedSize = 0; // size of the shader data in memory when uncompressed
        uint64_t dataHash = 0; // hash of the output data

        //--

        static uint32_t CalcHash(const ShaderBlob& key);

        bool operator==(const ShaderBlob& other) const;
        bool operator!=(const ShaderBlob& other) const;
    };

#pragma pack(pop)

    //----

    // data storage for shared library - immutable (as opposite to the ShaderLibrary resource that can be reloaded)
    class RENDERING_DEVICE_API ShaderLibraryData : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ShaderLibraryData, base::IObject);

    public:
        ShaderLibraryData();
        ShaderLibraryData(base::Buffer structureData, base::Buffer shaderData);

        //--

        INLINE const base::StringID* names() const { return m_names.typedData(); }

        INLINE const PipelineIndex* indirectIndices() const { return (const PipelineIndex*)chunkData(Chunk_IndirectIndices); }

        INLINE const DataLayoutElement* dataLayoutElements() const { return (const DataLayoutElement*)chunkData(Chunk_DataElements); }
        INLINE const DataLayoutStructure* dataLayoutStructures() const { return (const DataLayoutStructure*)chunkData(Chunk_DataStructures); }

        INLINE const VertexInputLayout* vertexInputLayouts() const { return (const VertexInputLayout*)chunkData(Chunk_VertexInputLayouts); }
        INLINE const VertexInputState* vertexInputStates() const { return (const VertexInputState*)chunkData(Chunk_VertexInputStates); }

        INLINE const ParameterResourceLayoutElement* parameterLayoutsElements() const { return (const ParameterResourceLayoutElement*)chunkData(Chunk_ParameterResourceElements); }
        INLINE const ParameterResourceLayoutTable* parameterLayouts() const { return (const ParameterResourceLayoutTable*)chunkData(Chunk_ParameterResourceTables); }
        INLINE const ParameterBindingState* parameterBindingStates() const { return (const ParameterBindingState*)chunkData(Chunk_ParameterBindingStates); }

        INLINE const ShaderBlob* shaderBlobs() const { return (const ShaderBlob*)chunkData(Chunk_ShaderBlobs); }
        INLINE const ShaderBundle* shaderBundles() const { return (const ShaderBundle*)chunkData(Chunk_ShaderBundles); }
        INLINE const uint8_t* packedShaderData() const { return m_shaderData.data(); }


        //--

        INLINE uint32_t numVertexInputLayouts() const { return chunkCount(Chunk_VertexInputLayouts); }
        INLINE uint32_t numVertexInputStates() const { return chunkCount(Chunk_VertexInputStates); }

        INLINE uint32_t numParameterLayoutsElements() const { return chunkCount(Chunk_ParameterResourceElements); }
        INLINE uint32_t numParameterLayouts() const { return chunkCount(Chunk_ParameterResourceTables); }
        INLINE uint32_t numParameterBindingStates() const { return chunkCount(Chunk_ParameterBindingStates); }

        INLINE uint32_t numShaderBlobs() const { return chunkCount(Chunk_ShaderBlobs); }
        INLINE uint32_t numShaderBundles() const { return chunkCount(Chunk_ShaderBundles); }

        //--

        // get total data size of all stuff in the library
        uint64_t dataSize() const;

        //--

        enum ChunkType
        {
            Chunk_StringTable = 0,
            Chunk_Names,
            Chunk_IndirectIndices,
            Chunk_DataElements,
            Chunk_DataStructures,
            Chunk_ParameterBindingStates,
            Chunk_ParameterResourceElements,
            Chunk_ParameterResourceTables,
            Chunk_VertexInputLayouts,
            Chunk_VertexInputStates,
            Chunk_ShaderBlobs,
            Chunk_ShaderBundles,

            Chunk_MAX,
        };

    private:
        base::Buffer m_structureData;
        base::Buffer m_shaderData;

        struct ChunkInfo
        {
            uint32_t offset = 0;
            uint32_t count = 0;
        };

        struct Header
        {
            ChunkInfo chunks[Chunk_MAX];
        };

        INLINE const Header& header() const
        {
            return *(const Header*)m_structureData.data();
        }

        INLINE const uint8_t* chunkData(ChunkType info) const
        {
            return m_structureData.data() + header().chunks[(uint8_t)info].offset;
        }

        INLINE const uint32_t chunkCount(ChunkType info) const
        {
            return header().chunks[(uint8_t)info].count;
        }

        base::Array<base::StringID> m_names;

        virtual void onPostLoad() override;
        void buildMaps();

        //---

        friend class compiler::ShaderLibraryBuilder;
    };

    //----

    /// shaders (as uploaded to device)
    class RENDERING_DEVICE_API ShaderObject : public IDeviceObject
    {
    public:
        ShaderObject(ObjectID id, const ShaderLibraryDataPtr& data, IDeviceObjectHandler* impl);

        // shaders data
        INLINE const ShaderLibraryDataPtr& data() const { return m_data; }

        //--

    private:
        ShaderLibraryDataPtr m_data;
    };

    //----

    /// shader state
    enum class ShaderState : uint8_t
    {
        Loaded, // we are loaded and valid to use
        Loading, // we were created and empty shell and promised to load at some point
        Failed, // we tried to load but failed
    };

    /// all possible states of pipeline aggregated in one setup
    class RENDERING_DEVICE_API ShaderLibrary : public base::res::IResource
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ShaderLibrary, base::res::IResource);

    public:
        ShaderLibrary();
        ShaderLibrary(const ShaderLibraryDataPtr& existingData);
        virtual ~ShaderLibrary();

        //--

        // get current state
        INLINE ShaderState state() const { return m_state.load(); }

        //--

        // get current rendering object, can be null if we are loading
        bool resolve(ObjectID& outObject, PipelineIndex& outIndex, ShaderLibraryDataPtr* outData=nullptr) const;

        // assign data, can happen asynchronously while shader is loading
        void bind(const ShaderObjectPtr& obj, PipelineIndex index);

        //--

    private:
        base::SpinLock m_lock;

        ShaderObjectPtr m_object;
        ShaderLibraryDataPtr m_data; // read only original one
        PipelineIndex m_index = 0;

        std::atomic<ShaderState> m_state;

        virtual void onPostLoad() override;

        void createDeviceResources_NoLock();
        void destroyDeviceResources_NoLock();
    };

    //----

} // rendering
