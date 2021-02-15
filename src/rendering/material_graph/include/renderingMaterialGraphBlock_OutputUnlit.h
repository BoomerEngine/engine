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

    /// unlit output (color is directly written to render target)
    /// NOTE: this block may contain internal lighting calculations
    class RENDERING_MATERIAL_GRAPH_API MaterialGraphBlockOutput_Unlit : public MaterialGraphBlockOutputCommon
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlockOutput_Unlit, MaterialGraphBlockOutputCommon);

    public:
        MaterialGraphBlockOutput_Unlit();

    private:
        virtual void MaterialGraphBlockOutput_Unlit::buildLayout(base::graph::BlockLayoutBuilder& builder) const override;
        virtual CodeChunk compileMainColor(MaterialStageCompiler& compiler, MaterialTechniqueRenderStates& outRenderState) const override;
    };

    //--
    
} // rendering