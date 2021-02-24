/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\math #]
***/

#include "build.h"
#include "renderingMaterialGraphBlock.h"

BEGIN_BOOMER_NAMESPACE(rendering)

///---

class MaterialGraphBlock_VectorReflect : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_VectorReflect, MaterialGraphBlock);

public:
    MaterialGraphBlock_VectorReflect()
    {}

    virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket());
        builder.socket("N"_id, MaterialInputSocket());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override
    {
        CodeChunk n;
        if (hasConnectionOnSocket("Normal"_id))
        {
            n = compiler.evalInput(this, "Normal"_id, base::Vector3(0, 0, 1)).conform(3);
        }
        else
        {
            n = compiler.vertexData(MaterialVertexDataType::WorldNormal);
        }

        CodeChunk a;
        if (hasConnectionOnSocket("X"_id))
        {
            a = compiler.evalInput(this, "X"_id, base::Vector3(0, 0, 0)).conform(3);
        }
        else
        {
            const auto worldPos = compiler.vertexData(MaterialVertexDataType::WorldPosition);
            const auto cameraPos = CodeChunk(CodeChunkType::Numerical3, "CameraParams.CameraPosition");
            a = cameraPos - worldPos;
        }

        return CodeChunkOp::Reflect(a, n);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_VectorReflect);
    RTTI_METADATA(base::graph::BlockInfoMetadata).title("reflect(x,n)").group("Vector").name("Reflect");
    RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialVector");
    RTTI_METADATA(base::graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE(rendering)
