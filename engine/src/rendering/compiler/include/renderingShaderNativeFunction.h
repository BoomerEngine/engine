/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
***/

#pragma once

#include "base/object/include/rttiMetadata.h"
#include "renderingShaderStaticExecution.h"

namespace rendering
{
    namespace compiler
    {
        ///---

        struct DataType;
        struct DataValue;

        class ExecutionValue;
        class TypeLibrary;

        ///---

        /// metadata for RTTI with the function name
        class RENDERING_COMPILER_API FunctionNameMetadata : public base::rtti::IMetadata
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionNameMetadata, base::rtti::IMetadata);

        public:
            FunctionNameMetadata();

            INLINE const FunctionNameMetadata& name(const char* name)
            {
                m_name = name;
                return *this;
            }

            INLINE const char* name() const
            {
                return m_name;
            }

        private:
            const char* m_name;
        };

        ///---

        /// a native (implemented in c++) function for shader language
        class RENDERING_COMPILER_API INativeFunction : public base::NoCopy
        {
            RTTI_DECLARE_POOL(POOL_SHADER_COMPILATION)
            RTTI_DECLARE_VIRTUAL_ROOT_CLASS(INativeFunction);

        public:
            INativeFunction();
            virtual ~INativeFunction();

            /// hack mode - expand texture to texture + sampler
            virtual bool expandCombinedSampler() const { return false; }

            /// determine if this function should mutate into something else (simpler or specialized version for example)
            /// this is mostly used for various flavors of the multiplication function but was made generic
            virtual const INativeFunction* mutateFunction(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const;

            /// determine output type for function based based on the input parameters, can fail 
            /// NOTE: the function may change the input types (and if so, the casting nodes are inserted)
            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const = 0;

            /// partial evaluation
            virtual bool partialEvaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const { return false; };

            /// evaluate function value for the purpose of constant folding
            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const {};

            //--

            // find native function by name
            static const INativeFunction* FindFunctionByName(const char* name);
        };

        //---

    } // shader
} // rendering
