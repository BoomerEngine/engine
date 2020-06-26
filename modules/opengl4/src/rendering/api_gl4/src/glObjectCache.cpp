/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver\objects #]
***/

#include "build.h"
#include "glDriver.h"
#include "glObjectCache.h"
#include "glSampler.h"
#include "glBuffer.h"
#include "glImage.h"
#include "glUtils.h"

#include "rendering/driver/include/renderingShaderLibrary.h"
#include "rendering/driver/include/renderingConstantsView.h"

namespace rendering
{
    namespace gl4
    {
        //---

        base::ConfigProperty<bool> cvDumpCachedObject("Rendering.GL4", "DumpCachedObjects", true);

        //---

        ObjectCache::ObjectCache(Driver* device)
            : m_device(device)
        {
            m_vertexLayoutMap.reserve(256);
            m_vertexBindPointMap.reserve(256);
        }

        ObjectCache::~ObjectCache()
        {
            for (auto value : m_vertexLayoutMap.values())
                GL_PROTECT(glDeleteVertexArrays(1, &value->glVertexArrayObject));

            for (auto value : m_shaderMap.values())
                if (value)
                    GL_PROTECT(glDeleteProgram(value));

            for (auto value : m_programMap.values())
                if (value)
                    GL_PROTECT(glDeleteProgramPipelines(1, &value));

            for (auto value : m_samplerMap.values())
                if (value)
                    GL_PROTECT(glDeleteSamplers(1, &value));

            m_vertexLayoutMap.clearPtr();
            m_paramBindingMap.clearPtr();
        }

        uint16_t ObjectCache::resolveVertexBindPointIndex(base::StringID name)
        {
            uint16_t ret = 0;
            if (m_vertexBindPointMap.find(name, ret))
                return ret;

            ret = m_vertexBindPointMap.size();
            m_vertexBindPointMap[name] = ret;

            if (cvDumpCachedObject.get())
            {
                TRACE_INFO("Cached vertex bind point '{}' -> {}", name, ret);
            }

            return ret;
        }

        uint16_t ObjectCache::resolveParametersBindPointIndex(base::StringID name, ParametersLayoutID layout)
        {
            DEBUG_CHECK_EX(layout, "Invalid layout used");

            UniqueParamBindPointKey key;
            key.name = name;
            key.layout = layout;

            uint16_t ret = 0;
            if (m_paramBindPointMap.find(key, ret))
                return ret;

            ret = m_paramBindPointMap.size();
            m_paramBindPointMap[key] = ret;
            m_paramBindPointLayouts.pushBack(layout);

            if (cvDumpCachedObject.get())
            {
                TRACE_INFO("Cached parameters bind point '{}' layout '{}' -> {}", name, layout, ret);
            }

            return ret;
        }

        uint16_t ObjectCache::resolveParametersBindPointIndex(const rendering::ShaderLibraryData& shaderLib, PipelineIndex paramLayoutIndex)
        {
            const auto& paramTable = shaderLib.parameterLayouts()[paramLayoutIndex];
            const auto paramTableName = shaderLib.names()[paramTable.name];

            base::InplaceArray<ObjectViewType, 32> viewTypes;
            viewTypes.reserve(paramTable.numElements);

            for (uint32_t i = 0; i < paramTable.numElements; ++i)
            {
                const auto& paramTableElement = shaderLib.parameterLayoutsElements()[shaderLib.indirectIndices()[i + paramTable.firstElementIndex]];
                switch (paramTableElement.type)
                {
                    case ResourceType::Buffer: viewTypes.pushBack(ObjectViewType::Buffer); break;
                    case ResourceType::Constants: viewTypes.pushBack(ObjectViewType::Constants); break;
                    case ResourceType::Texture: viewTypes.pushBack(ObjectViewType::Image); break;
                    default: DEBUG_CHECK(!"Invalid resource type");
                }
            }

            auto layoutId = ParametersLayoutID::FromObjectTypes(viewTypes.typedData(), viewTypes.size());
            return resolveParametersBindPointIndex(paramTableName, layoutId);
        }

        ResolvedVertexBindingState* ObjectCache::resolveVertexLayout(const rendering::ShaderLibraryData& shaderLib, PipelineIndex vertexInputStateIndex)
        {
            DEBUG_CHECK(vertexInputStateIndex != INVALID_PIPELINE_INDEX);
            const auto& srcBinding = shaderLib.vertexInputStates()[vertexInputStateIndex];

            // use cached one
            ResolvedVertexBindingState* ret = nullptr;
            if (m_vertexLayoutMap.find(srcBinding.structureKey, ret))
                return ret;

            // create the return wrapper
            ret = MemNew(ResolvedVertexBindingState);
            ret->vertexBindPoints.reserve(srcBinding.numStreamLayouts);

            // create the VAO
            PC_SCOPE_LVL0(ResolveVertexLayout);
            GL_PROTECT(glCreateVertexArrays(1, &ret->glVertexArrayObject));

            // create the attribute mapping from all active streams
            uint32_t attributeIndex = 0;
            for (uint32_t i = 0; i < srcBinding.numStreamLayouts; ++i)
            {
                auto vertexLayoutIndex = shaderLib.indirectIndices()[srcBinding.firstStreamLayout + i];
                DEBUG_CHECK_EX(vertexLayoutIndex != INVALID_PIPELINE_INDEX, "Invalid layout index");

                // get the data layout description for the vertex layout
                auto& vertexLayout = shaderLib.vertexInputLayouts()[vertexLayoutIndex];
                auto& dataStructure = shaderLib.dataLayoutStructures()[vertexLayout.structureIndex];

                // get name of this binding point
                auto& bindingPointInfo = ret->vertexBindPoints.emplaceBack();
                bindingPointInfo.name = shaderLib.names()[vertexLayout.name];
                bindingPointInfo.stride = vertexLayout.customStride ? vertexLayout.customStride : dataStructure.size;
                bindingPointInfo.bindPointIndex = resolveVertexBindPointIndex(bindingPointInfo.name);
                bindingPointInfo.instanced = vertexLayout.instanced;

                // is this instanced stream ?
                auto isInstanceData = vertexLayout.instanced;
                GL_PROTECT(glVertexArrayBindingDivisor(ret->glVertexArrayObject, i, isInstanceData ? 1 : 0));

                // setup the vertex attributes as in the layout
                for (uint32_t j = 0; j < dataStructure.numElements; ++j)
                {
                    auto structureElementIndex = shaderLib.indirectIndices()[dataStructure.firstElementIndex + j];
                    auto& dataElement = shaderLib.dataLayoutElements()[structureElementIndex];

                    // translate the format
                    auto glFormat = TranslateImageFormat(dataElement.format);

                    // convert to old school format
                    GLenum glBaseFormat = 0;
                    GLuint glNumComponents = 0;
                    GLboolean glFormatNormalized = GL_FALSE;
                    DecomposeVertexFormat(glFormat, glBaseFormat, glNumComponents, glFormatNormalized);

                    GL_PROTECT(glVertexArrayAttribFormat(ret->glVertexArrayObject, attributeIndex, glNumComponents, glBaseFormat, glFormatNormalized, dataElement.offset));
                    GL_PROTECT(glVertexArrayAttribBinding(ret->glVertexArrayObject, attributeIndex, i)); // attribute {attributeIndex} in buffer {i}
                    GL_PROTECT(glEnableVertexArrayAttrib(ret->glVertexArrayObject, attributeIndex)); // we use consecutive attribute indices

                    // shader expects the attributes to be numbered in particular order
                    attributeIndex += 1;
                }
            }

            if (cvDumpCachedObject.get())
            {
                TRACE_INFO("Cached vertex binding state {}:", ret->glVertexArrayObject);

                for (uint32_t i = 0; i < ret->vertexBindPoints.size(); ++i)
                {
                    const auto& info = ret->vertexBindPoints[i];
                    TRACE_INFO("  [{}]: {} ({}), stride {} {}", i, info.name, info.bindPointIndex, info.stride, info.instanced ? "INSTANCED" : "");
                }
            }

            m_vertexLayoutMap[srcBinding.structureKey] = ret;
            return ret;
        }

        GLuint ObjectCache::resolveCompiledShader(const rendering::ShaderLibraryData& shaderLib, PipelineIndex shaderIndex, GLenum shaderType)
        {
            DEBUG_CHECK(shaderIndex != INVALID_PIPELINE_INDEX);
            const auto& setup = shaderLib.shaderBlobs()[shaderIndex];

            // use cached one
            GLuint ret = 0;
            if (m_shaderMap.find(setup.dataHash, ret))
                return ret;

            // decompress or use existing data
            void* shaderData = nullptr;
            base::Buffer decompressionBuffer;
            if (setup.packedSize == setup.unpackedSize)
            {
                // shader data is not compressed, use the data directly
                shaderData = (uint8_t*)shaderLib.packedShaderData() + setup.offset;
            }
            else
            {
                PC_SCOPE_LVL0(DecompressShader);
                // decompress into temporary memory
                const void* sourceData = shaderLib.packedShaderData() + setup.offset;
                decompressionBuffer = base::mem::Decompress(base::mem::CompressionType::LZ4HC, sourceData, setup.packedSize, setup.unpackedSize);
                if (!decompressionBuffer)
                    return 0;

                // use decompressed data
                shaderData = decompressionBuffer.data();
            }

            // create new shader
            PC_SCOPE_LVL0(ResolveCompiledShader);
            auto shaderID = glCreateShader(shaderType);
            GL_PROTECT(glShaderSource(shaderID, 1, (const char**)&shaderData, nullptr));
            base::mem::PoolStats::GetInstance().notifyAllocation(POOL_GL_SHADERS, setup.unpackedSize);

            // compile the shader
            GL_PROTECT(glCompileShader(shaderID));

            // check compilation error
            GLint shaderCompiled = 0;;
            GL_PROTECT(glGetShaderiv(shaderID, GL_COMPILE_STATUS, &shaderCompiled));
            if (shaderCompiled != GL_TRUE)
            {
                // get the compilation error
                GLsizei messageSize = 0;
                GLchar message[4096];
                GL_PROTECT(glGetShaderInfoLog(shaderID, sizeof(message), &messageSize, message));
                message[messageSize] = 0;

                // print the error
                TRACE_ERROR("Shader {} compilation error!: {}", shaderIndex, message);
                TRACE_ERROR("{}", (const char*)shaderData);

                // cleanup
                GL_PROTECT(glDeleteShader(shaderID));
                m_shaderMap[setup.dataHash] = 0;
                return 0;
            }

            // create shader program from this shader
            GLuint programID = 0;
            if (shaderID)
            {
                GL_PROTECT(programID = glCreateProgram());
                base::mem::PoolStats::GetInstance().notifyAllocation(POOL_GL_PROGRAMS, 1);

                // use the separable programs if possible
               // if (shaderType != GL_COMPUTE_SHADER)
                {
                    GL_PROTECT(glProgramParameteri(programID, GL_PROGRAM_SEPARABLE, GL_TRUE));
                }

                // link shader into program
                GL_PROTECT(glAttachShader(programID, shaderID));
                GL_PROTECT(glLinkProgram(programID));

                // check if the linking worked
                GLint linkStatus = GL_TRUE;
                GL_PROTECT(glGetProgramiv(programID, GL_LINK_STATUS, &linkStatus));
                GL_PROTECT(glDetachShader(programID, shaderID));

                // link failed
                if (linkStatus != GL_TRUE)
                {
                    // get linking error
                    int bufferSize = 0;
                    GL_PROTECT(glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &bufferSize));
                    if (bufferSize > 0)
                    {
                        base::Array<char> buffer;
                        buffer.resize(bufferSize);
                        GL_PROTECT(glGetProgramInfoLog(programID, bufferSize, &bufferSize, buffer.typedData()));

                        // print the error
                        TRACE_ERROR("Shader {} linking error!: {}", shaderIndex, buffer.typedData());
                        TRACE_ERROR("{}", (const char*)shaderData);

                        // cleanup
                        GL_PROTECT(glDeleteProgram(programID));
                        m_shaderMap[setup.dataHash] = 0;
                        return 0;
                    }
                }

                // validate program
                GL_PROTECT(glValidateProgram(programID));
            }

            // discard shader
            GL_PROTECT(glDeleteShader(shaderID));

            if (cvDumpCachedObject.get())
            {
                TRACE_INFO("Cached shader {}, type {}, hash {}", programID, setup.type, setup.dataHash);
            }

            // store compiled and linked program
            m_shaderMap[setup.dataHash] = programID;
            return programID;
        }

        GLuint ObjectCache::resolveCompiledShaderBundle(const rendering::ShaderLibraryData& shaderLib, PipelineIndex programIndex)
        {
            DEBUG_CHECK(programIndex != INVALID_PIPELINE_INDEX);
            const auto& setup = shaderLib.shaderBundles()[programIndex];

            // use cached one
            GLuint ret = 0;
            if (m_programMap.find(setup.bundleKey, ret))
                return ret;

            // create pipeline object
            PC_SCOPE_LVL0(ResolveCompiledShaderBundle);
            GL_PROTECT(glCreateProgramPipelines(1, &ret));

            // link shaders
            for (uint32_t i = 0; i < setup.numShaders; ++i)
            {
                const auto shaderIndex = shaderLib.indirectIndices()[setup.firstShaderIndex + i];
                const auto& shaderEntry = shaderLib.shaderBlobs()[shaderIndex];

                GLuint glShaderType = 0;
                GLuint glShaderBit = 0;
                switch (shaderEntry.type)
                {
                case ShaderType::Pixel: glShaderType = GL_FRAGMENT_SHADER; glShaderBit = GL_FRAGMENT_SHADER_BIT; break;
                case ShaderType::Vertex: glShaderType = GL_VERTEX_SHADER; glShaderBit = GL_VERTEX_SHADER_BIT; break;
                case ShaderType::Geometry: glShaderType = GL_GEOMETRY_SHADER; glShaderBit = GL_GEOMETRY_SHADER_BIT; break;
                case ShaderType::Compute: glShaderType = GL_COMPUTE_SHADER; glShaderBit = GL_COMPUTE_SHADER_BIT; break;
                case ShaderType::Hull: glShaderType = GL_TESS_CONTROL_SHADER; glShaderBit = GL_TESS_CONTROL_SHADER_BIT; break;
                case ShaderType::Domain: glShaderType = GL_TESS_EVALUATION_SHADER; glShaderBit = GL_TESS_EVALUATION_SHADER_BIT; break;
                default: DEBUG_CHECK(!"Unknown shader type");
                }

                if (glShaderType != 0)
                {
                    auto compiledShader = resolveCompiledShader(shaderLib, shaderIndex, glShaderType);
                    DEBUG_CHECK_EX(compiledShader, "Shader was not compiled by OpenGL");
                    GL_PROTECT(glUseProgramStages(ret, glShaderBit, compiledShader));
                }
            }

            if (cvDumpCachedObject.get())
            {
                TRACE_INFO("Cached linked program {}", ret);
            }

            // store in cache
            m_programMap[setup.bundleKey] = ret;
            return ret;
        }

        static GLuint TranslateReadWriteMode(rendering::ResourceAccess access)
        {
            switch (access)
            {
            case rendering::ResourceAccess::UAVReadWrite: return GL_READ_WRITE;
            case rendering::ResourceAccess::UAVWriteOnly: return GL_WRITE_ONLY;
            }
            return GL_READ_ONLY;
        }

        ResolvedParameterBindingState* ObjectCache::resolveParametersBinding(const rendering::ShaderLibraryData& shaderLib, PipelineIndex parameterBindingStateIndex)
        {
            DEBUG_CHECK(parameterBindingStateIndex != INVALID_PIPELINE_INDEX);
            const auto& bindingSetup = shaderLib.parameterBindingStates()[parameterBindingStateIndex];
            
            // already cached ?
            ResolvedParameterBindingState* ret = nullptr;
            if (m_paramBindingMap.find(bindingSetup.structureKey, ret))
                return ret;

            // binding slot assignment
            // NOTE: MUST MATCH THE ONE IN GLSL CODE GENERATOR!!!!
            uint8_t numUniformBuffers = 0;
            uint8_t numStorageBuffers = 0;
            uint8_t numImages = 0;
            uint8_t numTextures = 0;
            uint8_t numSamplers = 0;

            // create new state
            ret = MemNew(ResolvedParameterBindingState);
            for (uint32_t i = 0; i < bindingSetup.numParameterLayoutIndices; ++i)
            {
                const auto& parameterLayoutIndex = shaderLib.indirectIndices()[i + bindingSetup.firstParameterLayoutIndex];
                const auto parameterLayoutBindingIndex = resolveParametersBindPointIndex(shaderLib, parameterLayoutIndex);

                const auto& parameterLayoutData = shaderLib.parameterLayouts()[parameterLayoutIndex];
                const auto parameterLayoutName = shaderLib.names()[parameterLayoutData.name];

                uint32_t structureOffset = 0;
                for (uint32_t j = 0; j < parameterLayoutData.numElements; ++j)
                {
                    const auto& parameterElementIndex = shaderLib.indirectIndices()[j + parameterLayoutData.firstElementIndex];
                    const auto& parameterElementData = shaderLib.parameterLayoutsElements()[parameterElementIndex];
                    const auto parameterElementName = shaderLib.names()[parameterElementData.name];

                    auto& bindingElement = ret->bindingElements.emplaceBack();
                    bindingElement.bindPointName = parameterLayoutName;
                    bindingElement.bindPointLayout = m_paramBindPointLayouts[parameterLayoutBindingIndex];
                    bindingElement.paramName = parameterElementName;
                    bindingElement.bindPointIndex = parameterLayoutBindingIndex;
                    bindingElement.offsetToView = structureOffset;
                    bindingElement.objectReadWriteMode = TranslateReadWriteMode(parameterElementData.access);
                    bindingElement.objectFormat = parameterElementData.format;

                    switch (parameterElementData.type)
                    {
                        case rendering::ResourceType::Constants:
                        {
                            bindingElement.objectSlot = numUniformBuffers++;
                            bindingElement.objectPackingType = ResolvedParameterBindingState::PackingType::Constants;
                            bindingElement.objectType = ObjectViewType::Constants;
                            structureOffset += sizeof(ConstantsView);
                            break;
                        }

                        case rendering::ResourceType::Texture:
                        {
                            if (parameterElementData.access == rendering::ResourceAccess::ReadOnly)
                            {
                                bindingElement.objectSlot = numTextures++;
                                bindingElement.objectPackingType = ResolvedParameterBindingState::PackingType::Texture;
                            }
                            else
                            {
                                bindingElement.objectSlot = numImages++;
                                bindingElement.objectPackingType = ResolvedParameterBindingState::PackingType::Image;
                            }
    
                            bindingElement.objectType = ObjectViewType::Image;
                            structureOffset += sizeof(ImageView);
                            break;
                        }

                        case rendering::ResourceType::Buffer:
                        {
                            if (parameterElementData.layout != INVALID_PIPELINE_INDEX)
                            {
                                bindingElement.objectPackingType = ResolvedParameterBindingState::PackingType::StorageBuffer;
                                bindingElement.objectSlot = numStorageBuffers++;
                            }
                            else
                            {
                                bindingElement.objectPackingType = ResolvedParameterBindingState::PackingType::Buffer;
                                bindingElement.objectSlot = numImages++;
                            }

                            bindingElement.objectType = ObjectViewType::Buffer;
                            structureOffset += sizeof(BufferView);
                            break;
                        }
                        
                        default:
                            DEBUG_CHECK(!"Rendering: Invalid resource type");
                    }
                }   
            }

            // dump
            if (cvDumpCachedObject.get())
            {
                TRACE_INFO("Cached binding state {}:");
                for (uint32_t i = 0; i < ret->bindingElements.size(); ++i)
                {
                    const auto& info = ret->bindingElements[i];
                    TRACE_INFO("  [{}]: {}.{} @ {} (S{}, O{}), {}", i, 
                        info.bindPointName, info.paramName, (uint8_t)info.objectPackingType, 
                        info.bindPointIndex, info.offsetToView, info.objectFormat);
                }
            }

            // store in map
            m_paramBindingMap[bindingSetup.structureKey] = ret;
            return ret;
        }
/*
        ParameterPacker* ObjectCache::resolvedParameterPacker(const rendering::ShaderLibraryData& shaderLib, PipelineIndex resourceTableIndex)
        {
            DEBUG_CHECK(resourceTableIndex != INVALID_PIPELINE_INDEX);
            const auto& setup = shaderLib.parameterLayouts()[resourceTableIndex];

            // use cached one
            ParameterPacker* ret = nullptr;
            if (m_packerMap.find(setup.structureKey, ret))
                return ret;

            // create packer
            ret = MemNew(ParameterPacker, shaderLib, resourceTableIndex);
            m_packerMap[setup.structureKey] = ret;
            return ret;
        }*/


        static GLenum TranslateFilter(const FilterMode mode)
        {
            switch (mode)
            {
            case FilterMode::Nearest:
            {
                return GL_NEAREST;
            }

            case FilterMode::Linear:
            {
                return GL_LINEAR;
            }
            }

            FATAL_ERROR("Invalid filter mode");
            return GL_LINEAR;
        }

        static GLenum TranslateFilter(const FilterMode mode, const MipmapFilterMode mipMode)
        {
            switch (mode)
            {
            case FilterMode::Nearest:
            {
                switch (mipMode)
                {
                case MipmapFilterMode::None: return GL_NEAREST;
                case MipmapFilterMode::Nearest: return GL_NEAREST_MIPMAP_NEAREST;
                case MipmapFilterMode::Linear: return GL_NEAREST_MIPMAP_LINEAR;
                }

                FATAL_ERROR("Invalid mipmap mode");
                return GL_NEAREST;
            }

            case FilterMode::Linear:
            {
                switch (mipMode)
                {
                case MipmapFilterMode::None: return GL_LINEAR;
                case MipmapFilterMode::Nearest: return GL_LINEAR_MIPMAP_NEAREST;
                case MipmapFilterMode::Linear: return GL_LINEAR_MIPMAP_LINEAR;
                }

                FATAL_ERROR("Invalid mipmap mode");
                return GL_LINEAR;
            }
            }

            FATAL_ERROR("Invalid filter mode");
            return GL_LINEAR;
        }

        static GLenum TranslateAddressMode(const AddressMode mode)
        {
            switch (mode)
            {
            case AddressMode::Wrap: return GL_REPEAT;
            case AddressMode::Mirror: return GL_MIRRORED_REPEAT;
            case AddressMode::ClampToEdge: return GL_CLAMP_TO_EDGE;
            case AddressMode::ClampToBorder: return GL_CLAMP_TO_BORDER;
            case AddressMode::MirrorClampToEdge: return GL_MIRROR_CLAMP_TO_EDGE;
            }

            FATAL_ERROR("Invalid mipmap filter mode");
            return GL_REPEAT;
        }

        static GLenum TranslateCompareOp(const CompareOp mode)
        {
            switch (mode)
            {
            case CompareOp::Never: return GL_NEVER;
            case CompareOp::Less: return GL_LESS;
            case CompareOp::LessEqual: return GL_LEQUAL;
            case CompareOp::Greater: return GL_GREATER;
            case CompareOp::NotEqual: return GL_NOTEQUAL;
            case CompareOp::GreaterEqual: return GL_GEQUAL;
            case CompareOp::Always: return GL_ALWAYS;
            }

            FATAL_ERROR("Invalid depth comparision op");
            return GL_ALWAYS;
        }

        static base::Vector4 TranslateBorderColor(const BorderColor mode)
        {
            switch (mode)
            {
            case BorderColor::FloatTransparentBlack: return base::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
            case BorderColor::FloatOpaqueBlack: return base::Vector4(0.0f, 0.0f, 0.0f, 1.0f);
            case BorderColor::FloatOpaqueWhite: return base::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
            case BorderColor::IntTransparentBlack: return base::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
            case BorderColor::IntOpaqueBlack: return base::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
            case BorderColor::IntOpaqueWhite: return base::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
            }

            FATAL_ERROR("Invalid mipmap filter mode");
            return base::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
        }

        GLuint ObjectCache::resolveSampler(const rendering::SamplerState& sampler)
        {
            // use cached one
            GLuint ret = 0;
            if (m_samplerMap.find(sampler, ret))
                return ret;

            auto borderColor = TranslateBorderColor(sampler.borderColor);

            // create sampler object
            GL_PROTECT(glGenSamplers(1, &ret));
            GL_PROTECT(glSamplerParameteri(ret, GL_TEXTURE_MIN_FILTER, TranslateFilter(sampler.minFilter, sampler.mipmapMode)));
            GL_PROTECT(glSamplerParameteri(ret, GL_TEXTURE_MAG_FILTER, TranslateFilter(sampler.magFilter)));
            GL_PROTECT(glSamplerParameteri(ret, GL_TEXTURE_WRAP_S, TranslateAddressMode(sampler.addresModeU)));
            GL_PROTECT(glSamplerParameteri(ret, GL_TEXTURE_WRAP_T, TranslateAddressMode(sampler.addresModeV)));
            GL_PROTECT(glSamplerParameteri(ret, GL_TEXTURE_WRAP_R, TranslateAddressMode(sampler.addresModeW)));
            GL_PROTECT(glSamplerParameterfv(ret, GL_TEXTURE_BORDER_COLOR, (GLfloat*)&borderColor));
            GL_PROTECT(glSamplerParameterf(ret, GL_TEXTURE_MIN_LOD, sampler.minLod));
            GL_PROTECT(glSamplerParameterf(ret, GL_TEXTURE_MAX_LOD, sampler.maxLod));
            GL_PROTECT(glSamplerParameterf(ret, GL_TEXTURE_LOD_BIAS, sampler.mipLodBias));
            GL_PROTECT(glSamplerParameteri(ret, GL_TEXTURE_COMPARE_MODE, sampler.compareEnabled ? GL_COMPARE_REF_TO_TEXTURE : GL_NONE));
            GL_PROTECT(glSamplerParameteri(ret, GL_TEXTURE_COMPARE_FUNC, TranslateCompareOp(sampler.compareOp)));

            m_samplerMap[sampler] = ret;
            return ret;
        }

        //---

    } // gl4
} // driver