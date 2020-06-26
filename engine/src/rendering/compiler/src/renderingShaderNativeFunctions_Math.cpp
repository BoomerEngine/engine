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

namespace rendering
{
    namespace compiler
    {

        // promote types to the most common one (precedence order int -> float), changes the input types if needed (for casting)
        // if this function fails there's no common type
        static DataType DetermineMathType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err)
        {
            // math uses const values
            argTypes[0] = argTypes[0].unmakePointer();
            argTypes[1] = argTypes[1].unmakePointer();

            // we must have 2 arguments here
            if (numArgs != 2)
            {
                err.reportError(loc, "Binary function requires 2 arguments");
                return DataType();
            }

            // get the base type of the operands
            auto baseType0 = ExtractBaseType(argTypes[0]);
            auto baseType1 = ExtractBaseType(argTypes[1]);
            if (baseType0 == BaseType::Invalid)
            {
                err.reportError(loc, base::TempString("Type '{}' is not valid type for math operation", argTypes[0]));
                return DataType();
            }
            else if (baseType1 == BaseType::Invalid)
            {
                err.reportError(loc, base::TempString("Type '{}' is not valid type for math operation", argTypes[1]));
                return DataType();
            }

            // the base type of operands must match
            if (baseType0 != baseType1)
            {
                // we can convert to floats
                if (baseType0 == BaseType::Float || baseType1 == BaseType::Float)
                {
                    argTypes[0] = GetCastedType(typeLibrary, argTypes[0], BaseType::Float);
                    argTypes[1] = GetCastedType(typeLibrary, argTypes[1], BaseType::Float);
                }
                // we can convert to signed
                else if (baseType0 == BaseType::Int || baseType1 == BaseType::Int)
                {
                    argTypes[0] = GetCastedType(typeLibrary, argTypes[0], BaseType::Int);
                    argTypes[1] = GetCastedType(typeLibrary, argTypes[1], BaseType::Int);
                }
                else
                {
                    err.reportError(loc, "Math functions can only operate on numerical types");
                    return DataType();
                }
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

            // force type case
            argTypes[0] = GetContractedType(typeLibrary, argTypes[0], componentCount0);
            argTypes[1] = GetContractedType(typeLibrary, argTypes[1], componentCount1);

            // const expr magic
            auto retType = argTypes[0];
            return retType;
        }

        static DataType DetermineUnaryMathType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err)
        {
            argTypes[0] = argTypes[0].unmakePointer();
            if (numArgs != 1)
            {
                err.reportError(loc, "Math unary functions require one argument");
                return DataType();
            }

            // unary math operations work only with floats and ints
            auto baseType0 = ExtractBaseType(argTypes[0]);
            if (baseType0 != BaseType::Float && baseType0 != BaseType::Int && baseType0 != BaseType::Uint)
            {
                err.reportError(loc, "Math unary functions can only operate on numerical types");
                return DataType();
            }

            return argTypes[0];
        }

        static DataType DetermineUnaryMathTypeFloat(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err)
        {
            argTypes[0] = argTypes[0].unmakePointer();
            if (numArgs != 1)
            {
                err.reportError(loc, "Math unary functions require one argument");
                return DataType();
            }

            // unary math operations work only with floats and ints
            auto baseType0 = ExtractBaseType(argTypes[0]);
            if (baseType0 != BaseType::Float)
            {
                err.reportError(loc, "Math unary functions can only operate on numerical types");
                return DataType();
            }

            argTypes[0] = typeLibrary.floatType(ExtractComponentCount(argTypes[0]));
            return argTypes[0];
        }

        //---

        class FunctionAdd : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionAdd, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineMathType(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                auto baseType = ExtractBaseType(retData.type());
                ASSERT(baseType != BaseType::Invalid && baseType != BaseType::Boolean);

                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    if (baseType == BaseType::Float)
                        retData.component(i, valop::FAdd(argValues[0].component(i), argValues[1].component(i)));
                    else if (baseType == BaseType::Int)
                        retData.component(i, valop::IAdd(argValues[0].component(i), argValues[1].component(i)));
                    else if (baseType == BaseType::Uint)
                        retData.component(i, valop::UAdd(argValues[0].component(i), argValues[1].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionAdd);
            RTTI_METADATA(FunctionNameMetadata).name("__add");
        RTTI_END_TYPE();

        //---

        class FunctionDiv : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionDiv, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineMathType(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                auto baseType = ExtractBaseType(retData.type());
                ASSERT(baseType != BaseType::Invalid && baseType != BaseType::Boolean);

                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    if (baseType == BaseType::Float)
                        retData.component(i, valop::FDiv(argValues[0].component(i), argValues[1].component(i)));
                    else if (baseType == BaseType::Int)
                        retData.component(i, valop::IDiv(argValues[0].component(i), argValues[1].component(i)));
                    else if (baseType == BaseType::Uint)
                        retData.component(i, valop::UDiv(argValues[0].component(i), argValues[1].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionDiv);
        RTTI_METADATA(FunctionNameMetadata).name("__div");
        RTTI_END_TYPE();

        //---

        class FunctionMod : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionMod, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineMathType(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                auto baseType = ExtractBaseType(retData.type());
                ASSERT(baseType != BaseType::Invalid && baseType != BaseType::Boolean);

                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    if (baseType == BaseType::Float)
                        retData.component(i, valop::FMod(argValues[0].component(i), argValues[1].component(i)));
                    else if (baseType == BaseType::Int)
                        retData.component(i, valop::IMod(argValues[0].component(i), argValues[1].component(i)));
                    else if (baseType == BaseType::Uint)
                        retData.component(i, valop::UMod(argValues[0].component(i), argValues[1].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionMod);
        RTTI_METADATA(FunctionNameMetadata).name("__mod");
        RTTI_END_TYPE();

        //---

        class FunctionMul : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionMul, INativeFunction);

        public:
            virtual const INativeFunction* mutateFunction(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                if (numArgs == 2)
                {
                    auto& leftType = argTypes[0];
                    auto& rightType = argTypes[1];

                    if (leftType.isScalar() && rightType.isMatrix())
                    {
                        return FindFunctionByName("__smmul");
                    }
                    else if (leftType.isScalar() && rightType.isVector())
                    {
                        return FindFunctionByName("__svmul");
                    }
                    else if (leftType.isVector() && rightType.isScalar())
                    {
                        return FindFunctionByName("__vsmul");
                    }
                    else if (leftType.isMatrix() && rightType.isScalar())
                    {
                        return FindFunctionByName("__msmul");
                    }
                    else if (leftType.isMatrix() && rightType.isVector())
                    {
                        return FindFunctionByName("__mvmul");
                    }
                    else if (leftType.isVector() && rightType.isMatrix())
                    {
                        return FindFunctionByName("__vmmul");
                    }
                    else if (leftType.isMatrix() && rightType.isMatrix())
                    {
                        return FindFunctionByName("__mmmul");
                    }
                }

                return this;
            }

            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineMathType(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                auto baseType = ExtractBaseType(retData.type());
                ASSERT(baseType != BaseType::Invalid && baseType != BaseType::Boolean);

                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    if (baseType == BaseType::Float)
                        retData.component(i, valop::FMul(argValues[0].component(i), argValues[1].component(i)));
                    else if (baseType == BaseType::Int)
                        retData.component(i, valop::IMul(argValues[0].component(i), argValues[1].component(i)));
                    else if (baseType == BaseType::Uint)
                        retData.component(i, valop::UMul(argValues[0].component(i), argValues[1].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionMul);
        RTTI_METADATA(FunctionNameMetadata).name("__mul");
        RTTI_END_TYPE();

        //---

        class FunctionSub : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionSub, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineMathType(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                auto baseType = ExtractBaseType(retData.type());
                ASSERT(baseType != BaseType::Invalid && baseType != BaseType::Boolean);

                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    if (baseType == BaseType::Float)
                        retData.component(i, valop::FSub(argValues[0].component(i), argValues[1].component(i)));
                    else if (baseType == BaseType::Int)
                        retData.component(i, valop::ISub(argValues[0].component(i), argValues[1].component(i)));
                    else if (baseType == BaseType::Uint)
                        retData.component(i, valop::USub(argValues[0].component(i), argValues[1].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionSub);
        RTTI_METADATA(FunctionNameMetadata).name("__sub");
        RTTI_END_TYPE();

        //---

        class FunctionLerp : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionLerp, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                if (numArgs != 3)
                {
                    err.reportError(loc, "Function requires 3 arguments");
                    return DataType();
                }

                argTypes[2] = DataType::FloatScalarType();

                return DetermineMathType(typeLibrary, 2, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                auto baseType = ExtractBaseType(retData.type());
                ASSERT(baseType != BaseType::Invalid && baseType != BaseType::Boolean);

                auto factor = argValues[2].component(0);
                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    retData.component(i, valop::Lerp(argValues[0].component(i), argValues[1].component(i), factor));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionLerp);
        RTTI_METADATA(FunctionNameMetadata).name("lerp");
        RTTI_END_TYPE();

        //---

        class FunctionNeg : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionNeg, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineUnaryMathType(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                auto baseType = ExtractBaseType(retData.type());
                ASSERT(baseType != BaseType::Invalid && baseType != BaseType::Boolean);

                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    if (baseType == BaseType::Float)
                        retData.component(i, valop::FNeg(argValues[0].component(i)));
                    else if (baseType == BaseType::Int)
                        retData.component(i, valop::INeg(argValues[0].component(i)));
                    else if (baseType == BaseType::Uint)
                        retData.component(i, valop::UNeg(argValues[0].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionNeg);
        RTTI_METADATA(FunctionNameMetadata).name("__neg");
        RTTI_END_TYPE();

        //---

        class FunctionSqrt : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionSqrt, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineUnaryMathTypeFloat(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                auto baseType = ExtractBaseType(retData.type());
                ASSERT(baseType != BaseType::Invalid && baseType != BaseType::Boolean);

                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, valop::Sqrt(argValues[0].component(i)));
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionSqrt);
        RTTI_METADATA(FunctionNameMetadata).name("sqrt");
        RTTI_END_TYPE();

        //---

        class FunctionRsqrt : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionRsqrt, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineUnaryMathTypeFloat(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                auto baseType = ExtractBaseType(retData.type());
                ASSERT(baseType != BaseType::Invalid && baseType != BaseType::Boolean);

                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, valop::Rsqrt(argValues[0].component(i)));
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionRsqrt);
        RTTI_METADATA(FunctionNameMetadata).name("rsqrt");
        RTTI_END_TYPE();

        //---

        class FunctionAtan2 : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionAtan2, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineMathType(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                auto baseType = ExtractBaseType(retData.type());
                ASSERT(baseType != BaseType::Invalid && baseType != BaseType::Boolean);

                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, valop::Atan2(argValues[0].component(i), argValues[1].component(i)));
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionAtan2);
            RTTI_METADATA(FunctionNameMetadata).name("atan2");
        RTTI_END_TYPE();

        //---

        class FunctionPow : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionPow, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineMathType(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                auto baseType = ExtractBaseType(retData.type());
                ASSERT(baseType != BaseType::Invalid && baseType != BaseType::Boolean);

                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, valop::Pow(argValues[0].component(i), argValues[1].component(i)));
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionPow);
            RTTI_METADATA(FunctionNameMetadata).name("pow");
        RTTI_END_TYPE();

        //---

        class FunctionMod2 : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionMod2, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineMathType(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                auto baseType = ExtractBaseType(retData.type());
                ASSERT(baseType != BaseType::Invalid && baseType != BaseType::Boolean);

                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, valop::FMod(argValues[0].component(i), argValues[1].component(i)));
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionMod2);
        RTTI_METADATA(FunctionNameMetadata).name("mod");
        RTTI_END_TYPE();

        //---

        class FunctionLog : public INativeFunction
        {
        RTTI_DECLARE_VIRTUAL_CLASS(FunctionLog, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineUnaryMathTypeFloat(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                auto baseType = ExtractBaseType(retData.type());
                ASSERT(baseType != BaseType::Invalid && baseType != BaseType::Boolean);

                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, valop::Log(argValues[0].component(i)));
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionLog);
            RTTI_METADATA(FunctionNameMetadata).name("log");
        RTTI_END_TYPE();

        //---

        class FunctionLog2 : public INativeFunction
        {
        RTTI_DECLARE_VIRTUAL_CLASS(FunctionLog2, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineUnaryMathTypeFloat(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                auto baseType = ExtractBaseType(retData.type());
                ASSERT(baseType != BaseType::Invalid && baseType != BaseType::Boolean);

                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, valop::Log2(argValues[0].component(i)));
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionLog2);
            RTTI_METADATA(FunctionNameMetadata).name("log2");
        RTTI_END_TYPE();

        //---

        class FunctionExp : public INativeFunction
        {
        RTTI_DECLARE_VIRTUAL_CLASS(FunctionExp, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineUnaryMathTypeFloat(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                auto baseType = ExtractBaseType(retData.type());
                ASSERT(baseType != BaseType::Invalid && baseType != BaseType::Boolean);

                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, valop::Exp(argValues[0].component(i)));
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionExp);
            RTTI_METADATA(FunctionNameMetadata).name("exp");
        RTTI_END_TYPE();

        //---

        class FunctionExp2 : public INativeFunction
        {
        RTTI_DECLARE_VIRTUAL_CLASS(FunctionExp2, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineUnaryMathTypeFloat(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                auto baseType = ExtractBaseType(retData.type());
                ASSERT(baseType != BaseType::Invalid && baseType != BaseType::Boolean);

                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, valop::Exp2(argValues[0].component(i)));
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionExp2);
            RTTI_METADATA(FunctionNameMetadata).name("exp2");
        RTTI_END_TYPE();

        //---

    } // shader
} // rendering

