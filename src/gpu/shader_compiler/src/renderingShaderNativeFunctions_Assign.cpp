/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\functions #]
***/

#include "build.h"
#include "renderingShaderFunction.h"
#include "renderingShaderNativeFunction.h"
#include "renderingShaderTypeUtils.h"
#include "renderingShaderTypeLibrary.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::compiler)

///---

class FunctionAssign : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionAssign, INativeFunction);

public:
    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        if (numArgs != 2)
            return DataType();

        argTypes[0] = argTypes[0].makePointer();
        argTypes[1] = argTypes[0].unmakePointer();
        return DataType(BaseType::Void);
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        argValues[0].writeValue(argValues[1]);
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionAssign);
    RTTI_METADATA(FunctionNameMetadata).name("__assign");
RTTI_END_TYPE();


///---

class FunctionSelect : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionSelect, INativeFunction);

public:
    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        argTypes[1] = argTypes[1].unmakePointer(); // we need to load before select :(
        argTypes[2] = argTypes[1];
        argTypes[0] = DataType::BoolScalarType();
        return argTypes[1];
    }

    virtual bool partialEvaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        if (numArgs == 2)
        {
            auto selector = argValues[0].component(0);
            auto& optionTrue = argValues[1];
            if (selector.isDefined() && selector.valueBool())
            {
                retData = optionTrue;
                return true;
            }
        }

        return false;
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        auto selector = argValues[0].component(0);
        const auto& optionTrue = argValues[1];
        const auto& optionFalse = argValues[2];

        if (selector.isDefined() && selector.valueBool())
        {
            retData = optionTrue;
        }
        else if (selector.isDefined() && !selector.valueBool())
        {
            retData = optionFalse;
        }
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionSelect);
    RTTI_METADATA(FunctionNameMetadata).name("__select");
RTTI_END_TYPE();

END_BOOMER_NAMESPACE_EX(gpu::compiler)
