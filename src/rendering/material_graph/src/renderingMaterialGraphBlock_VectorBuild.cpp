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

class MaterialGraphBlock_VectorBuild : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_VectorBuild, MaterialGraphBlock);

public:
    MaterialGraphBlock_VectorBuild()
    {}

    virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        if (m_components >= 1)
        {
            builder.socket("X"_id, MaterialInputSocket());
            if (m_components >= 2)
            {
                builder.socket("Y"_id, MaterialInputSocket());
                if (m_components >= 3)
                {
                    builder.socket("Z"_id, MaterialInputSocket());
                    if (m_components >= 4)
                    {
                        builder.socket("W"_id, MaterialInputSocket());
                    }
                }
            }
        }
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override
    {
        CodeChunk x = compiler.evalInput(this, "X"_id, 0.0f).conform(1);
        if (m_components == 1) return x;

        CodeChunk y = compiler.evalInput(this, "Y"_id, 0.0f).conform(1);
        if (m_components == 2) return CodeChunkOp::Float2(x, y);

        CodeChunk z = compiler.evalInput(this, "Z"_id, 0.0f).conform(1);
        if (m_components == 3) return CodeChunkOp::Float3(x, y, z);

        CodeChunk w = compiler.evalInput(this, "W"_id, 0.0f).conform(1);
        if (m_components == 4) CodeChunkOp::Float4(x, y, z, w);

        return 0.0f;
    }

    virtual void onPropertyChanged(base::StringView path) override
    {
        TBaseClass::onPropertyChanged(path);
        rebuildLayout();
    }

public:
    uint8_t m_components = 4;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_VectorBuild);
    RTTI_METADATA(base::graph::BlockInfoMetadata).title("Build").group("Vector");
    RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialVector");
    RTTI_METADATA(base::graph::BlockHelpMetadata).help("Builds vector from separate components[br] [br][b]X[/b] - first input component[br][b]Y[/b] - second input component[br][b]Z[/b] - third input component[br][b]W[/b] - fourth input component");
    RTTI_METADATA(base::graph::BlockShapeMetadata).slanted();
    RTTI_PROPERTY(m_components).editable("Number of output vector components");
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE(rendering)
