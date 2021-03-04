/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\parameters #]
***/

#include "build.h"
#include "graph.h"
#include "graphBlock_Parameter.h"
#include "engine/material/include/runtimeLayout.h"
#include "engine/texture/include/texture.h"

BEGIN_BOOMER_NAMESPACE()

#define BLOCK_COMMON(x) \
    RTTI_METADATA(graph::BlockInfoMetadata).name(x).group("Parameters"); \
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialParameter"); \
    RTTI_METADATA(graph::BlockShapeMetadata).slanted();  \

///---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IMaterialGraphBlockBoundParameter);
RTTI_PROPERTY(m_paramName);
RTTI_END_TYPE();

IMaterialGraphBlockBoundParameter::IMaterialGraphBlockBoundParameter()
{}

void IMaterialGraphBlockBoundParameter::bind(StringID paramName)
{
    if (m_paramName != paramName)
    {
        m_paramName = paramName;
        onPropertyChanged("paramName");
    }
}

void IMaterialGraphBlockBoundParameter::onPropertyChanged(StringView path)
{
    if (path == "paramName")
        invalidateStyle();

    TBaseClass::onPropertyChanged(path);
}

StringBuf IMaterialGraphBlockBoundParameter::chooseTitle() const
{
    if (m_paramName)
        return TempString("{} ({})", TBaseClass::chooseTitle(), m_paramName);
    else
        return TBaseClass::chooseTitle();
}

///---

class MaterialGraphBlock_BoundParameterColor : public IMaterialGraphBlockBoundParameter
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_BoundParameterColor, IMaterialGraphBlockBoundParameter);

public:
    MaterialGraphBlock_BoundParameterColor()
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("RGB"_id, MaterialOutputSocket().swizzle("xyz"_id));
        builder.socket("RGBA"_id, MaterialOutputSocket().hiddenByDefault());
        builder.socket("R"_id, MaterialOutputSocket().swizzle("x"_id).hiddenByDefault());
        builder.socket("G"_id, MaterialOutputSocket().swizzle("y"_id).hiddenByDefault());
        builder.socket("B"_id, MaterialOutputSocket().swizzle("z"_id).hiddenByDefault());
        builder.socket("A"_id, MaterialOutputSocket().swizzle("w"_id));
    }

    virtual MaterialDataLayoutParameterType parameterType() const override
    {
        return MaterialDataLayoutParameterType::Color;
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        if (const auto* entry = compiler.findParamEntry(m_paramName))
        {
            if (entry->type == MaterialDataLayoutParameterType::Color)
            {
                const auto& layout = compiler.dataLayout()->discreteDataLayout();
                return CodeChunk(CodeChunkType::Numerical4, TempString("{}.{}", layout.descriptorName, entry->name), false);
            }
        }

        return CodeChunk(Vector4(1, 1, 1, 1));
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_BoundParameterColor);
    BLOCK_COMMON("Parameter Color");
RTTI_END_TYPE();

///---

class MaterialGraphBlock_BoundParameterVector2 : public IMaterialGraphBlockBoundParameter
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_BoundParameterVector2, IMaterialGraphBlockBoundParameter);

public:
    MaterialGraphBlock_BoundParameterVector2()
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket());
        builder.socket("X"_id, MaterialOutputSocket().swizzle("x"_id).color(COLOR_SOCKET_RED).hiddenByDefault());
        builder.socket("Y"_id, MaterialOutputSocket().swizzle("y"_id).color(COLOR_SOCKET_GREEN).hiddenByDefault());
    }

    virtual MaterialDataLayoutParameterType parameterType() const override
    {
        return MaterialDataLayoutParameterType::Vector2;
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        if (const auto* entry = compiler.findParamEntry(m_paramName))
        {
            if (entry->type == MaterialDataLayoutParameterType::Vector2)
            {
                const auto& layout = compiler.dataLayout()->discreteDataLayout();
                return CodeChunk(CodeChunkType::Numerical2, TempString("{}.{}", layout.descriptorName, entry->name), false);
            }
        }

        return CodeChunk(Vector2(0, 0));
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_BoundParameterVector2);
BLOCK_COMMON("Parameter Vector2");
RTTI_END_TYPE();

///---

class MaterialGraphBlock_BoundParameterVector3 : public IMaterialGraphBlockBoundParameter
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_BoundParameterVector3, IMaterialGraphBlockBoundParameter);

public:
    MaterialGraphBlock_BoundParameterVector3()
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket());
        builder.socket("X"_id, MaterialOutputSocket().swizzle("x"_id).color(COLOR_SOCKET_RED).hiddenByDefault());
        builder.socket("Y"_id, MaterialOutputSocket().swizzle("y"_id).color(COLOR_SOCKET_GREEN).hiddenByDefault());
        builder.socket("Z"_id, MaterialOutputSocket().swizzle("z"_id).color(COLOR_SOCKET_BLUE).hiddenByDefault());
    }

    virtual MaterialDataLayoutParameterType parameterType() const override
    {
        return MaterialDataLayoutParameterType::Vector3;
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        if (const auto* entry = compiler.findParamEntry(m_paramName))
        {
            if (entry->type == MaterialDataLayoutParameterType::Vector3)
            {
                const auto& layout = compiler.dataLayout()->discreteDataLayout();
                return CodeChunk(CodeChunkType::Numerical3, TempString("{}.{}", layout.descriptorName, entry->name), false);
            }
        }

        return CodeChunk(Vector3(0, 0, 0));
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_BoundParameterVector3);
BLOCK_COMMON("Parameter Vector3");
RTTI_END_TYPE();

///---

class MaterialGraphBlock_BoundParameterVector4 : public IMaterialGraphBlockBoundParameter
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_BoundParameterVector4, IMaterialGraphBlockBoundParameter);

public:
    MaterialGraphBlock_BoundParameterVector4()
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket());
        builder.socket("X"_id, MaterialOutputSocket().swizzle("x"_id).color(COLOR_SOCKET_RED).hiddenByDefault());
        builder.socket("Y"_id, MaterialOutputSocket().swizzle("y"_id).color(COLOR_SOCKET_GREEN).hiddenByDefault());
        builder.socket("Z"_id, MaterialOutputSocket().swizzle("z"_id).color(COLOR_SOCKET_BLUE).hiddenByDefault());
        builder.socket("W"_id, MaterialOutputSocket().swizzle("w"_id).color(COLOR_SOCKET_ALPHA).hiddenByDefault());
    }

    virtual MaterialDataLayoutParameterType parameterType() const override
    {
        return MaterialDataLayoutParameterType::Vector4;
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        if (const auto* entry = compiler.findParamEntry(m_paramName))
        {
            if (entry->type == MaterialDataLayoutParameterType::Vector4)
            {
                const auto& layout = compiler.dataLayout()->discreteDataLayout();
                return CodeChunk(CodeChunkType::Numerical4, TempString("{}.{}", layout.descriptorName, entry->name), false);
            }
        }

        return CodeChunk(Vector4(0, 0, 0, 0));
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_BoundParameterVector4);
BLOCK_COMMON("Parameter Vector4");
RTTI_END_TYPE();

///---

class MaterialGraphBlock_BoundParameterScalar : public IMaterialGraphBlockBoundParameter
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_BoundParameterScalar, IMaterialGraphBlockBoundParameter);

public:
    MaterialGraphBlock_BoundParameterScalar()
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Value"_id, MaterialOutputSocket().hideCaption());
    }

    virtual MaterialDataLayoutParameterType parameterType() const override
    {
        return MaterialDataLayoutParameterType::Float;
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        if (const auto* entry = compiler.findParamEntry(m_paramName))
        {
            if (entry->type == MaterialDataLayoutParameterType::Float)
            {
                const auto& layout = compiler.dataLayout()->discreteDataLayout();
                return CodeChunk(CodeChunkType::Numerical1, TempString("{}.{}", layout.descriptorName, entry->name), false);
            }
        }

        return CodeChunk(0.0f);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_BoundParameterScalar);
    BLOCK_COMMON("Parameter Scalar");
RTTI_END_TYPE();

///---

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_StaticSwitch);
    BLOCK_COMMON("Static Switch");
    RTTI_PROPERTY(m_invert).editable("Invert the logic");
    RTTI_PROPERTY(m_valueFalse).editable("Default value to return for the false case");
    RTTI_PROPERTY(m_valueTrue).editable("Default value to return for the true case");
RTTI_END_TYPE();

MaterialGraphBlock_StaticSwitch::MaterialGraphBlock_StaticSwitch()
{}

MaterialDataLayoutParameterType MaterialGraphBlock_StaticSwitch::parameterType() const
{
    return MaterialDataLayoutParameterType::StaticBool;
}

void MaterialGraphBlock_StaticSwitch::buildLayout(graph::BlockLayoutBuilder& builder) const
{
    builder.socket("Value"_id, MaterialOutputSocket().hideCaption());
    builder.socket("0"_id, MaterialInputSocket());
    builder.socket("1"_id, MaterialInputSocket());
}

CodeChunk MaterialGraphBlock_StaticSwitch::compile(MaterialStageCompiler& compiler, StringID outputName) const
{
    const auto value = compiler.evalStaticSwitch(parameterName()) ^ m_invert;
    if (value)
        return compiler.evalInput(this, "1"_id, m_valueTrue);
    else
        return compiler.evalInput(this, "0"_id, m_valueFalse);
}

///---

END_BOOMER_NAMESPACE()
