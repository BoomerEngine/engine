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

using namespace CodeChunkOp;

///---

class MaterialGraphBlock_DerivativeToNormal : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_DerivativeToNormal, MaterialGraphBlock);

public:
    MaterialGraphBlock_DerivativeToNormal()
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Normal"_id, MaterialOutputSocket());
        builder.socket("DXDY"_id, MaterialInputSocket());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk dxdy = compiler.evalInput(this, "DXDY"_id, Vector2(0,0)).conform(2);
        CodeChunk invZSquared = (dxdy.x() * dxdy.x()) + (dxdy.y() * dxdy.y()) + 1.0f;
        CodeChunk z = 1.0f / invZSquared.sqrt();
        return Float3(dxdy.x() * z, dxdy.y() * z, z);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_DerivativeToNormal);
    RTTI_METADATA(graph::BlockInfoMetadata).title("dx/dy to normal").group("Functions");
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialGeneric");
RTTI_END_TYPE();

///---

///---

class MaterialGraphBlock_Desaturate : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_Desaturate, MaterialGraphBlock);

public:
    MaterialGraphBlock_Desaturate()
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket());
        builder.socket("In"_id, MaterialInputSocket());
        builder.socket("Amount"_id, MaterialInputSocket());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk color = compiler.evalInput(this, "In"_id, Vector3(0,0,0)).conform(3);
        CodeChunk lum = Dot3(color, Vector3(0.299f, 0.587f, 0.114f)); // color perceptual luminance weights

        if (hasConnectionOnSocket("Amount"_id))
        {
            CodeChunk amount = compiler.evalInput(this, "Amount"_id, 0.5f);
            return Lerp(color, lum.xxx(), amount);
        }
        else
        {
            return lum.xxx();
        }
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_Desaturate);
RTTI_METADATA(graph::BlockInfoMetadata).title("Desaturate").group("Functions");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialGeneric");
RTTI_END_TYPE();

///---

class MaterialGraphBlock_Dither : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_Dither, MaterialGraphBlock);

public:
    MaterialGraphBlock_Dither()
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket());
        builder.socket("In"_id, MaterialInputSocket());
        builder.socket("Amount"_id, MaterialInputSocket());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk color = compiler.evalInput(this, "In"_id, Vector3(0,0,0)).conform(3);
        // TODO
        return color;
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_Dither);
RTTI_METADATA(graph::BlockInfoMetadata).title("Dither").group("Functions");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialGeneric");
RTTI_END_TYPE();

///---

class MaterialGraphBlock_NormalToDerivative : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_NormalToDerivative, MaterialGraphBlock);

public:
    MaterialGraphBlock_NormalToDerivative()
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("DXDY"_id, MaterialOutputSocket());
        builder.socket("Normal"_id, MaterialInputSocket());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk normal = compiler.evalInput(this, "Normal"_id, Vector3(0,0,1)).conform(3);
        return Float2(normal.x() / normal.z(), normal.y() / normal.z());
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_NormalToDerivative);
RTTI_METADATA(graph::BlockInfoMetadata).title("Normal to dx/dy").group("Functions");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialGeneric");
RTTI_END_TYPE();

///---

class MaterialGraphBlock_SmoothStep : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_SmoothStep, MaterialGraphBlock);

public:
    MaterialGraphBlock_SmoothStep()
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket());
        builder.socket("X"_id, MaterialInputSocket());
        builder.socket("Start"_id, MaterialInputSocket());
        builder.socket("End"_id, MaterialInputSocket());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "X"_id, 0.5f);
        CodeChunk start = compiler.evalInput(this, "Start"_id, 0.0f);
        CodeChunk end = compiler.evalInput(this, "End"_id, 1.0f);

        const auto comps = a.components();
        start = start.conform(comps);
        end = end.conform(comps);

        return CodeChunkOp::SmoothStep(start, end, a);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_SmoothStep);
RTTI_METADATA(graph::BlockInfoMetadata).title("SmoothStep").group("Functions");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialGeneric");
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE()
