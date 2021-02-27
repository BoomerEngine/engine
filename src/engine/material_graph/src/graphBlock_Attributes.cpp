/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\attribute #]
***/

#include "build.h"
#include "graphBlock.h"

#define BLOCK_COMMON(x) \
    RTTI_METADATA(graph::BlockInfoMetadata).title(x).group("Attributes"); \
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialAttribute"); \
    RTTI_METADATA(graph::BlockShapeMetadata).rectangle();

BEGIN_BOOMER_NAMESPACE()

///---

class MaterialGraphBlock_AttributeFaceDirection : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_AttributeFaceDirection, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Side"_id, MaterialOutputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        if (compiler.stage() == gpu::ShaderStage::Pixel)
            return CodeChunk(CodeChunkType::Numerical1, TempString("(gl_FrontFacing ? {} : {})", m_frontValue, m_backValue));
        else
            return m_frontValue;
    }

    float m_frontValue = 1.0f;
    float m_backValue = 0.0f;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_AttributeFaceDirection);
    BLOCK_COMMON("FaceDirection")
    RTTI_PROPERTY(m_frontValue).editable("Value to output when front facing");
    RTTI_PROPERTY(m_backValue).editable("Value to output when back facing (only when two sided rendering is enabled)");
RTTI_END_TYPE();

///---

class MaterialGraphBlock_AttributeObjectColor : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_AttributeObjectColor, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("RGB"_id, MaterialOutputSocket().swizzle("xyz"_id));
        builder.socket("R"_id, MaterialOutputSocket().swizzle("x"_id).hiddenByDefault());
        builder.socket("G"_id, MaterialOutputSocket().swizzle("y"_id).hiddenByDefault());
        builder.socket("B"_id, MaterialOutputSocket().swizzle("z"_id).hiddenByDefault());
        builder.socket("A"_id, MaterialOutputSocket().swizzle("w"_id));
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        return Vector4(1,0,1,0);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_AttributeObjectColor);
    BLOCK_COMMON("ObjectColor")
RTTI_END_TYPE();

///---

class MaterialGraphBlock_AttributeObjectOrientation : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_AttributeObjectOrientation, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Forward"_id, MaterialOutputSocket());
        builder.socket("Up"_id, MaterialOutputSocket().hiddenByDefault());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        return Vector3(0, 0, 0);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_AttributeObjectOrientation);
    BLOCK_COMMON("ObjectOrientation")
RTTI_END_TYPE();

///---

class MaterialGraphBlock_AttributeObjectPosition : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_AttributeObjectPosition, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Position"_id, MaterialOutputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        return Vector3(0, 0, 0);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_AttributeObjectPosition);
    BLOCK_COMMON("ObjectPosition")
RTTI_END_TYPE();

///---

class MaterialGraphBlock_AttributePixelPosition : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_AttributePixelPosition, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Position"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialOutputSocket().swizzle("x"_id).hiddenByDefault());
        builder.socket("Y"_id, MaterialOutputSocket().swizzle("y"_id).hiddenByDefault());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        if (compiler.stage() == gpu::ShaderStage::Pixel)
            return CodeChunk(CodeChunkType::Numerical2, "(gl_FragCoord.xy)");
        else
            return Vector2(0.0f, 0.0f);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_AttributePixelPosition);
    BLOCK_COMMON("PixelPosition")
RTTI_END_TYPE();

///---

class MaterialGraphBlock_AttributeWorldBitangent : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_AttributeWorldBitangent, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Bitangent"_id, MaterialOutputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        return compiler.vertexData(MaterialVertexDataType::WorldBitangent);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_AttributeWorldBitangent);
    BLOCK_COMMON("WorldBitangent")
RTTI_END_TYPE();

///---

class MaterialGraphBlock_AttributeWorldNormal : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_AttributeWorldNormal, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Normal"_id, MaterialOutputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        return compiler.vertexData(MaterialVertexDataType::WorldNormal);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_AttributeWorldNormal);
BLOCK_COMMON("WorldNormal")
RTTI_END_TYPE();

///---

class MaterialGraphBlock_AttributeWorldPosition : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_AttributeWorldPosition, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Position"_id, MaterialOutputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        return compiler.vertexData(MaterialVertexDataType::WorldPosition);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_AttributeWorldPosition);
BLOCK_COMMON("WorldPosition")
RTTI_END_TYPE();

///---

class MaterialGraphBlock_AttributeWorldTangent : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_AttributeWorldTangent, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Tangent"_id, MaterialOutputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        return compiler.vertexData(MaterialVertexDataType::WorldTangent);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_AttributeWorldTangent);
BLOCK_COMMON("WorldTangent")
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE()
