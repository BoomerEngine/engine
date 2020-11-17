/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver\transient #]
***/

#pragma once

#include "rendering/driver/include/renderingParametersLayoutID.h"

namespace rendering
{
    namespace gl4
    {

        //---

        class Shader;
        class ParameterPacker;

        /// cached vertex state
        struct ResolvedVertexBindingState : public base::mem::GlobalPoolObject<POOL_API_PIPELINES>
        {
            struct BindingInfo
            {
                uint16_t bindPointIndex = 0;
                uint32_t stride = 0;
                base::StringID name;
                bool instanced = 0;
            };

            base::InplaceArray<BindingInfo, 8> vertexBindPoints;
            GLuint glVertexArrayObject = 0;
        };

        /// cached binding
        struct ResolvedParameterBindingState : public base::mem::GlobalPoolObject<POOL_API_PIPELINES>
        {
            enum class PackingType : uint8_t
            {
                NotPacked,
                Constants,
                Texture,
                Image,
                Buffer,
                StorageBuffer,
            };

            struct BindingElement
            {
                uint16_t bindPointIndex = 0; // where to look for "ParameterInfo"
                uint16_t offsetToView = 0; // in parameter struct

                ParametersLayoutID bindPointLayout;
                base::StringID bindPointName;
                base::StringID paramName;

                ObjectViewType objectType = ObjectViewType::Invalid;
                PackingType objectPackingType = PackingType::NotPacked;
                ImageFormat objectFormat = ImageFormat::UNKNOWN;
                uint16_t objectSlot = 0; // in OpenGL
                GLuint objectReadWriteMode = GL_READ_ONLY;
            };

            base::Array<BindingElement> bindingElements;
        };

        /// key for a unique bindpoint
        struct UniqueParamBindPointKey
        {
            base::StringID name;
            ParametersLayoutID layout;

            INLINE UniqueParamBindPointKey() {};

            INLINE static uint32_t CalcHash(const UniqueParamBindPointKey& key) { return base::CRC32() << key.name << key.layout.value(); }

            INLINE bool operator==(const UniqueParamBindPointKey& other) const { return (name == other.name) && (layout == other.layout); }
            INLINE bool operator!=(const UniqueParamBindPointKey& other) const { return !operator==(other); }
        };

        /// cache for most commonly used object
        class ObjectCache : public base::NoCopy, public base::mem::GlobalPoolObject<POOL_API_RUNTIME>
        {
        public:
            ObjectCache(Driver* device);
            ~ObjectCache();

            /// find a bindpoint index by name
            uint16_t resolveVertexBindPointIndex(base::StringID name);

            /// find a parameter index by name and type
            uint16_t resolveParametersBindPointIndex(base::StringID name, ParametersLayoutID layout);

            /// find a parameter index by name and type
            uint16_t resolveParametersBindPointIndex(const rendering::ShaderLibraryData& shaderLib, PipelineIndex paramLayoutIndex);

            /// find/create VBO layout
            ResolvedVertexBindingState* resolveVertexLayout(const rendering::ShaderLibraryData& shaderLib, PipelineIndex vertexInputStateIndex);

            /// get single compiled shader
            GLuint resolveCompiledShader(const rendering::ShaderLibraryData& shaderLib, PipelineIndex shaderIndex, GLenum shaderType);

            /// get compiled shader bundle as GL pipeline with all shaders programs linked
            GLuint resolveCompiledShaderBundle(const rendering::ShaderLibraryData& shaderLib, PipelineIndex pipelineIndex);

            /// resolve the mapping of inputs parameters to actual slots in OpenGL
            ResolvedParameterBindingState* resolveParametersBinding(const rendering::ShaderLibraryData& shaderLib, PipelineIndex parameterBindingState);

            /// get a sampler for given setup
            GLuint resolveSampler(const rendering::SamplerState& sampler);

        private:
            Driver* m_device;

            base::HashMap<base::StringID, uint16_t> m_vertexBindPointMap;
            base::HashMap<UniqueParamBindPointKey, uint16_t> m_paramBindPointMap;
            base::Array<ParametersLayoutID> m_paramBindPointLayouts;

            base::HashMap<uint64_t, ResolvedVertexBindingState*> m_vertexLayoutMap;
            base::HashMap<uint64_t, ResolvedParameterBindingState*> m_paramBindingMap;

            base::HashMap<uint64_t, GLuint> m_shaderMap;
            base::HashMap<uint64_t, GLuint> m_programMap;

            //base::HashMap<uint64_t, ParameterPacker*> m_packerMap;

            base::HashMap<rendering::SamplerState, GLuint> m_samplerMap;
        };

        //---

    } // gl4
} // rendering
