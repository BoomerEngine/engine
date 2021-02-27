/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\constants #]
***/

#include "build.h"
#include "graphBlock.h"

BEGIN_BOOMER_NAMESPACE()

#define BLOCK_COMMON(x) \
    RTTI_METADATA(graph::BlockInfoMetadata).name(x).group("Constants"); \
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialConst"); \
    RTTI_METADATA(graph::BlockShapeMetadata).rectangle();

///---

class MaterialGraphBlock_ConstColor : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_ConstColor, MaterialGraphBlock);

public:
    MaterialGraphBlock_ConstColor()
        : m_color(255,255,255,255)
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("RGB"_id, MaterialOutputSocket().swizzle("xyz"_id));
        builder.socket("RGBA"_id, MaterialOutputSocket().hiddenByDefault());
        builder.socket("R"_id, MaterialOutputSocket().swizzle("x"_id).hiddenByDefault().color(COLOR_SOCKET_RED));
        builder.socket("G"_id, MaterialOutputSocket().swizzle("y"_id).hiddenByDefault().color(COLOR_SOCKET_GREEN));
        builder.socket("B"_id, MaterialOutputSocket().swizzle("z"_id).hiddenByDefault().color(COLOR_SOCKET_BLUE));
        builder.socket("A"_id, MaterialOutputSocket().swizzle("w"_id).color(COLOR_SOCKET_ALPHA));
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        if (m_linearColor)
            return CodeChunk(m_color.toVectorLinear());
        else
            return CodeChunk(m_color.toVectorSRGB());
    }

private:
    Color m_color;
    bool m_linearColor = false;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_ConstColor);
    BLOCK_COMMON("Constant Color")
    RTTI_PROPERTY(m_color).editable("Constant color");
    RTTI_PROPERTY(m_linearColor).editable("Color is in linear space, no sRGB conversion is required");
RTTI_END_TYPE();

///---

///---

class MaterialGraphBlock_ConstFloat : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_ConstFloat, MaterialGraphBlock);

public:
    MaterialGraphBlock_ConstFloat()
            : m_value(0.0f)
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Value"_id, MaterialOutputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        return CodeChunk(m_value);
    }

private:
    float m_value;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_ConstFloat);
    BLOCK_COMMON("Constant Scalar")
    RTTI_PROPERTY(m_value).editable("Constant value");
RTTI_END_TYPE();

///---

///---

class MaterialGraphBlock_ConstVector2 : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_ConstVector2, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket());
        builder.socket("X"_id, MaterialOutputSocket().swizzle("x"_id).color(COLOR_SOCKET_ALPHA).hiddenByDefault());
        builder.socket("Y"_id, MaterialOutputSocket().swizzle("y"_id).color(COLOR_SOCKET_ALPHA).hiddenByDefault());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        return m_value;
    }

private:
    Vector2 m_value;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_ConstVector2);
BLOCK_COMMON("Constant Vector2")
RTTI_PROPERTY(m_value).editable("Constant vector's value");
RTTI_END_TYPE();

///---

class MaterialGraphBlock_ConstVector3 : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_ConstVector3, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialOutputSocket().swizzle("x"_id).color(COLOR_SOCKET_ALPHA).hiddenByDefault());
        builder.socket("Y"_id, MaterialOutputSocket().swizzle("y"_id).color(COLOR_SOCKET_ALPHA).hiddenByDefault());
        builder.socket("Z"_id, MaterialOutputSocket().swizzle("z"_id).color(COLOR_SOCKET_ALPHA).hiddenByDefault());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        return m_value;
    }

private:
    Vector3 m_value;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_ConstVector3);
BLOCK_COMMON("Constant Vector3")
RTTI_PROPERTY(m_value).editable("Constant vector's value");
RTTI_END_TYPE();

///---

class MaterialGraphBlock_ConstVector4 : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_ConstVector4, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket());
        builder.socket("X"_id, MaterialOutputSocket().swizzle("x"_id).color(COLOR_SOCKET_ALPHA).hiddenByDefault());
        builder.socket("Y"_id, MaterialOutputSocket().swizzle("y"_id).color(COLOR_SOCKET_ALPHA).hiddenByDefault());
        builder.socket("Z"_id, MaterialOutputSocket().swizzle("z"_id).color(COLOR_SOCKET_ALPHA).hiddenByDefault());
        builder.socket("W"_id, MaterialOutputSocket().swizzle("w"_id).color(COLOR_SOCKET_ALPHA).hiddenByDefault());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        return m_value;
    }

private:
    Vector4 m_value;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_ConstVector4);
BLOCK_COMMON("Constant Vector4")
RTTI_PROPERTY(m_value).editable("Constant vector's value");
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE()
