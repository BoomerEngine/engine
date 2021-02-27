/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\globals #]
***/

#include "build.h"
#include "graphBlock.h"

BEGIN_BOOMER_NAMESPACE()

#define BLOCK_COMMOM(x) \
    RTTI_METADATA(graph::BlockInfoMetadata).title(x).group("Globals"); \
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialGlobal"); \
    RTTI_METADATA(graph::BlockShapeMetadata).rectangle();

///---

class MaterialGraphBlock_GlobalCameraDirection : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_GlobalCameraDirection, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Value"_id, MaterialOutputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        return CodeChunk(CodeChunkType::Numerical3, "CameraParams.CameraForward");
    }

private:
    float m_value;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_GlobalCameraDirection);
    BLOCK_COMMOM("Camera Direction")
RTTI_END_TYPE();

///---

class MaterialGraphBlock_GlobalCameraPosition : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_GlobalCameraPosition, MaterialGraphBlock);

public:
    MaterialGraphBlock_GlobalCameraPosition()
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Value"_id, MaterialOutputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        return CodeChunk(CodeChunkType::Numerical3, "CameraParams.CameraPosition");
    }

private:
    float m_value;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_GlobalCameraPosition);
BLOCK_COMMOM("Camera Position")
RTTI_END_TYPE();

///---

class MaterialGraphBlock_GlobalCameraRight : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_GlobalCameraRight, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Value"_id, MaterialOutputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        return CodeChunk(CodeChunkType::Numerical3, "CameraParams.CameraRight");
    }

private:
    float m_value;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_GlobalCameraRight);
BLOCK_COMMOM("Camera Right")
RTTI_END_TYPE();

///---

class MaterialGraphBlock_GlobalCameraUp : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_GlobalCameraUp, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Value"_id, MaterialOutputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        return CodeChunk(CodeChunkType::Numerical3, "CameraParams.CameraUp");
    }

private:
    float m_value;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_GlobalCameraUp);
BLOCK_COMMOM("Camera Up")
RTTI_END_TYPE();

///---

class MaterialGraphBlock_GlobalEngineTime : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_GlobalEngineTime, MaterialGraphBlock);

public:
    MaterialGraphBlock_GlobalEngineTime()
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Value"_id, MaterialOutputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        return CodeChunk(CodeChunkType::Numerical1, "FrameParams.EngineTime");
    }

private:
    float m_value;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_GlobalEngineTime);
    BLOCK_COMMOM("Engine Time")
RTTI_END_TYPE();

///---

class MaterialGraphBlock_GlobalGameTime : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_GlobalGameTime, MaterialGraphBlock);

public:
    MaterialGraphBlock_GlobalGameTime()
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Value"_id, MaterialOutputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        return CodeChunk(CodeChunkType::Numerical1, "FrameParams.GameTime");
    }

private:
    float m_value;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_GlobalGameTime);
BLOCK_COMMOM("Game Time")
RTTI_END_TYPE();



END_BOOMER_NAMESPACE()
