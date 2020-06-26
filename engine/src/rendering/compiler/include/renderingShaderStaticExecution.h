/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
***/

#pragma once

#include "base/memory/include/linearAllocator.h"

#include "renderingShaderDataType.h"
#include "renderingShaderDataValue.h"
#include "renderingShaderProgramInstance.h"

namespace rendering
{
    namespace compiler
    {
        ///---

        enum class ExecutionResult : uint8_t
        {
            Finished,
            Return,
            Break,
            Continue,
            Discard,
            Error,
        };

        ///---

        /// execution value
        class ExecutionValue
        {
        public:
            ExecutionValue(); // invalid
            ExecutionValue(const DataType& type); // undefined value of given type
            ExecutionValue(const DataType& type, const DataValue& value); // const value of given type
            ExecutionValue(const DataValueRef& refx); // reference to memory of given type

            // get type of the value
            INLINE const DataType& type() const { return m_type; }

            // is this a reference ?
            INLINE bool isReference() const { return m_ref.valid(); }

            // get number of components
            INLINE uint32_t size() const { return m_val.size(); }

            //---

            // get array sub-element
            ExecutionValue arrayElement(uint32_t index) const;

            // get the compound element reference
            ExecutionValue compositeElement(const base::StringID name) const;

            //---

            // get component count
            uint32_t numComponents() const;

            // get component value
            DataValueComponent component(uint32_t index) const;

            // set component value
            void component(uint32_t index, const DataValueComponent& newValue);

            // get matrix component (safe), returns undefined when out of range
            DataValueComponent matrixComponent(uint32_t numCols, uint32_t numRows, uint32_t col, uint32_t row) const;

            // set matrix component, does not set if out of range
            void matrixComponent(uint32_t numCols, uint32_t numRows, uint32_t col, uint32_t row, const DataValueComponent& val);

            //---

            // check if all components are defined
            bool isWholeValueDefined() const;

            // load the value
            bool readValue(DataValue& outValue) const;

            // write value to reference
            bool writeValue(const DataValue& newValue) const;

            // write value to reference using given component mask
            bool writeValueWithMask(const DataValue& newValue, const ComponentMask& mask) const;

            // write value to reference
            bool writeValue(const ExecutionValue& newValue) const;

            // write value to reference using given component mask
            bool writeValueWithMask(const ExecutionValue& newValue, const ComponentMask& mask) const;

            //--

            // debug dump
            void print(base::IFormatStream& f) const;

        private:
            DataType m_type; // type of the value
            DataValueRef m_ref; // for references
            DataValue m_val; // for actual values
        };

        ///---

        /// helper class containing execution context
        class ExecutionStack
        {
        public:
            ExecutionStack(base::mem::LinearAllocator& mem, CodeLibrary& code, base::parser::IErrorReporter& err, const Function* function, const ProgramInstance* thisVars);
            ~ExecutionStack();

            //---

            /// get memory allocator
            INLINE base::mem::LinearAllocator& allocator() const { return m_mem; }

            /// code library
            INLINE CodeLibrary& code() const { return m_code; }

            /// get error reporter
            INLINE base::parser::IErrorReporter& errorReporter() const { return m_err; }

            /// get the context of program we are executing, does not have to be defined (for example global functions don't have it)
            INLINE const ProgramInstance* contextThis() const { return m_contextThisVars; }

            /// get he context function, may be NULL when evaluating a context-less shit like constant initialization
            INLINE const Function* contextFunction() const { return m_contextFunction; }

            //---

            /// execute function call on the context, note that we may have some additional predefined parameters
            ExecutionResult callFunction(const base::Array<ExecutionValue>& args, ExecutionValue& result);

            //---

            /// get reference to data of given data parameter
            /// NOTE: may return invalid reference if the parameter does not exist
            bool paramReference(const DataParameter* param, ExecutionValue& outValue);

            //---

            /// store a value for a constant evaluated in current scope
            void storeConstantValue(const DataParameter* param, const ExecutionValue& value);

            //---

            /// error exit
            ExecutionResult returnCode(const base::parser::Location& loc, ExecutionResult error);

        private:
            base::mem::LinearAllocator& m_mem;
            base::parser::IErrorReporter& m_err;
            CodeLibrary& m_code;

            // all evaluated global constants
            struct GlobalConstant { DataType m_type; DataValue* m_value; };
            typedef base::HashMap<const DataParameter*, GlobalConstant> TGlobalEvaluatedConstantsMap;
            mutable TGlobalEvaluatedConstantsMap m_globalConstantsValues;

            //--           

            const ProgramInstance* m_contextThisVars = nullptr;
            const Function* m_contextFunction = nullptr;

            struct ParamMap
            {
                ParamMap();
                ~ParamMap();

                struct Param
                {
                    const DataParameter* m_param;
                    DataValue m_storage;
                    ExecutionValue m_ref; // set up as a reference to the value
                    bool m_valueSet;

                    Param(const DataParameter* param);
                };

                base::HashMap<base::StringID, Param*> m_paramMap;
                base::HashMap<const DataParameter*, Param*> m_localMap;

                //--

                bool paramReference(base::mem::LinearAllocator& mem, const DataParameter* param, ExecutionValue& outValue);
                void storeConstantValue(base::mem::LinearAllocator& mem, const DataParameter* param, const ExecutionValue& value);
            };

            ParamMap m_functionParams;
            ParamMap m_programParams;

            void buildProgramParams();
            void buildFunctionParams();

            //--            

            DataValueRef resolveGlobalConstantValue(const DataParameter* val, base::parser::IErrorReporter& err);
        };

        ///---

        /// shader code execution VM
        extern ExecutionResult ExecuteProgramCode(ExecutionStack& stack, const CodeNode* code, ExecutionValue& result);

        ///---

    } // shader
} // rendering