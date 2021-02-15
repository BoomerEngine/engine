/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material\output #]
***/

#include "build.h"
#include "renderingMaterialGraph.h"
#include "renderingMaterialGraphBlock_OutputUnlit.h"
#include "renderingMaterialCode.h"

#include "rendering/material/include/renderingMaterialRuntimeTechnique.h"

namespace rendering
{
    ///---

    RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlockOutput_Unlit);
        RTTI_CATEGORY("Output");
        RTTI_METADATA(base::graph::BlockInfoMetadata).title("UnlitMaterial").group("Material outputs");
    RTTI_END_TYPE();

    MaterialGraphBlockOutput_Unlit::MaterialGraphBlockOutput_Unlit()
    {}

    void MaterialGraphBlockOutput_Unlit::buildLayout(base::graph::BlockLayoutBuilder& builder) const
    {
        builder.socket("Color"_id, MaterialInputSocket());
        builder.socket("Opacity"_id, MaterialInputSocket());
        //builder.addSocket("RefractionDelta"_id, MaterialInputSocket());
        //builder.addSocket("ReflectionOffset"_id, MaterialInputSocket());
        //builder.addSocket("ReflectionAmount"_id, MaterialInputSocket());
        //builder.addSocket("GlobalReflection"_id, MaterialInputSocket());
        builder.socket("Mask"_id, MaterialInputSocket());
    }

    CodeChunk MaterialGraphBlockOutput_Unlit::compileMainColor(MaterialStageCompiler& compiler, MaterialTechniqueRenderStates& outRenderState) const
    {
        const auto defaultColor = base::Vector3(0.5f, 0.5f, 0.5f);
        return compiler.evalInput(this, "Color"_id, defaultColor).conform(3);
    }

    ///---

} // rendering
