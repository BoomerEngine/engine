/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver\shaders #]
***/

#include "build.h"
#include "glDriver.h"
#include "glObject.h"
#include "glObjectCache.h"
#include "glShaderLibraryAdapter.h"
#include "glUtils.h"

#include "rendering/driver/include/renderingShaderLibrary.h"

namespace rendering
{
    namespace gl4
    {
        //---

        ShaderLibraryAdapter::ShaderLibraryAdapter(Driver* drv, const ShaderLibraryData* data)
            : Object(drv, ObjectType::ShaderLibraryAdapter)
            , m_data(AddRef(data))
        {
            m_shaderBundleMap.resizeWith(data->numShaderBundles(), INDEX_MAX);
            m_vertexStateMap.resizeWith(data->numVertexInputStates(), nullptr);
            m_parameterBindingStateMap.resizeWith(data->numParameterBindingStates(), nullptr);
        }

        bool ShaderLibraryAdapter::CheckClassType(ObjectType type)
        {
            return type == ObjectType::ShaderLibraryAdapter;
        }

        ShaderLibraryAdapter::~ShaderLibraryAdapter()
        {
            // nothing to release here, all objects are owned by cache
        }

        const ResolvedParameterBindingState* ShaderLibraryAdapter::parameterBindingState(PipelineIndex index) const
        {
            DEBUG_CHECK_EX(index != INVALID_PIPELINE_INDEX, "Invalid index");
            DEBUG_CHECK_EX(index < m_parameterBindingStateMap.size(), "Out of range index");
            if (index != INVALID_PIPELINE_INDEX && index < m_parameterBindingStateMap.size())
            {
                if (const auto* ret = m_parameterBindingStateMap[index])
                    return ret;

                m_parameterBindingStateMap[index] = driver()->objectCache().resolveParametersBinding(data(), index);
                return m_parameterBindingStateMap[index];
            }

            return nullptr;
        }

        const ResolvedVertexBindingState* ShaderLibraryAdapter::vertexState(PipelineIndex index) const
        {
            DEBUG_CHECK_EX(index != INVALID_PIPELINE_INDEX, "Invalid index");
            DEBUG_CHECK_EX(index < m_vertexStateMap.size(), "Out of range index");
            if (index != INVALID_PIPELINE_INDEX && index < m_vertexStateMap.size())
            {
                if (ResolvedVertexBindingState* ret = m_vertexStateMap[index])
                    return ret;

                m_vertexStateMap[index] = driver()->objectCache().resolveVertexLayout(data(), index);
                return m_vertexStateMap[index];
            }

            return nullptr;
        }

        GLuint ShaderLibraryAdapter::shaderBundle(PipelineIndex index) const
        {
            DEBUG_CHECK_EX(index != INVALID_PIPELINE_INDEX, "Invalid index");
            DEBUG_CHECK_EX(index < m_shaderBundleMap.size(), "Out of range index");
            if (index != INVALID_PIPELINE_INDEX && index < m_shaderBundleMap.size())
            {
                GLuint ret = m_shaderBundleMap[index];
                if (ret != INDEX_MAX)
                    return ret;

                m_shaderBundleMap[index] = driver()->objectCache().resolveCompiledShaderBundle(data(), index);
                return m_shaderBundleMap[index];
            }

            return 0;
        }

        //--

    } // gl4
} // driver
