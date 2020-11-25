/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects\shaders #]
***/

#include "build.h"
#include "glDevice.h"
#include "glObject.h"
#include "glObjectCache.h"
#include "glShaders.h"
#include "glUtils.h"

#include "rendering/device/include/renderingShaderLibrary.h"

namespace rendering
{
    namespace gl4
    {
        //---

        Shaders::Shaders(Device* device, const ShaderLibraryData* data)
            : Object(device, ObjectType::Shaders)
            , m_data(AddRef(data))
        {
            m_shaderBundleMap.resizeWith(data->numShaderBundles(), INDEX_MAX);
            m_vertexStateMap.resizeWith(data->numVertexInputStates(), nullptr);
            m_parameterBindingStateMap.resizeWith(data->numParameterBindingStates(), nullptr);
        }

        Shaders::~Shaders()
        {
            // nothing to release here, all objects are owned by cache
        }

        const ResolvedParameterBindingState* Shaders::parameterBindingState(PipelineIndex index) const
        {
            DEBUG_CHECK_EX(index != INVALID_PIPELINE_INDEX, "Invalid index");
            DEBUG_CHECK_EX(index < m_parameterBindingStateMap.size(), "Out of range index");
            if (index != INVALID_PIPELINE_INDEX && index < m_parameterBindingStateMap.size())
            {
                if (const auto* ret = m_parameterBindingStateMap[index])
                    return ret;

                m_parameterBindingStateMap[index] = device()->objectCache().resolveDescriptorBinding(data(), index);
                return m_parameterBindingStateMap[index];
            }

            return nullptr;
        }

        const ResolvedVertexBindingState* Shaders::vertexState(PipelineIndex index) const
        {
            DEBUG_CHECK_EX(index != INVALID_PIPELINE_INDEX, "Invalid index");
            DEBUG_CHECK_EX(index < m_vertexStateMap.size(), "Out of range index");
            if (index != INVALID_PIPELINE_INDEX && index < m_vertexStateMap.size())
            {
                if (ResolvedVertexBindingState* ret = m_vertexStateMap[index])
                    return ret;

                m_vertexStateMap[index] = device()->objectCache().resolveVertexLayout(data(), index);
                return m_vertexStateMap[index];
            }

            return nullptr;
        }

        GLuint Shaders::shaderBundle(PipelineIndex index) const
        {
            DEBUG_CHECK_EX(index != INVALID_PIPELINE_INDEX, "Invalid index");
            DEBUG_CHECK_EX(index < m_shaderBundleMap.size(), "Out of range index");
            if (index != INVALID_PIPELINE_INDEX && index < m_shaderBundleMap.size())
            {
                GLuint ret = m_shaderBundleMap[index];
                if (ret != INDEX_MAX)
                    return ret;

                m_shaderBundleMap[index] = device()->objectCache().resolveCompiledShaderBundle(data(), index);
                return m_shaderBundleMap[index];
            }

            return 0;
        }

        //--

    } // gl4
} // rendering
