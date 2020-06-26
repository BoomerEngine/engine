/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks #]
***/

#pragma once

#include "renderingMaterialGraphBlock.h"

namespace rendering
{
    //--

    /// an output block for the material graph
    class RENDERING_MATERIAL_GRAPH_API MaterialGraphBlockOutput : public MaterialGraphBlock
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlockOutput, MaterialGraphBlock);

    public:
        MaterialGraphBlockOutput();

        // determine sort group for this material - static, can't depend on the particular technique
        virtual MaterialSortGroup resolveSortGroup() const;

        // compile the pixel shader side of the material block
        virtual void compilePixelFunction(MaterialStageCompiler& compiler, MaterialTechniqueRenderStates& outRenderState) const = 0;

        // compile the vertex shader side of this material block
        virtual void compileVertexFunction(MaterialStageCompiler& compiler, MaterialTechniqueRenderStates& outRenderState) const;

    private:
        virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override;
    };

    //--
    
} // rendering