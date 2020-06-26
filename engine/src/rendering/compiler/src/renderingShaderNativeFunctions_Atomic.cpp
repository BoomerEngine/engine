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
        //---


        class IFunctionAtomicBase : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(IFunctionAtomicBase, INativeFunction);

        public:
            IFunctionAtomicBase(uint8_t numArgs, bool supportsFloat)
                : m_numArgs(numArgs)
                , m_supportsFloat(supportsFloat)
            {}

            bool typeSupportsAtomic(const DataType& type) const
            {             

                if (type.baseType() == BaseType::Uint)
                    return true;
                if (type.baseType() == BaseType::Int)
                    return true;
                if (type.baseType() == BaseType::Float)
                    return m_supportsFloat;

                return false;
            }

            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                // check argument count
                if (numArgs < m_numArgs)
                {
                    err.reportError(loc, base::TempString("Invalid number of function arguments, this function requires {} arguments", m_numArgs));
                    return DataType();
                }

                // must be reference
                if (!argTypes[0].isReference())
                {
                    err.reportError(loc, base::TempString("Atomic functions require '{}' to be a reference to memory location/texel", argTypes[0]));
                    return DataType();
                }

                // must support atomic
                if (!argTypes[0].isAtomic())
                {
                    err.reportError(loc, base::TempString("Type '{}' does not support atomic operations", argTypes[0]));
                    return DataType();
                }

                // we can work only with the buffer
                if (!typeSupportsAtomic(argTypes[0]))
                {
                    err.reportError(loc, base::TempString("Type '{}' does not support atomic operations", argTypes[0]));
                    return DataType();
                }

                auto valueType = argTypes[0].unmakeAtomic();
                if (m_numArgs >= 2)
                    argTypes[1] = valueType.unmakePointer().unmakeAtomic();
                if (m_numArgs >= 3)
                    argTypes[2] = valueType.unmakePointer().unmakeAtomic();

                // determine the output type based on the image format
                return valueType;
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                for (uint32_t i = 0; i < retData.size(); ++i)
                    retData.component(i, DataValueComponent());
            }

        private:
            uint8_t m_numArgs;
            bool m_supportsFloat;
        };

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IFunctionAtomicBase);
        RTTI_END_TYPE();

        //---

        class FunctionAtomicIncrement : public IFunctionAtomicBase
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionAtomicIncrement, IFunctionAtomicBase);

        public:
            FunctionAtomicIncrement()
                    : IFunctionAtomicBase(1, false)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionAtomicIncrement);
            RTTI_METADATA(FunctionNameMetadata).name("atomicIncrement");
        RTTI_END_TYPE();

        //---

        class FunctionAtomicDecrement : public IFunctionAtomicBase
        {
        RTTI_DECLARE_VIRTUAL_CLASS(FunctionAtomicDecrement, IFunctionAtomicBase);

        public:
            FunctionAtomicDecrement()
                    : IFunctionAtomicBase(1, false)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionAtomicDecrement);
            RTTI_METADATA(FunctionNameMetadata).name("atomicDecrement");
        RTTI_END_TYPE();

        //---

        class FunctionAtomicAdd : public IFunctionAtomicBase
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionAtomicAdd, IFunctionAtomicBase);

        public:
            FunctionAtomicAdd()
                : IFunctionAtomicBase(2, false)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionAtomicAdd);
            RTTI_METADATA(FunctionNameMetadata).name("atomicAdd");
        RTTI_END_TYPE();

        //---

        class FunctionAtomicSubtract : public IFunctionAtomicBase
        {
        RTTI_DECLARE_VIRTUAL_CLASS(FunctionAtomicSubtract, IFunctionAtomicBase);

        public:
            FunctionAtomicSubtract()
                    : IFunctionAtomicBase(2, false)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionAtomicSubtract);
            RTTI_METADATA(FunctionNameMetadata).name("atomicSubtract");
        RTTI_END_TYPE();

        //---

        class FunctionAtomicMin : public IFunctionAtomicBase
        {
        RTTI_DECLARE_VIRTUAL_CLASS(FunctionAtomicMin, IFunctionAtomicBase);

        public:
            FunctionAtomicMin()
                    : IFunctionAtomicBase(2, true)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionAtomicMin);
            RTTI_METADATA(FunctionNameMetadata).name("atomicMin");
        RTTI_END_TYPE();

        //---

        class FunctionAtomicMax : public IFunctionAtomicBase
        {
        RTTI_DECLARE_VIRTUAL_CLASS(FunctionAtomicMax, IFunctionAtomicBase);

        public:
            FunctionAtomicMax()
                    : IFunctionAtomicBase(2, true)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionAtomicMax);
            RTTI_METADATA(FunctionNameMetadata).name("atomicMax");
        RTTI_END_TYPE();

        //---

        class FunctionAtomicAnd : public IFunctionAtomicBase
        {
        RTTI_DECLARE_VIRTUAL_CLASS(FunctionAtomicAnd, IFunctionAtomicBase);

        public:
            FunctionAtomicAnd()
                    : IFunctionAtomicBase(2, false)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionAtomicAnd);
            RTTI_METADATA(FunctionNameMetadata).name("atomicAnd");
        RTTI_END_TYPE();

        //---

        class FunctionAtomicOr : public IFunctionAtomicBase
        {
        RTTI_DECLARE_VIRTUAL_CLASS(FunctionAtomicOr, IFunctionAtomicBase);

        public:
            FunctionAtomicOr()
                    : IFunctionAtomicBase(2, false)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionAtomicOr);
            RTTI_METADATA(FunctionNameMetadata).name("atomicOr");
        RTTI_END_TYPE();

        //---

        class FunctionAtomicXor : public IFunctionAtomicBase
        {
        RTTI_DECLARE_VIRTUAL_CLASS(FunctionAtomicXor, IFunctionAtomicBase);

        public:
            FunctionAtomicXor()
                    : IFunctionAtomicBase(2, false)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionAtomicXor);
            RTTI_METADATA(FunctionNameMetadata).name("atomicXor");
        RTTI_END_TYPE();

        //---

        class FunctionAtomicExchange : public IFunctionAtomicBase
        {
        RTTI_DECLARE_VIRTUAL_CLASS(FunctionAtomicExchange, IFunctionAtomicBase);

        public:
            FunctionAtomicExchange()
                    : IFunctionAtomicBase(2, true)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionAtomicExchange);
            RTTI_METADATA(FunctionNameMetadata).name("atomicExchange");
        RTTI_END_TYPE();

        //---

        class FunctionAtomicCompareSwap : public IFunctionAtomicBase
        {
        RTTI_DECLARE_VIRTUAL_CLASS(FunctionAtomicCompareSwap, IFunctionAtomicBase);

        public:
            FunctionAtomicCompareSwap()
                    : IFunctionAtomicBase(3, true)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionAtomicCompareSwap);
            RTTI_METADATA(FunctionNameMetadata).name("atomicCompSwap");
        RTTI_END_TYPE();

        //---

        class FunctionMemoryBarrier : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionMemoryBarrier, INativeFunction);

        public:
            FunctionMemoryBarrier()
            {}

            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DataType(BaseType::Void);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionMemoryBarrier);
        RTTI_METADATA(FunctionNameMetadata).name("memoryBarrier");
        RTTI_END_TYPE();

        //---

        class FunctionBarrier : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionBarrier, INativeFunction);

        public:
            FunctionBarrier()
            {}

            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return DataType(BaseType::Void);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionBarrier);
            RTTI_METADATA(FunctionNameMetadata).name("barrier");
        RTTI_END_TYPE();

        //---

        class FunctionMemoryBarrierShared : public FunctionMemoryBarrier
        {
        RTTI_DECLARE_VIRTUAL_CLASS(FunctionMemoryBarrierShared, FunctionMemoryBarrier);
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionMemoryBarrierShared);
            RTTI_METADATA(FunctionNameMetadata).name("memoryBarrierShared");
        RTTI_END_TYPE();

        //---

        class FunctionGroupMemoryBarrier : public FunctionMemoryBarrier
        {
        RTTI_DECLARE_VIRTUAL_CLASS(FunctionGroupMemoryBarrier, FunctionMemoryBarrier);
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionGroupMemoryBarrier);
            RTTI_METADATA(FunctionNameMetadata).name("groupMemoryBarrier");
        RTTI_END_TYPE();

        //---

    } // shader
} // rendering