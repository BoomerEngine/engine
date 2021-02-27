/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\math #]
***/

#include "build.h"
#include "graphBlock.h"

BEGIN_BOOMER_NAMESPACE()

///---

class MaterialGraphBlock_VectorBuild : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_VectorBuild, MaterialGraphBlock);

public:
    MaterialGraphBlock_VectorBuild()
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
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

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
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

    virtual void onPropertyChanged(StringView path) override
    {
        TBaseClass::onPropertyChanged(path);
        rebuildLayout();
    }

public:
    uint8_t m_components = 4;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_VectorBuild);
    RTTI_METADATA(graph::BlockInfoMetadata).title("Build").group("Vector");
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialVector");
    RTTI_METADATA(graph::BlockHelpMetadata).help("Builds vector from separate components[br] [br][b]X[/b] - first input component[br][b]Y[/b] - second input component[br][b]Z[/b] - third input component[br][b]W[/b] - fourth input component");
    RTTI_METADATA(graph::BlockShapeMetadata).slanted();
    RTTI_PROPERTY(m_components).editable("Number of output vector components");
RTTI_END_TYPE();

///---

class MaterialGraphBlock_VectorCross : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_VectorCross, MaterialGraphBlock);

public:
    MaterialGraphBlock_VectorCross()
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("A"_id, MaterialInputSocket().hideCaption());
        builder.socket("B"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "A"_id, Vector3(0, 0, 1)).conform(3);
        CodeChunk b = compiler.evalInput(this, "B"_id, Vector3(0, 0, 1)).conform(3);
        return CodeChunkOp::Cross(a, b);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_VectorCross);
RTTI_METADATA(graph::BlockInfoMetadata).title("cross(a,b)").group("Vector").name("Cross");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialVector");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_VectorDot2 : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_VectorDot2, MaterialGraphBlock);

public:
    MaterialGraphBlock_VectorDot2()
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("A"_id, MaterialInputSocket().hideCaption());
        builder.socket("B"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "A"_id, Vector2(1, 1)).conform(2);
        CodeChunk b = compiler.evalInput(this, "B"_id, Vector2(1, 1)).conform(2);
        return CodeChunkOp::Dot2(a, b);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_VectorDot2);
RTTI_METADATA(graph::BlockInfoMetadata).title("dot2(a,b)").group("Vector").name("Dot2");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialVector");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();

RTTI_END_TYPE();

///---

class MaterialGraphBlock_VectorDot3 : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_VectorDot3, MaterialGraphBlock);

public:
    MaterialGraphBlock_VectorDot3()
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("A"_id, MaterialInputSocket().hideCaption());
        builder.socket("B"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "A"_id, Vector3(1, 1, 1)).conform(3);
        CodeChunk b = compiler.evalInput(this, "B"_id, Vector3(1, 1, 1)).conform(3);
        return CodeChunkOp::Dot3(a, b);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_VectorDot3);
RTTI_METADATA(graph::BlockInfoMetadata).title("dot3(a,b)").group("Vector").name("Dot3");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialVector");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_VectorDot4 : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_VectorDot4, MaterialGraphBlock);

public:
    MaterialGraphBlock_VectorDot4()
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("A"_id, MaterialInputSocket().hideCaption());
        builder.socket("B"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "A"_id, Vector4(1, 1, 1, 1)).conform(4);
        CodeChunk b = compiler.evalInput(this, "B"_id, Vector4(1, 1, 1, 1)).conform(4);
        return CodeChunkOp::Dot4(a, b);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_VectorDot4);
RTTI_METADATA(graph::BlockInfoMetadata).title("dot4").group("Vector").name("Dot4");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialVector");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_VectorNormalize : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_VectorNormalize, MaterialGraphBlock);

public:
    MaterialGraphBlock_VectorNormalize()
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "In"_id, Vector3(0, 0, 1));
        if (a.components() == 2 || a.components() == 3 || a.components() == 4)
            return CodeChunkOp::Normalize(a);
        else
            return 1.0f;
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_VectorNormalize);
RTTI_METADATA(graph::BlockInfoMetadata).title("normalize(x)").group("Vector").name("Normalize");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialVector");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE()
