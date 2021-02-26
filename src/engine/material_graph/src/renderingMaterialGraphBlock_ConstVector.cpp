/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\constants #]
***/

#include "build.h"
#include "renderingMaterialGraphBlock.h"

BEGIN_BOOMER_NAMESPACE()

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
    RTTI_METADATA(graph::BlockInfoMetadata).name("Constant Vector2").group("Constants").title("Vector2");
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialConst");
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
    RTTI_METADATA(graph::BlockInfoMetadata).name("Constant Vector3").group("Constants").title("Vector3");
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialConst");
    RTTI_METADATA(graph::BlockShapeMetadata).rectangle();
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
    RTTI_METADATA(graph::BlockInfoMetadata).name("Constant Vector4").group("Constants").title("Vector4");
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialConst");
    RTTI_PROPERTY(m_value).editable("Constant vector's value");
RTTI_END_TYPE();

END_BOOMER_NAMESPACE()
