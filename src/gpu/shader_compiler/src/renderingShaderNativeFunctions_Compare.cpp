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

// both types must have equal component count
static DataType DetermineCompareType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err, bool equalityTest)
{
    // math uses const values
    argTypes[0] = argTypes[0].unmakePointer();
    argTypes[1] = argTypes[1].unmakePointer();

    // we must have 2 arguments here
    if (numArgs != 2)
    {
        err.reportError(loc, "Comparison function requires 2 arguments");
        return DataType();
    }

    // get the base type of the operands
    auto baseType0 = ExtractBaseType(argTypes[0]);
    auto baseType1 = ExtractBaseType(argTypes[1]);
    if (baseType0 == BaseType::Invalid)
    {
        err.reportError(loc, TempString("Type '{}' is not valid type for compare operation", argTypes[0]));
        return DataType();
    }
    else if (baseType1 == BaseType::Invalid)
    {
        err.reportError(loc, TempString("Type '{}' is not valid type for compare operation", argTypes[1]));
        return DataType();
    }

    // the base type of operands must match
    if (baseType0 != baseType1)
    {
        // we cannot compare bool with non-bool
        if (baseType0 == BaseType::Boolean || baseType1 == BaseType::Boolean)
        {
            err.reportError(loc, "Cannot compare with boolean types");
            return DataType();
        }
        // we can promote numbers to floats
        else if (baseType0 == BaseType::Float || baseType1 == BaseType::Float)
        {
            err.reportWarning(loc, "float/Integer comparison");
            argTypes[0] = GetCastedType(typeLibrary, argTypes[0], BaseType::Float);
            argTypes[1] = GetCastedType(typeLibrary, argTypes[1], BaseType::Float);
        }
        else if ((baseType0 == BaseType::Uint && baseType1 == BaseType::Int) || (baseType0 == BaseType::Int && baseType1 == BaseType::Uint))
        {
            // we don't warn on simple comparisions
            if (!equalityTest)
                err.reportWarning(loc, "Signed/Unsigned comparison");
        }
        else 
        {
            err.reportError(loc, TempString("Comparison functions can only operate on types that are the same type '{}' is not the same as '{}'", argTypes[0], argTypes[1]));
            return DataType();
        }
    }

    // booleans are not allowed in relational comparisons
    if (baseType0 == BaseType::Boolean && !equalityTest)
    {
        err.reportError(loc, "Relational comparison is not allowed on boolean type");
        return DataType();
    }

    // both types must match in size
    auto componentCount0 = ExtractComponentCount(argTypes[0]);
    auto componentCount1 = ExtractComponentCount(argTypes[1]);
    if (componentCount0 == 1)
        componentCount0 = componentCount1;
    else if (componentCount1 == 1)
        componentCount1 = componentCount0;
    if (componentCount0 != componentCount1)
    {
        err.reportError(loc, "Operands of binary math function must have the same row/elem counts");
        return DataType();
    }

    // make sure both operands have the same amount of components
    if (baseType0 == BaseType::Boolean || baseType0 == BaseType::Int || baseType0 == BaseType::Uint || baseType0 == BaseType::Float)
        argTypes[0] = GetContractedType(typeLibrary, argTypes[0], componentCount0);

    if (baseType1 == BaseType::Boolean || baseType1 == BaseType::Int || baseType1 == BaseType::Uint || baseType1 == BaseType::Float)
        argTypes[1] = GetContractedType(typeLibrary, argTypes[1], componentCount1);

    // the comparison always returns bool
    return typeLibrary.booleanType(componentCount0);
}

//---

class FunctionCompareEq : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionCompareEq, INativeFunction);

public:
    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        return DetermineCompareType(typeLibrary, numArgs, argTypes, loc, err, true);
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        auto baseType = ExtractBaseType(argValues[0].type());
        ASSERT(baseType != BaseType::Invalid);

        for (uint32_t i = 0; i < retData.size(); ++i)
        {
            switch (baseType)
            {
                case BaseType::Float:
                    retData.component(i, valop::FloatOrderedEqual(argValues[0].component(i), argValues[1].component(i)));
                    break;

                case BaseType::Int:
                    retData.component(i, valop::IntEqual(argValues[0].component(i), argValues[1].component(i)));
                    break;

                case BaseType::Uint:
                    retData.component(i, valop::UintEqual(argValues[0].component(i), argValues[1].component(i)));
                    break;

                case BaseType::Boolean:
                    retData.component(i, valop::LogicalEqual(argValues[0].component(i), argValues[1].component(i)));
                    break;

                case BaseType::Name:
                    retData.component(i, valop::NameEqual(argValues[0].component(i), argValues[1].component(i)));
                    break;

                default: FATAL_ERROR("Invalid comparision type");
            }
        }
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionCompareEq);
    RTTI_METADATA(FunctionNameMetadata).name("__eq");
RTTI_END_TYPE();

//---

class FunctionCompareNotEq : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionCompareNotEq, INativeFunction);

public:
    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        return DetermineCompareType(typeLibrary, numArgs, argTypes, loc, err, true);
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        auto baseType = ExtractBaseType(argValues[0].type());
        ASSERT(baseType != BaseType::Invalid);

        for (uint32_t i = 0; i < retData.size(); ++i)
        {
            switch (baseType)
            {
                case BaseType::Float:
                    retData.component(i, valop::FloatOrderedNotEqual(argValues[0].component(i), argValues[1].component(i)));
                    break;

                case BaseType::Int:
                    retData.component(i, valop::IntNotEqual(argValues[0].component(i), argValues[1].component(i)));
                    break;

                case BaseType::Uint:
                    retData.component(i, valop::UintNotEqual(argValues[0].component(i), argValues[1].component(i)));
                    break;

                case BaseType::Boolean:
                    retData.component(i, valop::LogicalNotEqual(argValues[0].component(i), argValues[1].component(i)));
                    break;

                case BaseType::Name:
                    retData.component(i, valop::NameNotEqual(argValues[0].component(i), argValues[1].component(i)));
                    break;

                default: FATAL_ERROR("Invalid comparision type");
            }
        }
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionCompareNotEq);
    RTTI_METADATA(FunctionNameMetadata).name("__neq");
RTTI_END_TYPE();

//---

class FunctionCompareLess : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionCompareLess, INativeFunction);

public:
    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        return DetermineCompareType(typeLibrary, numArgs, argTypes, loc, err, false);
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        auto baseType = ExtractBaseType(argValues[0].type());
        ASSERT(baseType != BaseType::Invalid);

        for (uint32_t i = 0; i < retData.size(); ++i)
        {
            switch (baseType)
            {
                case BaseType::Float:
                    retData.component(i, valop::FloatOrderedLess(argValues[0].component(i), argValues[1].component(i)));
                    break;

                case BaseType::Int:
                    retData.component(i, valop::IntLess(argValues[0].component(i), argValues[1].component(i)));
                    break;

                case BaseType::Uint:
                    retData.component(i, valop::UintLess(argValues[0].component(i), argValues[1].component(i)));
                    break;

                default: FATAL_ERROR("Invalid comparision type");
            }
        }
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionCompareLess);
RTTI_METADATA(FunctionNameMetadata).name("__lt");
RTTI_END_TYPE();

//---

class FunctionCompareLessEqual : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionCompareLessEqual, INativeFunction);

public:
    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        return DetermineCompareType(typeLibrary, numArgs, argTypes, loc, err, false);
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        auto baseType = ExtractBaseType(argValues[0].type());
        ASSERT(baseType != BaseType::Invalid);

        for (uint32_t i = 0; i < retData.size(); ++i)
        {
            switch (baseType)
            {
                case BaseType::Float:
                    retData.component(i, valop::FloatOrderedLessEqual(argValues[0].component(i), argValues[1].component(i)));
                    break;

                case BaseType::Int:
                    retData.component(i, valop::IntLessEqual(argValues[0].component(i), argValues[1].component(i)));
                    break;

                case BaseType::Uint:
                    retData.component(i, valop::UintLessEqual(argValues[0].component(i), argValues[1].component(i)));
                    break;

                default: FATAL_ERROR("Invalid comparision type");
            }
        }
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionCompareLessEqual);
RTTI_METADATA(FunctionNameMetadata).name("__le");
RTTI_END_TYPE();

//---

class FunctionCompareGreater : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionCompareGreater, INativeFunction);

public:
    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        return DetermineCompareType(typeLibrary, numArgs, argTypes, loc, err, false);
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        auto baseType = ExtractBaseType(argValues[0].type());
        ASSERT(baseType != BaseType::Invalid);

        for (uint32_t i = 0; i < retData.size(); ++i)
        {
            switch (baseType)
            {
                case BaseType::Float:
                    retData.component(i, valop::FloatOrderedGreater(argValues[0].component(i), argValues[1].component(i)));
                    break;

                case BaseType::Int:
                    retData.component(i, valop::IntGreater(argValues[0].component(i), argValues[1].component(i)));
                    break;

                case BaseType::Uint:
                    retData.component(i, valop::UintGreater(argValues[0].component(i), argValues[1].component(i)));
                    break;

                default: FATAL_ERROR("Invalid comparision type");
            }
        }
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionCompareGreater);
RTTI_METADATA(FunctionNameMetadata).name("__gt");
RTTI_END_TYPE();

//---

class FunctionCompareGreaterEqual : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionCompareGreaterEqual, INativeFunction);

public:
    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        return DetermineCompareType(typeLibrary, numArgs, argTypes, loc, err, false);
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        auto baseType = ExtractBaseType(argValues[0].type());
        ASSERT(baseType != BaseType::Invalid);

        for (uint32_t i = 0; i < retData.size(); ++i)
        {
            switch (baseType)
            {
                case BaseType::Float:
                    retData.component(i, valop::FloatOrderedGreaterEqual(argValues[0].component(i), argValues[1].component(i)));
                    break;

                case BaseType::Int:
                    retData.component(i, valop::IntGreaterEqual(argValues[0].component(i), argValues[1].component(i)));
                    break;

                case BaseType::Uint:
                    retData.component(i, valop::UintGreaterEqual(argValues[0].component(i), argValues[1].component(i)));
                    break;

                default: FATAL_ERROR("Invalid comparision type");
            }
        }
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionCompareGreaterEqual);
RTTI_METADATA(FunctionNameMetadata).name("__ge");
RTTI_END_TYPE();

//---

class FunctionAll : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionAll, INativeFunction);

public:
    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        // we must have 1 arguments here
        if (numArgs != 1)
        {
            err.reportError(loc, "Function requires 1 argument");
            return DataType();
        }

        // math uses const values
        argTypes[0] = argTypes[0].unmakePointer();

        // get the base type of the operands
        auto baseType0 = ExtractBaseType(argTypes[0]);
        if (baseType0 != BaseType::Boolean)
        {
            err.reportError(loc, TempString("Type '{}' is not valid type for this function, this function requires boolean vector", argTypes[0]));
            return DataType();
        }

        // both types must match in size
        auto componentCount0 = ExtractComponentCount(argTypes[0]);
        if (componentCount0 == 1)
        {
            err.reportError(loc, "This function does not make sense on scalar argument");
            return DataType();
        }
        else if (componentCount0 > 4)
        {
            err.reportError(loc, "Only up to 4 components are supported");
            return DataType();
        }

        // const expr magic
        return DataType::BoolScalarType();
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override
    {
        DataValueComponent ret(true);

        for (uint32_t i=0; i<retData.size(); ++i)
            ret = valop::LogicalAnd(ret, valop::ToBool(argValues[0].component(i)));

        retData.component(0, ret);
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionAll);
    RTTI_METADATA(FunctionNameMetadata).name("all");
RTTI_END_TYPE();

//---

class FunctionAny : public FunctionAll
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionAny, FunctionAll);

public:
    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        DataValueComponent ret(false);

        for (uint32_t i=0; i<retData.size(); ++i)
            ret = valop::LogicalOr(ret, valop::ToBool(argValues[0].component(i)));

        retData.component(0, ret);
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionAny);
    RTTI_METADATA(FunctionNameMetadata).name("any");
RTTI_END_TYPE();

//---

END_BOOMER_NAMESPACE_EX(gpu::compiler)
