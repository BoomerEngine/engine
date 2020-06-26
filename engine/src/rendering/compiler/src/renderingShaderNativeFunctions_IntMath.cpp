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

        // logical operations require both types to be the same, conversion from float to int is not legal
        static DataType DetermineIntMathType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err)
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

            // we can convert to floats
            if (baseType0 == BaseType::Float || baseType1 == BaseType::Float)
            {
                err.reportError(loc, "Bit-wise functions are not compatible with floating point types");
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

            // const expr magic
            auto retType = argTypes[0];
            argTypes[1] = argTypes[0];

            // we have a valid common type for the expression
            return retType;
        }

        static DataType DetermineUnaryIntMathType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err)
        {
            argTypes[0] = argTypes[0].unmakePointer();
            if (numArgs != 1)
            {
                err.reportError(loc, "Math unary functions require one argument");
                return DataType();
            }

            // unary math operations work only with floats and ints
            auto baseType0 = ExtractBaseType(argTypes[0]);
            if (baseType0 != BaseType::Int && baseType0 != BaseType::Uint)
            {
                err.reportError(loc, "Bitwise functions can only operate on numerical types");
                return DataType();
            }

            return argTypes[0];
        }

        static DataType DetermineUnaryBoolMathType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err)
        {
            argTypes[0] = argTypes[0].unmakePointer();
            if (numArgs != 1)
            {
                err.reportError(loc, "Math unary functions require one argument");
                return DataType();
            }

            argTypes[0] = DataType::BoolScalarType();
            return argTypes[0];
        }

        //---

        class FunctionAnd : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionAnd, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineIntMathType(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                auto baseType = ExtractBaseType(retData.type());
                ASSERT(baseType != BaseType::Invalid && baseType != BaseType::Boolean);

                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, valop::BitwiseAnd(argValues[0].component(i), argValues[1].component(i)));

                //TRACE_INFO("And: {} {} -> {}", argValues[0].component(0), argValues[1].component(0), retData.component(0));
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionAnd);
        RTTI_METADATA(FunctionNameMetadata).name("__and");
        RTTI_END_TYPE();

        //---

        class FunctionLogicAnd : public INativeFunction
        {
        RTTI_DECLARE_VIRTUAL_CLASS(FunctionLogicAnd, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                argTypes[0] = argTypes[1] = DataType::BoolScalarType();
                return DataType::BoolScalarType();
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                auto baseType = ExtractBaseType(retData.type());
                ASSERT(baseType == BaseType::Boolean);

                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, valop::LogicalAnd(argValues[0].component(i), argValues[1].component(i)));
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionLogicAnd);
            RTTI_METADATA(FunctionNameMetadata).name("__logicAnd");
        RTTI_END_TYPE();

        //---

        class FunctionOr : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionOr, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineIntMathType(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                auto baseType = ExtractBaseType(retData.type());
                ASSERT(baseType != BaseType::Invalid && baseType != BaseType::Boolean);

                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, valop::BitwiseOr(argValues[0].component(i), argValues[1].component(i)));
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionOr);
        RTTI_METADATA(FunctionNameMetadata).name("__or");
        RTTI_END_TYPE();

        //---

        class FunctionLogicOr : public INativeFunction
        {
        RTTI_DECLARE_VIRTUAL_CLASS(FunctionLogicOr, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                argTypes[0] = argTypes[1] = DataType::BoolScalarType();
                return DataType::BoolScalarType();
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                auto baseType = ExtractBaseType(retData.type());
                ASSERT(baseType == BaseType::Boolean);

                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, valop::LogicalOr(argValues[0].component(i), argValues[1].component(i)));
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionLogicOr);
            RTTI_METADATA(FunctionNameMetadata).name("__logicOr");
        RTTI_END_TYPE();

        //---

        class FunctionXor : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionXor, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineIntMathType(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                auto baseType = ExtractBaseType(retData.type());
                ASSERT(baseType != BaseType::Invalid && baseType != BaseType::Boolean);

                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, valop::BitwiseXor(argValues[0].component(i), argValues[1].component(i)));
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionXor);
        RTTI_METADATA(FunctionNameMetadata).name("__xor");
        RTTI_END_TYPE();

        //---

        class FunctionNot : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionNot, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineUnaryIntMathType(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                auto baseType = ExtractBaseType(retData.type());
                ASSERT(baseType != BaseType::Invalid && baseType != BaseType::Boolean);

                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, valop::BitwiseNot(argValues[0].component(i)));

                //TRACE_INFO("Not: {} {} -> {}", argValues[0].component(0), retData.component(0));
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionNot);
            RTTI_METADATA(FunctionNameMetadata).name("__not");
        RTTI_END_TYPE();

        //---

        class FunctionLogicalNot : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionLogicalNot, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineUnaryBoolMathType(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                auto baseType = ExtractBaseType(retData.type());
                ASSERT(baseType == BaseType::Boolean);

                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, valop::LogicalNot(argValues[0].component(i)));

                //TRACE_INFO("LogicalNot: {} {} -> {}", argValues[0].component(0), retData.component(0));
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionLogicalNot);
            RTTI_METADATA(FunctionNameMetadata).name("__logicalNot");
        RTTI_END_TYPE();

        //---

        class FunctionBitShiftLeft : public INativeFunction
        {
        RTTI_DECLARE_VIRTUAL_CLASS(FunctionBitShiftLeft, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineIntMathType(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                auto baseType = ExtractBaseType(retData.type());
                ASSERT(baseType != BaseType::Invalid && baseType != BaseType::Boolean);

                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, valop::LogicalShiftLeft(argValues[0].component(i), argValues[1].component(i)));
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionBitShiftLeft);
            RTTI_METADATA(FunctionNameMetadata).name("__shl");
        RTTI_END_TYPE();

        //---

        class FunctionBitShiftRight : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionBitShiftRight, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineIntMathType(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                auto baseType = ExtractBaseType(retData.type());
                ASSERT(baseType != BaseType::Invalid && baseType != BaseType::Boolean);

                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, valop::LogicalShiftRight(argValues[0].component(i), argValues[1].component(i)));
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionBitShiftRight);
            RTTI_METADATA(FunctionNameMetadata).name("__shr");
        RTTI_END_TYPE()

        //---

        class FunctionArithmeticShiftRight : public INativeFunction
        {
        RTTI_DECLARE_VIRTUAL_CLASS(FunctionArithmeticShiftRight, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DetermineIntMathType(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                auto baseType = ExtractBaseType(retData.type());
                ASSERT(baseType != BaseType::Invalid && baseType != BaseType::Boolean);

                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, valop::ArithmeticShiftRight(argValues[0].component(i), argValues[1].component(i)));
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionArithmeticShiftRight);
            RTTI_METADATA(FunctionNameMetadata).name("__sar");
        RTTI_END_TYPE();

        //---

    } // shaders
} // rendering