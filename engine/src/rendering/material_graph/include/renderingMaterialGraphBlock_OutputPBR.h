/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material\output #]
***/

#pragma once

#include "renderingMaterialGraphBlock_OutputCommon.h"

namespace rendering
{
    //--

    /// lit output using PBR shader
    class RENDERING_MATERIAL_GRAPH_API MaterialGraphBlockOutput_PBR : public MaterialGraphBlockOutputCommon
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlockOutput_PBR, MaterialGraphBlockOutputCommon);

    public:
        MaterialGraphBlockOutput_PBR();

    private:
        virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override;
        virtual CodeChunk compileMainColor(MaterialStageCompiler& compiler, MaterialTechniqueRenderStates& outRenderState) const override;

        bool m_twoSidedLighting = false;
        
        bool m_applyGlobalAmbient = true;
        bool m_applyGlobalDirectionalLighting = true;
        bool m_applyAmbientOcclusion = true;
        bool m_applyReflectionProbes = true;
        bool m_applyLocalLighting = true;

        bool m_receiveGlobalShadows = true;
        bool m_receiveTerrainShadows = true;
        bool m_receiveLocalShadows = true;
    };

    //--
    
} // rendering