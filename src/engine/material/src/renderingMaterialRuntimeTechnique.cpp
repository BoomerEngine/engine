/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#include "build.h"
#include "renderingMaterialRuntimeTechnique.h"
#include "gpu/device/include/renderingShaderData.h"
#include "gpu/device/include/renderingShader.h"
#include "gpu/device/include/renderingPipeline.h"

BEGIN_BOOMER_NAMESPACE()
 
///---

MaterialTechniqueRenderStates::MaterialTechniqueRenderStates()
{}

///---

static MaterialCompiledTechnique theEmptyTechnique;

const MaterialCompiledTechnique& MaterialCompiledTechnique::EMPTY()
{
    return theEmptyTechnique;
}

///---

static std::atomic<uint32_t> GMaterialTechniqueID = 1;

MaterialTechnique::MaterialTechnique(const MaterialCompilationSetup& setup)
    : m_setup(setup)
{
	m_id = GMaterialTechniqueID++;
}

MaterialTechnique::~MaterialTechnique()
{
    if (auto* expiredState = m_data.exchange(nullptr))
        m_expiredData.pushBack(NoAddRef(expiredState));

    m_expiredData.clear();
}

void MaterialTechnique::pushData(const gpu::ShaderData* newState)
{
    if (newState && newState->deviceShader())
    {
        if (auto pso = newState->deviceShader()->createGraphicsPipeline())
        {
            pso->addRef();

            if (auto* expiredState = m_data.exchange(pso.get()))
                m_expiredData.pushBack(NoAddRef(expiredState));
        }
    }
}

///---

END_BOOMER_NAMESPACE()
