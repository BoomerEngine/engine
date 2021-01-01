/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shader #]
*/

#include "build.h"
#include "renderingShaderFile.h"
#include "renderingShaderData.h"
#include "renderingShaderStaticPermutation.h"
#include "renderingShader.h"

namespace rendering
{

    //--

    StaticShaderPermutation::StaticShaderPermutation(base::StringView path, const ShaderSelector& selector)
        : m_path(path)
        , m_selector(selector)
    {

    }

    StaticShaderPermutation::~StaticShaderPermutation()
    {

    }

    GraphicsPipelineObjectPtr StaticShaderPermutation::loadGraphicsPSO() const
    {
        GraphicsPipelineObjectPtr ret;

        if (auto file = base::LoadResource<ShaderFile>(m_path).acquire())
        {
            if (const auto* data = file->findShader(m_selector))
            {
                if (auto deviceShader = data->deviceShader())
                {
                    ret = deviceShader->createGraphicsPipeline();
                }
            }
        }

        {
            auto lock = base::CreateLock(m_lock);
            if (!ret)
                ret = m_lastValidGraphicsPSO;
            else
                m_lastValidGraphicsPSO = ret;
        }

        return ret;
    }

    ComputePipelineObjectPtr StaticShaderPermutation::loadComputePSO() const
    {
        ComputePipelineObjectPtr ret;

        if (auto file = base::LoadResource<ShaderFile>(m_path).acquire())
        {
            if (const auto* data = file->findShader(m_selector))
            {
                if (auto deviceShader = data->deviceShader())
                {
                    ret = deviceShader->createComputePipeline();
                }
            }
        }
        {
            auto lock = base::CreateLock(m_lock);
            if (!ret)
                ret = m_lastValidComputePSO;
            else
                m_lastValidComputePSO = ret;
        }

        return ret;
    }

    //--

} // rendering
