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

namespace rendering
{
    namespace compiler
    {

        //---

        class FunctionCastToBool : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionCastToBool, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                if (numArgs != 1)
                    return DataType();

                return GetCastedType(typeLibrary, argTypes[0], BaseType::Boolean);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i=0; i<retData.size(); ++i)
                    retData.component(i, valop::ToBool(argValues[0].component(i)));
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionCastToBool);
        RTTI_METADATA(FunctionNameMetadata).name("__castToBool");
        RTTI_END_TYPE();

        //---

        class FunctionCastToInt : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionCastToInt, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                if (numArgs != 1)
                    return DataType();

                return GetCastedType(typeLibrary, argTypes[0], BaseType::Int);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i=0; i<retData.size(); ++i)
                    retData.component(i, valop::ToInt(argValues[0].component(i)));
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionCastToInt);
        RTTI_METADATA(FunctionNameMetadata).name("__castToInt");
        RTTI_END_TYPE();

        //---

        class FunctionCastToUint : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionCastToUint, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                if (numArgs != 1)
                    return DataType();

                return GetCastedType(typeLibrary, argTypes[0], BaseType::Uint);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i=0; i<retData.size(); ++i)
                    retData.component(i, valop::ToUint(argValues[0].component(i)));
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionCastToUint);
        RTTI_METADATA(FunctionNameMetadata).name("__castToUint");
        RTTI_END_TYPE();

        //---

        class FunctionCastToFloat : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionCastToFloat, INativeFunction);

        public:
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                if (numArgs != 1)
                    return DataType();

                return GetCastedType(typeLibrary, argTypes[0], BaseType::Float);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i=0; i<retData.size(); ++i)
                    retData.component(i, valop::ToFloat(argValues[0].component(i)));
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionCastToFloat);
        RTTI_METADATA(FunctionNameMetadata).name("__castToFloat");
        RTTI_END_TYPE();

        //---   

    } // shader 
} // rendering