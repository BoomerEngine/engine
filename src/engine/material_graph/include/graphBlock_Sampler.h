/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\samplers #]
***/

#include "build.h"
#include "graphBlock_Parameter.h"

BEGIN_BOOMER_NAMESPACE()

using namespace CodeChunkOp;

//--

/// generic sampler block
class ENGINE_MATERIAL_GRAPH_API IMaterialGraphBlockSamplerParameter : public IMaterialGraphBlockBoundParameter
{
    RTTI_DECLARE_VIRTUAL_CLASS(IMaterialGraphBlockSamplerParameter, IMaterialGraphBlockBoundParameter);

public:
    IMaterialGraphBlockSamplerParameter();

protected:
    char m_texCoordIndex = 0;

    void buildDefaultSockets(graph::BlockLayoutBuilder& builder) const;

    CodeChunk computeDefaultUV(MaterialStageCompiler& compiler) const;
    CodeChunk compileTextureRef(MaterialStageCompiler& compiler) const;
};

///--

END_BOOMER_NAMESPACE()
