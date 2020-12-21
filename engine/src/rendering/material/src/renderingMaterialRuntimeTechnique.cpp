/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#include "build.h"
#include "renderingMaterialRuntimeTechnique.h"

namespace rendering
{
 
    ///---

    RTTI_BEGIN_TYPE_ENUM(MaterialBlendMode);
    RTTI_ENUM_OPTION(Opaque);
    RTTI_ENUM_OPTION(AlphaBlend);
    RTTI_ENUM_OPTION(Addtive);
    RTTI_ENUM_OPTION(Refractive);
    RTTI_END_TYPE();

    ///---

    RTTI_BEGIN_TYPE_CLASS(MaterialTechniqueRenderStates);
    RTTI_PROPERTY(blendMode);
    RTTI_PROPERTY(twoSided);
    RTTI_PROPERTY(depthTest);
    RTTI_PROPERTY(depthWrite);
    RTTI_END_TYPE();

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
            m_expiredData.pushBack(expiredState);

        m_expiredData.clearPtr();
    }

    void MaterialTechnique::pushData(MaterialCompiledTechnique* newState)
    {
        if (newState)
        {
            if (auto* expiredState = m_data.exchange(newState))
				m_expiredData.pushBack(expiredState);
        }
    }

    ///---

} // rendering