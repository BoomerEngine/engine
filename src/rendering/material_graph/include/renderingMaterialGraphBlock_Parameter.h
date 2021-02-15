/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks #]
***/

#pragma once

#include "renderingMaterialGraphBlock.h"
#include "rendering/material/include/renderingMaterialRuntimeLayout.h"

namespace rendering
{
    //--

    /// material parameter block
    class RENDERING_MATERIAL_GRAPH_API MaterialGraphBlockParameter : public MaterialGraphBlock
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlockParameter, MaterialGraphBlock);

    public:
        MaterialGraphBlockParameter();
        MaterialGraphBlockParameter(base::StringID name);

        // get name
        INLINE base::StringID name() const { return m_name; }

        // get display category
        INLINE base::StringID category() const { return m_category; }

        // get parameter type
        virtual base::Type dataType() const = 0;

        // get the current default value (matches the reported type)
        virtual const void* dataValue() const = 0;

        // get the parameter type
        virtual MaterialDataLayoutParameterType parameterType() const = 0;

        //--

        // read value
        virtual bool resetValue();

        // write new value
        virtual bool writeValue(const void* data, base::Type type);

        // read current value
        virtual bool readValue(void* data, base::Type type, bool defaultValueOnly = false) const;

    protected:
        virtual base::StringBuf chooseTitle() const override;
        virtual void onPropertyChanged(base::StringView path) override;

    private:
        base::StringID m_name;
        base::StringID m_category;
    };

    //--
    
} // rendering