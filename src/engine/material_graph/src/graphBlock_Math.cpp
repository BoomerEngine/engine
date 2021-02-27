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

class MaterialGraphBlock_MathAbs : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathAbs, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "X"_id, 0.0f);
        return a.abs();
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathAbs);
    RTTI_METADATA(graph::BlockInfoMetadata).title("abs(x)").group("Math");
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
    RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathAdd : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathAdd, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("A"_id, MaterialInputSocket().hideCaption());
        builder.socket("B"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "A"_id, 0.0f);
        CodeChunk b = compiler.evalInput(this, "B"_id, 0.0f);

        const auto maxComponents = std::max<uint8_t>(a.components(), b.components());
        a = a.conform(maxComponents);
        b = b.conform(maxComponents);

        return a + b;
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathAdd);
RTTI_METADATA(graph::BlockInfoMetadata).title("a + b").group("Math").name("Add");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathArcCos : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathArcCos, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "X"_id, 0.0f);
        return CodeChunkOp::ArcCos(a);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathArcCos);
RTTI_METADATA(graph::BlockInfoMetadata).title("acos(x)").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathArcSin : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathArcSin, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "X"_id, 0.0f);
        return CodeChunkOp::ArcSin(a);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathArcSin);
RTTI_METADATA(graph::BlockInfoMetadata).title("asin(x)").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathArcTan : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathArcTan, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "X"_id, 0.0f);
        return CodeChunkOp::ArcTan(a);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathArcTan);
RTTI_METADATA(graph::BlockInfoMetadata).title("atan(x)").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathArcTan2 : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathArcTan2, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket());
        builder.socket("Y"_id, MaterialInputSocket());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk x = compiler.evalInput(this, "X"_id, 0.0f).x();
        CodeChunk y = compiler.evalInput(this, "Y"_id, 0.0f).x();
        return CodeChunkOp::ArcTan2(y,x);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathArcTan2);
RTTI_METADATA(graph::BlockInfoMetadata).title("atan2(y,x)").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathCeil : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathCeil, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "X"_id, 0.0f);
        return CodeChunkOp::Floor(a);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathCeil);
RTTI_METADATA(graph::BlockInfoMetadata).title("ceil(x)").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathClamp : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathClamp, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("In"_id, MaterialInputSocket().hideCaption());
        builder.socket("Min"_id, MaterialInputSocket().hiddenByDefault());
        builder.socket("Max"_id, MaterialInputSocket().hiddenByDefault());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "In"_id, 0.0f);
        CodeChunk minV = compiler.evalInput(this, "Min"_id, m_defaultMin);
        CodeChunk maxV = compiler.evalInput(this, "Max"_id, m_defaultMax);

        minV = minV.conform(a.components());
        maxV = maxV.conform(a.components());

        return CodeChunkOp::Clamp(a, minV, maxV);
    }

    float m_defaultMin = 0.0f;
    float m_defaultMax = 1.0f;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathClamp);
RTTI_METADATA(graph::BlockInfoMetadata).title("clamp(x)").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_PROPERTY(m_defaultMin).editable("Minimum clamp value (in case custom value is not provided)");
RTTI_PROPERTY(m_defaultMax).editable("Maximum clamp value (in case custom value is not provided)");
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathCos : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathCos, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "X"_id, 0.0f);
        return CodeChunkOp::Cos(a);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathCos);
RTTI_METADATA(graph::BlockInfoMetadata).title("cos(x)").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathDerivative : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathDerivative, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("dF/dx"_id, MaterialOutputSocket());
        builder.socket("dF/dy"_id, MaterialOutputSocket());
        builder.socket("F"_id, MaterialInputSocket());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk f = compiler.evalInput(this, "F"_id, 0.0f);
        if (outputName == "dF/dx")
            return f.ddx();
        else if (outputName == "dF/dy")
            return f.ddy();
        else
            return 0.0f;
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathDerivative);
RTTI_METADATA(graph::BlockInfoMetadata).title("dF/d").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathDiv : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathDiv, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("A"_id, MaterialInputSocket());
        builder.socket("B"_id, MaterialInputSocket());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "A"_id, 0.0f);
        CodeChunk b = compiler.evalInput(this, "B"_id, 0.0f);

        const auto maxComponents = std::max<uint8_t>(a.components(), b.components());
        a = a.conform(maxComponents);
        b = b.conform(maxComponents);

        return a / b;
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathDiv);
RTTI_METADATA(graph::BlockInfoMetadata).title("a / b").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathExp : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathExp, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "X"_id, 0.0f);
        return a.exp();
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathExp);
RTTI_METADATA(graph::BlockInfoMetadata).title("exp(x)").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathExp2 : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathExp2, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "X"_id, 0.0f);
        return a.exp2();
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathExp2);
RTTI_METADATA(graph::BlockInfoMetadata).title("exp2(x)").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathFloor : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathFloor, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "X"_id, 0.0f);
        return CodeChunkOp::Floor(a);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathFloor);
RTTI_METADATA(graph::BlockInfoMetadata).title("floor(x)").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathFrac : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathFrac, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "X"_id, 0.0f);
        return CodeChunkOp::Fract(a);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathFrac);
RTTI_METADATA(graph::BlockInfoMetadata).title("frac(x)").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathLerp : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathLerp, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("A"_id, MaterialInputSocket());
        builder.socket("B"_id, MaterialInputSocket());
        builder.socket("F"_id, MaterialInputSocket());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "A"_id, 0.0f);
        CodeChunk b = compiler.evalInput(this, "B"_id, 1.0f);
        CodeChunk f = compiler.evalInput(this, "F"_id, 0.5f);

        const auto maxComponents = std::max<uint8_t>(a.components(), b.components());
        a = a.conform(maxComponents);
        b = b.conform(maxComponents);

        return CodeChunkOp::Lerp(a, b, f.x());
    }

private:
    float m_value;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathLerp);
RTTI_METADATA(graph::BlockInfoMetadata).title("lerp(a,b,f)").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathLog : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathLog, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "X"_id, 0.0f);
        return a.log();
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathLog);
RTTI_METADATA(graph::BlockInfoMetadata).title("log(x)").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathLog2 : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathLog2, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "X"_id, 0.0f);
        return a.log2();
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathLog2);
RTTI_METADATA(graph::BlockInfoMetadata).title("log2(x)").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathMap : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathMap, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("In"_id, MaterialInputSocket());
        builder.socket("Min"_id, MaterialInputSocket().hiddenByDefault());
        builder.socket("Max"_id, MaterialInputSocket().hiddenByDefault());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "In"_id, 0.0f);
        CodeChunk minV = compiler.evalInput(this, "Min"_id, m_defaultMin);
        CodeChunk maxV = compiler.evalInput(this, "Max"_id, m_defaultMax);

        minV = minV.conform(a.components());
        maxV = maxV.conform(a.components());

        return ((a - minV) / (maxV - minV)).saturate();
    }

    float m_defaultMin = 0.0f;
    float m_defaultMax = 1.0f;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathMap);
RTTI_METADATA(graph::BlockInfoMetadata).title("map(x)").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_PROPERTY(m_defaultMin).editable("Minimum range value (in case custom value is not provided)");
RTTI_PROPERTY(m_defaultMax).editable("Maximum range value (in case custom value is not provided)");
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathMax : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathMax, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("A"_id, MaterialInputSocket().hideCaption());
        builder.socket("B"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "A"_id, 0.0f);
        CodeChunk b = compiler.evalInput(this, "B"_id, 0.0f);

        const auto maxComponents = std::max<uint8_t>(a.components(), b.components());
        a = a.conform(maxComponents);
        b = b.conform(maxComponents);

        return CodeChunkOp::Max(a, b);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathMax);
RTTI_METADATA(graph::BlockInfoMetadata).title("max(a,b)").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathMin : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathMin, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("A"_id, MaterialInputSocket().hideCaption());
        builder.socket("B"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "A"_id, 0.0f);
        CodeChunk b = compiler.evalInput(this, "B"_id, 0.0f);

        const auto maxComponents = std::max<uint8_t>(a.components(), b.components());
        a = a.conform(maxComponents);
        b = b.conform(maxComponents);

        return CodeChunkOp::Min(a, b);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathMin);
RTTI_METADATA(graph::BlockInfoMetadata).title("min(a,b)").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathMul : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathMul, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("A"_id, MaterialInputSocket().hideCaption());
        builder.socket("B"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "A"_id, m_defaultA);
        CodeChunk b = compiler.evalInput(this, "B"_id, m_defaultB);

        const auto maxComponents = std::max<uint8_t>(a.components(), b.components());
        a = a.conform(maxComponents);
        b = b.conform(maxComponents);

        return a * b;
    }

    float m_defaultA = 1.0f;
    float m_defaultB = 1.0f;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathMul);
RTTI_METADATA(graph::BlockInfoMetadata).title("a * b").group("Math").name("Multiply");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_PROPERTY(m_defaultA).editable();
RTTI_PROPERTY(m_defaultB).editable();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathNegate : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathNegate, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "X"_id, 0.0f);
        return -a;
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathNegate);
RTTI_METADATA(graph::BlockInfoMetadata).title("-x").group("Math").name("Negate");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathOneMinus : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathOneMinus, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "X"_id, 0.0f);
        CodeChunk one = CodeChunk(1.0f).conform(a.components());
        return one - a;
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathOneMinus);
RTTI_METADATA(graph::BlockInfoMetadata).title("1 - x").group("Math").name("OneMinus");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathOneOver : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathOneOver, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "X"_id, 0.0f);
        CodeChunk one = CodeChunk(1.0f).conform(a.components());
        return one / a;
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathOneOver);
RTTI_METADATA(graph::BlockInfoMetadata).title("1 / x").group("Math").name("Inverse");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathPower : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathPower, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket());
        builder.socket("E"_id, MaterialInputSocket());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk x = compiler.evalInput(this, "X"_id, 0.0f);
        CodeChunk e = compiler.evalInput(this, "E"_id, 1.0f);
        e = e.conform(x.components());
        return x.pow(e);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathPower);
RTTI_METADATA(graph::BlockInfoMetadata).title("pow(x,e)").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathRound : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathRound, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "X"_id, 0.0f);
        return CodeChunkOp::Round(a);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathRound);
RTTI_METADATA(graph::BlockInfoMetadata).title("round(x)").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathSign : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathSign, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "X"_id, 0.0f);
        return CodeChunkOp::Round(a);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathSign);
RTTI_METADATA(graph::BlockInfoMetadata).title("sign(x)").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathSin : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathSin, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "X"_id, 0.0f);
        return CodeChunkOp::Cos(a);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathSin);
RTTI_METADATA(graph::BlockInfoMetadata).title("sin(x)").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathSqrt : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathSqrt, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "X"_id, 0.0f);
        return a.sqrt();
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathSqrt);
RTTI_METADATA(graph::BlockInfoMetadata).title("sqrt(x)").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathStep : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathStep, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "X"_id, 0.0f);
        return CodeChunkOp::Step(a);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathStep);
RTTI_METADATA(graph::BlockInfoMetadata).title("step(x)").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathSubtract : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathSubtract, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("A"_id, MaterialInputSocket());
        builder.socket("B"_id, MaterialInputSocket());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "A"_id, 0.0f);
        CodeChunk b = compiler.evalInput(this, "B"_id, 0.0f);

        const auto maxComponents = std::max<uint8_t>(a.components(), b.components());
        a = a.conform(maxComponents);
        b = b.conform(maxComponents);

        return a - b;
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathSubtract);
RTTI_METADATA(graph::BlockInfoMetadata).title("a - b").group("Math").name("Subtract");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathTan : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathTan, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "X"_id, 0.0f);
        return CodeChunkOp::Tan(a);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathTan);
RTTI_METADATA(graph::BlockInfoMetadata).title("tan(x)").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_MathTrunc : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathTrunc, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "X"_id, 0.0f);
        return CodeChunkOp::Trunc(a);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathTrunc);
RTTI_METADATA(graph::BlockInfoMetadata).title("trunc(x)").group("Math");
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE()
