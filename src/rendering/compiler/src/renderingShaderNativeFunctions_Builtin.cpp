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

        // input is any vector, output has the same size
        static DataType DetermineBultiInType_Vector(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err, BaseType defaultOutputType = BaseType::Float, BaseType defaultIntputType = BaseType::Float)
        {
            // we must have a single argument
            if (numArgs != 1)
            {
                err.reportError(loc, "Function requires 1 argument");
                return DataType();
            }

            // get the base type of the operands
            auto baseType0 = ExtractBaseType(argTypes[0]);
            if (ExtractRowCount(baseType0) != 1)
            {
                err.reportError(loc, "Math operation on matrices are not supported");
                return DataType();
            }
            else if (!argTypes[0].isNumericalVectorLikeOperand())
            {
                err.reportError(loc, base::TempString("Type '{}' is not valid type for math operation", argTypes[0]));
                return DataType();
            }

            // request a cast
            auto componentCount = ExtractComponentCount(argTypes[0]);
            argTypes[0] = typeLibrary.simpleCompositeType(defaultIntputType, componentCount).unmakePointer();

            // output type has the same element count as the input type
            if (defaultIntputType == defaultOutputType)
                return argTypes[0];
            else
                return typeLibrary.simpleCompositeType(defaultOutputType, componentCount).unmakePointer();
        }

        // input is any vector, output has the same size
        static DataType DetermineBultiInType_MatchingVectors(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err)
        {
            // we must have 2 arguments here
            if (numArgs != 2)
            {
                err.reportError(loc, "Function requires 2 arguments");
                return DataType();
            }

            // get the base type of the operands
            auto baseType0 = ExtractBaseType(argTypes[0]);
            auto baseType1 = ExtractBaseType(argTypes[1]);
            if (ExtractRowCount(baseType0) != 1 || ExtractRowCount(baseType1) != 1)
            {
                err.reportError(loc, "Math operation on matrices are not supported");
                return DataType();
            }
            else if (!argTypes[0].isNumericalVectorLikeOperand())
            {
                err.reportError(loc, base::TempString("Type '{}' is not valid type for math operation", argTypes[0]));
                return DataType();
            }
            else if (!argTypes[1].isNumericalVectorLikeOperand())
            {
                err.reportError(loc, base::TempString("Type '{}' is not valid type for math operation", argTypes[1]));
                return DataType();
            }

            // request a cast
            auto componentCount = ExtractComponentCount(argTypes[0]);
            argTypes[0] = typeLibrary.simpleCompositeType(BaseType::Float, componentCount).unmakePointer();
            argTypes[1] = typeLibrary.simpleCompositeType(BaseType::Float, componentCount).unmakePointer();

            // output type has the same element count as the input type
            return argTypes[0];
        }

        // input is any vector, output has the same size
        static DataType DetermineBultiInType_Matching3Vectors(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err)
        {
            // we must have 2 arguments here
            if (numArgs != 3)
            {
                err.reportError(loc, "Function requires 3 arguments");
                return DataType();
            }

            // get the base type of the operands
            auto baseType0 = ExtractBaseType(argTypes[0]);
            auto baseType1 = ExtractBaseType(argTypes[1]);
            auto baseType2 = ExtractBaseType(argTypes[2]);
            if (ExtractRowCount(baseType0) != 1 || ExtractRowCount(baseType1) != 1 || ExtractRowCount(baseType2) != 1)
            {
                err.reportError(loc, "Math operation on matrices are not supported");
                return DataType();
            }
            else if (!argTypes[0].isNumericalVectorLikeOperand())
            {
                err.reportError(loc, base::TempString("Type '{}' is not valid type for math operation", argTypes[0]));
                return DataType();
            }
            else if (!argTypes[1].isNumericalVectorLikeOperand())
            {
                err.reportError(loc, base::TempString("Type '{}' is not valid type for math operation", argTypes[1]));
                return DataType();
            }
            else if (!argTypes[2].isNumericalVectorLikeOperand())
            {
                err.reportError(loc, base::TempString("Type '{}' is not valid type for math operation", argTypes[2]));
                return DataType();
            }

            // request a cast
            auto componentCount = ExtractComponentCount(argTypes[0]);
            argTypes[0] = typeLibrary.simpleCompositeType(BaseType::Float, componentCount).unmakePointer();
            argTypes[1] = typeLibrary.simpleCompositeType(BaseType::Float, componentCount).unmakePointer();
            argTypes[2] = typeLibrary.simpleCompositeType(BaseType::Float, componentCount).unmakePointer();

            // output type has the same element count as the input type
            return argTypes[0];
        }

        //---

        class FunctionSin : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionSin, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.numComponents(); ++i)
                {
                    retData.component(i, valop::Sin(argValues[0].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionSin);
        RTTI_METADATA(FunctionNameMetadata).name("sin");
        RTTI_END_TYPE();

        //---

        class FunctionCos : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionCos, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.numComponents(); ++i)
                {
                    retData.component(i, valop::Cos(argValues[0].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionCos);
        RTTI_METADATA(FunctionNameMetadata).name("cos");
        RTTI_END_TYPE();

        //---

        class FunctionTan : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionTan, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    retData.component(i, valop::Tan(argValues[0].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionTan);
        RTTI_METADATA(FunctionNameMetadata).name("tan");
        RTTI_END_TYPE();

        //---

        class FunctionAsin : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionAsin, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    retData.component(i, valop::Asin(argValues[0].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionAsin);
        RTTI_METADATA(FunctionNameMetadata).name("asin");
        RTTI_END_TYPE();

        //---

        class FunctionAcos : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionAcos, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    retData.component(i, valop::Acos(argValues[0].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionAcos);
        RTTI_METADATA(FunctionNameMetadata).name("acos");
        RTTI_END_TYPE();

        //---

        class FunctionAtan : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionAtan, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    retData.component(i, valop::Atan(argValues[0].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionAtan);
        RTTI_METADATA(FunctionNameMetadata).name("atan");
        RTTI_END_TYPE();

        //---

        class FunctionSinh : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionSinh, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    retData.component(i, valop::Sinh(argValues[0].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionSinh);
        RTTI_METADATA(FunctionNameMetadata).name("sinh");
        RTTI_END_TYPE();

        //---

        class FunctionCosh : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionCosh, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    retData.component(i, valop::Cosh(argValues[0].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionCosh);
        RTTI_METADATA(FunctionNameMetadata).name("cosh");
        RTTI_END_TYPE();

        //---

        class FunctionTanh : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionTanh, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    retData.component(i, valop::Tanh(argValues[0].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionTanh);
        RTTI_METADATA(FunctionNameMetadata).name("tanh");
        RTTI_END_TYPE();

        //---

        class FunctionASinh : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionASinh, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    retData.component(i, valop::Asinh(argValues[0].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionASinh);
        RTTI_METADATA(FunctionNameMetadata).name("asinh");
        RTTI_END_TYPE();

        //---

        class FunctionACosh : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionACosh, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    retData.component(i, valop::Acosh(argValues[0].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionACosh);
        RTTI_METADATA(FunctionNameMetadata).name("acosh");
        RTTI_END_TYPE();

        //---

        class FunctionATanh : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionATanh, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    retData.component(i, valop::Atanh(argValues[0].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionATanh);
        RTTI_METADATA(FunctionNameMetadata).name("atanh");
        RTTI_END_TYPE();

        //---

        class FunctionRound : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionRound, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    retData.component(i, valop::Round(argValues[0].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionRound);
        RTTI_METADATA(FunctionNameMetadata).name("round");
        RTTI_END_TYPE();

        //---

        class FunctionRoundEven : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionRoundEven, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    retData.component(i, valop::Round(argValues[0].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionRoundEven);
        RTTI_METADATA(FunctionNameMetadata).name("roundEven");
        RTTI_END_TYPE();

        //---

        class FunctionCeil : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionCeil, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    retData.component(i, valop::Ceil(argValues[0].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionCeil);
        RTTI_METADATA(FunctionNameMetadata).name("ceil");
        RTTI_END_TYPE();

        //---

        class FunctionFloor : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionFloor, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    retData.component(i, valop::Floor(argValues[0].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionFloor);
        RTTI_METADATA(FunctionNameMetadata).name("floor");
        RTTI_END_TYPE();

        //---

        class FunctionFrac : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionFrac, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    retData.component(i, valop::Frac(argValues[0].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionFrac);
        RTTI_METADATA(FunctionNameMetadata).name("frac");
        RTTI_END_TYPE();

        //---

        class FunctionTrunc : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionTrunc, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    retData.component(i, valop::Trunc(argValues[0].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionTrunc);
        RTTI_METADATA(FunctionNameMetadata).name("trunc");
        RTTI_END_TYPE();

        //---

        class FunctionAbs : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionAbs, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    retData.component(i, valop::Abs(argValues[0].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionAbs);
        RTTI_METADATA(FunctionNameMetadata).name("abs");
        RTTI_END_TYPE();

        //---

        class FunctionSign : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionSign, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    retData.component(i, valop::Sign(argValues[0].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionSign);
        RTTI_METADATA(FunctionNameMetadata).name("sign");
        RTTI_END_TYPE();

        //---

        class FunctionStep : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionStep, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_MatchingVectors(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    auto diff = valop::FSub(argValues[1].component(i), argValues[0].component(i));
                    retData.component(i, valop::Step(diff));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionStep);
        RTTI_METADATA(FunctionNameMetadata).name("step");
        RTTI_END_TYPE();

        //---

        class FunctionSaturate : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionSaturate, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    retData.component(i, valop::Clamp(argValues[0].component(i), DataValueComponent(0.0f), DataValueComponent(1.0f)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionSaturate);
        RTTI_METADATA(FunctionNameMetadata).name("saturate");
        RTTI_END_TYPE();

        //---

        class FunctionFloatBitsToInt : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionFloatBitsToInt, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err, BaseType::Int, BaseType::Float);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, DataValueComponent());
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionFloatBitsToInt);
        RTTI_METADATA(FunctionNameMetadata).name("floatBitsToInt");
        RTTI_END_TYPE();

        //---

        class FunctionFloatBitsToUint : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionFloatBitsToUint, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err, BaseType::Uint, BaseType::Float);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, DataValueComponent());
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionFloatBitsToUint);
        RTTI_METADATA(FunctionNameMetadata).name("floatBitsToUint");
        RTTI_END_TYPE();

        //---

        class FunctionUintBitsToFloat : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionUintBitsToFloat, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err, BaseType::Float, BaseType::Uint);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, DataValueComponent());
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionUintBitsToFloat);
        RTTI_METADATA(FunctionNameMetadata).name("uintBitsToFloat");
        RTTI_END_TYPE();

        //---

        class FunctionIntBitsToFloat : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionIntBitsToFloat, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err, BaseType::Float, BaseType::Int);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, DataValueComponent());
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionIntBitsToFloat);
        RTTI_METADATA(FunctionNameMetadata).name("intBitsToFloat");
        RTTI_END_TYPE();

        //---

        class FunctionUnpackHalf2FromUint : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionUnpackHalf2FromUint, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                if (numArgs != 1)
                {
                    err.reportError(loc, "Function requires 1 argument");
                    return DataType();
                }

                argTypes[0] = DataType::UnsignedScalarType();
                return typeLibrary.floatType(2);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, DataValueComponent());
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionUnpackHalf2FromUint);
        RTTI_METADATA(FunctionNameMetadata).name("unpackHalf2x16");
        RTTI_END_TYPE();

        //----

        class FunctionPackUintFromHalf2 : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionPackUintFromHalf2, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                if (numArgs != 1)
                {
                    err.reportError(loc, "Function requires 1 argument");
                    return DataType();
                }

                argTypes[0] = typeLibrary.floatType(2);
                return DataType::UnsignedScalarType();
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, DataValueComponent());
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionPackUintFromHalf2);
        RTTI_METADATA(FunctionNameMetadata).name("packHalf2x16");
        RTTI_END_TYPE();

        //----

        class FunctionUnpackVec4FromUint : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionUnpackVec4FromUint, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                if (numArgs != 1)
                {
                    err.reportError(loc, "Function requires 1 argument");
                    return DataType();
                }

                argTypes[0] = typeLibrary.unsignedType();
                return typeLibrary.floatType(4);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, DataValueComponent());
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionUnpackVec4FromUint);
        RTTI_METADATA(FunctionNameMetadata).name("unpackUnorm4x8");
        RTTI_END_TYPE();

        //----

        class FunctionUnpackVec4FromUintSigned : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionUnpackVec4FromUintSigned, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                if (numArgs != 1)
                {
                    err.reportError(loc, "Function requires 1 argument");
                    return DataType();
                }

                argTypes[0] = typeLibrary.unsignedType();
                return typeLibrary.floatType(4);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, DataValueComponent());
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionUnpackVec4FromUintSigned);
        RTTI_METADATA(FunctionNameMetadata).name("unpackSnorm4x8");
        RTTI_END_TYPE();

        //----

        class FunctionUnpackVec2FromUint : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionUnpackVec2FromUint, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                if (numArgs != 1)
                {
                    err.reportError(loc, "Function requires 1 argument");
                    return DataType();
                }

                argTypes[0] = typeLibrary.unsignedType();
                return typeLibrary.floatType(2);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, DataValueComponent());
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionUnpackVec2FromUint);
        RTTI_METADATA(FunctionNameMetadata).name("unpackUnorm2x16");
        RTTI_END_TYPE();

        //----

        class FunctionUnpackVec2FromUintSigned : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionUnpackVec2FromUintSigned, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                if (numArgs != 1)
                {
                    err.reportError(loc, "Function requires 1 argument");
                    return DataType();
                }

                argTypes[0] = typeLibrary.unsignedType();
                return typeLibrary.floatType(2);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, DataValueComponent());
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionUnpackVec2FromUintSigned);
        RTTI_METADATA(FunctionNameMetadata).name("unpackSnorm2x16");
        RTTI_END_TYPE();

        //---

        class FunctionPackUintFromVec4 : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionPackUintFromVec4, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                if (numArgs != 1)
                {
                    err.reportError(loc, "Function requires 1 argument");
                    return DataType();
                }

                argTypes[0] = typeLibrary.floatType(4);
                return typeLibrary.unsignedType();
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, DataValueComponent());
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionPackUintFromVec4);
        RTTI_METADATA(FunctionNameMetadata).name("packUnorm4x8");
        RTTI_END_TYPE();

        //---

        class FunctionPackUintFromVec4Signed : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionPackUintFromVec4Signed, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                if (numArgs != 1)
                {
                    err.reportError(loc, "Function requires 1 argument");
                    return DataType();
                }

                argTypes[0] = typeLibrary.floatType(4);
                return typeLibrary.unsignedType();
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, DataValueComponent());
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionPackUintFromVec4Signed);
        RTTI_METADATA(FunctionNameMetadata).name("packSnorm4x8");
        RTTI_END_TYPE();

        //---

        class FunctionPackUintFromVec2 : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionPackUintFromVec2, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                if (numArgs != 1)
                {
                    err.reportError(loc, "Function requires 1 argument");
                    return DataType();
                }

                argTypes[0] = typeLibrary.floatType(2);
                return typeLibrary.unsignedType();
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, DataValueComponent());
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionPackUintFromVec2);
        RTTI_METADATA(FunctionNameMetadata).name("packUnorm2x16");
        RTTI_END_TYPE();

        //---

        class FunctionPackUintFromVec2Signed : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionPackUintFromVec2Signed, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                if (numArgs != 1)
                {
                    err.reportError(loc, "Function requires 1 argument");
                    return DataType();
                }

                argTypes[0] = typeLibrary.floatType(2);
                return typeLibrary.unsignedType();
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, DataValueComponent());
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionPackUintFromVec2Signed);
        RTTI_METADATA(FunctionNameMetadata).name("packSnorm2x16");
        RTTI_END_TYPE();

        //---

        class FunctionMin : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionMin, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_MatchingVectors(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    retData.component(i, valop::Min(argValues[0].component(i), argValues[1].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionMin);
        RTTI_METADATA(FunctionNameMetadata).name("min");
        RTTI_END_TYPE();

        //---

        class FunctionMax : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionMax, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_MatchingVectors(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    retData.component(i, valop::Max(argValues[0].component(i), argValues[1].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionMax);
        RTTI_METADATA(FunctionNameMetadata).name("max");
        RTTI_END_TYPE();

        //---

        class FunctionClamp : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionClamp, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Matching3Vectors(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    retData.component(i, valop::Clamp(argValues[0].component(i), argValues[1].component(i), argValues[2].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionClamp);
        RTTI_METADATA(FunctionNameMetadata).name("clamp");
        RTTI_END_TYPE();

        //---

        class FunctionSmoothStep : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionSmoothStep, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Matching3Vectors(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                {
                    retData.component(i, valop::Smoothstep(argValues[0].component(i), argValues[1].component(i), argValues[2].component(i)));
                }
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionSmoothStep);
        RTTI_METADATA(FunctionNameMetadata).name("smoothstep");
        RTTI_END_TYPE();

        //--

        class FunctionDDX : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionDDX, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, DataValueComponent());
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionDDX);
            RTTI_METADATA(FunctionNameMetadata).name("ddx");
        RTTI_END_TYPE();

        //--

        class FunctionDDXCoarse : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionDDXCoarse, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, DataValueComponent());
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionDDXCoarse);
        RTTI_METADATA(FunctionNameMetadata).name("ddx_coarse");
        RTTI_END_TYPE();

        //--

        class FunctionDDXFine : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionDDXFine, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, DataValueComponent());
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionDDXFine);
        RTTI_METADATA(FunctionNameMetadata).name("ddx_fine");
        RTTI_END_TYPE();

        //--

        class FunctionDDY : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionDDY, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, DataValueComponent());
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionDDY);
        RTTI_METADATA(FunctionNameMetadata).name("ddy");
        RTTI_END_TYPE();

        //--

        class FunctionDDYCoarse : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionDDYCoarse, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, DataValueComponent());
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionDDYCoarse);
        RTTI_METADATA(FunctionNameMetadata).name("ddy_coarse");
        RTTI_END_TYPE();

        //--

        class FunctionDDYFine : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionDDYFine, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, DataValueComponent());
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionDDYFine);
        RTTI_METADATA(FunctionNameMetadata).name("ddy_fine");
        RTTI_END_TYPE();

        class FunctionFWidth : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionFWidth, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineBultiInType_Vector(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, DataValueComponent());
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionFWidth);
            RTTI_METADATA(FunctionNameMetadata).name("fwidth");
        RTTI_END_TYPE();

        //--

        class FunctionEmitVertex : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionEmitVertex, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                if (numArgs != 0)
                {
                    err.reportError(loc, "Function does not take any arguments");
                    return DataType();
                }

                return DataType(BaseType::Void);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionEmitVertex);
            RTTI_METADATA(FunctionNameMetadata).name("EmitVertex");
        RTTI_END_TYPE();

        //--

        class FunctionEndPrimitive : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionEndPrimitive, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                if (numArgs != 0)
                {
                    err.reportError(loc, "Function does not take any arguments");
                    return DataType();
                }

                return DataType(BaseType::Void);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionEndPrimitive);
            RTTI_METADATA(FunctionNameMetadata).name("EndPrimitive");
        RTTI_END_TYPE();

        //--

    } // shader
} // rendering

