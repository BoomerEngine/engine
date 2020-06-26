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

    ///---

    static MaterialCompiledTechnique theEmptyTechnique;

    const MaterialCompiledTechnique& MaterialCompiledTechnique::EMPTY()
    {
        return theEmptyTechnique;
    }

    ///---

    MaterialTechnique::MaterialTechnique(const MaterialCompilationSetup& setup)
        : m_setup(setup)
    {}

    MaterialTechnique::~MaterialTechnique()
    {
        if (auto* expiredState = m_state.exchange(nullptr))
            m_expiredStates.pushBack(expiredState);

        m_expiredStates.clearPtr();
    }

    void MaterialTechnique::pushData(MaterialCompiledTechnique* newState)
    {
        if (newState)
        {
            if (auto* expiredState = m_state.exchange(newState))
                m_expiredStates.pushBack(expiredState);
        }
    }

    ///---

} // rendering