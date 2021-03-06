/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material\output #]
***/

#pragma once

#include "graphBlock_Output.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// lit output using PBR shader
class ENGINE_MATERIAL_GRAPH_API MaterialGraphBlockOutputCommon : public MaterialGraphBlockOutput
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlockOutputCommon, MaterialGraphBlockOutput);

public:
    MaterialGraphBlockOutputCommon();

protected:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override;

    virtual void evalRenderStates(const IMaterial& params, MaterialRenderState& outMetadata) const override final;
    virtual void compilePixelFunction(MaterialStageCompiler& compiler, MaterialTechniqueRenderStates& outRenderState) const override final;
    virtual void compileVertexFunction(MaterialStageCompiler& compiler, MaterialTechniqueRenderStates& outRenderState) const override final;

    virtual CodeChunk compileMainColor(MaterialStageCompiler& compiler, MaterialTechniqueRenderStates& outRenderState) const = 0;

    static inline const auto PARAM_USE_TRANSPARENCY = "UseTransparency"_id;
    static inline const auto PARAM_USE_TWOSIDED = "UseTwoSidedRendering"_id;
    static inline const auto PARAM_USE_TWOSIDED_LIGHTING = "UseTwoSidedLighting"_id;
    static inline const auto PARAM_USE_MASK = "UseMasking"_id;

    bool evalTwoSidedFlag(const MaterialStageCompiler& compiler) const;
    bool evalTwoSidedLightingFlag(const MaterialStageCompiler& compiler) const;
    bool evalTransparencyFlag(const MaterialStageCompiler& compiler) const;
    bool evalMaskingFlag(const MaterialStageCompiler& compiler) const;

    bool m_applyFog = true;

    bool m_twoSided = false;
    bool m_twoSidedLighting = false;

    bool m_maskAlphaToCoverage = false;
    bool m_depthWrite = true; // can be set to false even for "solid" materials
    bool m_premultiplyAlpha = true;
    float m_maskThreshold = 0.5f;
    bool m_transparent = false;
};

//--
    
END_BOOMER_NAMESPACE()
