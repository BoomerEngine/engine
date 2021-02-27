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

/// material parameter block
class ENGINE_MATERIAL_GRAPH_API MaterialGraphBlockParameter : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlockParameter, MaterialGraphBlock);

public:
    MaterialGraphBlockParameter();
    MaterialGraphBlockParameter(StringID name);

    // get name
    INLINE StringID name() const { return m_name; }

    // get display category
    INLINE StringID category() const { return m_category; }

    // get parameter type
    virtual Type dataType() const = 0;

    // get the current default value (matches the reported type)
    virtual const void* dataValue() const = 0;

    // get the parameter type
    virtual MaterialDataLayoutParameterType parameterType() const = 0;

    //--

    // read value
    virtual bool resetValue();

    // write new value
    virtual bool writeValue(const void* data, Type type);

    // read current value
    virtual bool readValue(void* data, Type type, bool defaultValueOnly = false) const;

protected:
    virtual StringBuf chooseTitle() const override;
    virtual void onPropertyChanged(StringView path) override;

private:
    StringID m_name;
    StringID m_category;
};

//--
    
END_BOOMER_NAMESPACE()
