/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks #]
***/

#pragma once

#include "graphBlock.h"
#include "engine/material/include/runtimeLayout.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// material block bound to a material parameter
class ENGINE_MATERIAL_GRAPH_API IMaterialGraphBlockBoundParameter : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(IMaterialGraphBlockBoundParameter, MaterialGraphBlock);

public:
    IMaterialGraphBlockBoundParameter();

    // get bound parameter
    INLINE const StringID parameterName() const { return m_paramName; }

    // bind to new parameter, will rebuild the layout
    void bind(StringID paramName);

    // get type of parameter that can be bound here
    virtual MaterialDataLayoutParameterType parameterType() const = 0;

protected:
    virtual StringBuf chooseTitle() const override;
    virtual void onPropertyChanged(StringView path) override;

    StringID m_paramName;
};

//--

// static switch block
class ENGINE_MATERIAL_GRAPH_API MaterialGraphBlock_StaticSwitch : public IMaterialGraphBlockBoundParameter
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_StaticSwitch, IMaterialGraphBlockBoundParameter);

public:
    MaterialGraphBlock_StaticSwitch();

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override;
    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override;

protected:
    // get type of parameter that can be bound here
    virtual MaterialDataLayoutParameterType parameterType() const;

    bool m_invert = false;

    float m_valueFalse = 0.0f;
    float m_valueTrue = 1.0f;
};

//--
    
END_BOOMER_NAMESPACE()
