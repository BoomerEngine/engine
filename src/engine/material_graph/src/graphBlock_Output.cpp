/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks #]
***/

#include "build.h"
#include "graph.h"
#include "graphBlock_Output.h"

BEGIN_BOOMER_NAMESPACE()

///---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(MaterialGraphBlockOutput);
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialOutput");
RTTI_END_TYPE();

MaterialGraphBlockOutput::MaterialGraphBlockOutput()
{}

void MaterialGraphBlockOutput::evalRenderStates(const IMaterial& params, MaterialRenderState& outMetadata) const
{
    // nothing specific
}

CodeChunk MaterialGraphBlockOutput::compile(MaterialStageCompiler& compiler, StringID outputName) const
{
    DEBUG_CHECK(!"Should not be called");
    return CodeChunk();
}

void MaterialGraphBlockOutput::compileVertexFunction(MaterialStageCompiler& compiler, MaterialTechniqueRenderStates& outRenderState) const
{
    // nothing here by default
}

///---

END_BOOMER_NAMESPACE()
