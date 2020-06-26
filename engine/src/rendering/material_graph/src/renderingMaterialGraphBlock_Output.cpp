/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks #]
***/

#include "build.h"
#include "renderingMaterialGraph.h"
#include "renderingMaterialGraphBlock_Output.h"

namespace rendering
{
    ///---

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(MaterialGraphBlockOutput);
        RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialOutput");
    RTTI_END_TYPE();

    MaterialGraphBlockOutput::MaterialGraphBlockOutput()
    {}

    MaterialSortGroup MaterialGraphBlockOutput::resolveSortGroup() const
    {
        return MaterialSortGroup::Opaque;
    }

    CodeChunk MaterialGraphBlockOutput::compile(MaterialStageCompiler& compiler, base::StringID outputName) const
    {
        DEBUG_CHECK(!"Should not be called");
        return CodeChunk();
    }

    void MaterialGraphBlockOutput::compileVertexFunction(MaterialStageCompiler& compiler, MaterialTechniqueRenderStates& outRenderState) const
    {
        // nothing here by default
    }

    ///---

} // rendering
