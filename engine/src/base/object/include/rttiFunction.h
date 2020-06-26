/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types\function #]
***/

#pragma once

#include "rttiFunctionPointer.h"
#include "rttiTypeRef.h"

namespace base
{
    namespace reflection
    {
        class FunctionBuilder;
    } // reflection

    namespace rtti
    {
        //---

        // modifier
            enum class FunctionParamFlag : uint8_t
        {
            Normal = 0,
            Ref = 1,
            Const = 2,
            ConstRef = 3,
            Ptr = 4,
            ConstPtr = 6,
        };

        typedef DirectFlags<FunctionParamFlag> FunctionParamFlags;

        // function parameter
        struct BASE_OBJECT_API FunctionParamType
        {
            Type m_type;
            FunctionParamFlags m_flags;
            uint16_t m_offset = 0;

            //---

            INLINE FunctionParamType() : m_type(nullptr), m_flags(FunctionParamFlag::Normal) {};
            INLINE FunctionParamType(const FunctionParamType& other) = default;
            INLINE FunctionParamType& operator=(const FunctionParamType& other) = default;
            FunctionParamType(Type, FunctionParamFlags flags = FunctionParamFlag::Normal);

            INLINE bool valid() const { return m_type != nullptr; }

            void print(IFormatStream& f) const;
        };

        //---

        // flags
        enum class FunctionFlag : uint8_t
        {
            Native = FLAG(0), // function has native (C++) implementation
            Const = FLAG(1), // function is guaranteed not to modify object
            Static = FLAG(2), // function is static (no context object required)
            Scripted = FLAG(3), // function is scripted (has script code blob)
        };

        typedef DirectFlags<FunctionFlag> FunctionFlags;

        //---

        // abstract function call stack frame
        class BASE_OBJECT_API IFunctionStackFrame : public base::NoCopy
        {
        public:
            virtual ~IFunctionStackFrame();

            virtual const IFunctionStackFrame* parent() const = 0;

            virtual StringID functionName() const = 0;
            virtual StringID className() const = 0;

            virtual StringBuf sourceFile() const = 0;
            virtual uint32_t sourceLine() const = 0;

            virtual void throwException(const char* txt) const;

            //--

            // print callstack
            void print(IFormatStream& f) const;

            // dump callstack to log (debug)
            void dump();
        };

        //---

        // function code block
        class BASE_OBJECT_API IFunctionCodeBlock : public base::NoCopy
        {
        public:
            virtual ~IFunctionCodeBlock();
            virtual void release() = 0;
            virtual uint64_t codeHash() const = 0;
            virtual void run(const IFunctionStackFrame* parentFrame, void* context, const FunctionCallingParams& params) const = 0;
        };

        //---

        // callable native function
        class BASE_OBJECT_API Function : public base::NoCopy
        {
        public:
            Function(const IType* parent, StringID name, bool isScripted = false);
            virtual ~Function();

            /// get the name of the property
            INLINE StringID name() const { return m_name; }

            /// get the owner of this property
            INLINE const IType* parent() const { return m_parent; }

            /// get type returned by the function
            INLINE FunctionParamType returnType() const { return m_returnType; }

            /// get number of function parameters
            INLINE uint32_t numParams() const { return m_paramTypes.size(); }

            /// get function parameter type
            INLINE const FunctionParamType* params() const { return m_paramTypes.typedData(); }

            /// get function flags
            INLINE FunctionFlags flags() const { return m_flags; }

            /// is this a static function ?
            /// NOTE: non static functions will require object pointer
            INLINE bool isStatic() const { return m_flags.test(FunctionFlag::Static); }

            /// is this a const function ?
            INLINE bool isConst() const { return m_flags.test(FunctionFlag::Const); }

            /// is this a native function
            INLINE bool isNative() const { return m_flags.test(FunctionFlag::Native); }

            /// is this a scripted function
            INLINE bool scripted() const { return m_flags.test(FunctionFlag::Scripted); }

            /// get size of memory needed to pass all function arguments
            INLINE uint32_t paramsDataBlockSize() const { return m_paramsDataBlockSize; }

            /// get size of memory needed to pass all function arguments
            INLINE uint32_t paramsDataBlockAlignment() const { return m_paramsDataBlockAlignment; }

            /// get the native function pointer (for direct calls)
            INLINE const FunctionPointer& nativeFunctionPointer() const { return m_functionPtr; }

            //--

            /// get function full name, usually className and functionName (ie. GameObject.Tick)
            StringBuf fullName() const;

            /// get function signature as string
            void print(IFormatStream& f) const;

            //--

            /// setup as native function
            void setupNative(const FunctionParamType& retType, const Array<FunctionParamType>& argTypes, const rtti::FunctionPointer& functionPointer, TFunctionWrapperPtr functionWrapper, bool isConst, bool isStatic);

            /// setup as scripted function
            void setupScripted(const FunctionParamType& retType, const Array<FunctionParamType>& argTypes, IFunctionCodeBlock* scriptedCode, bool isConst, bool isStatic);

            /// cleanup
            void cleanupScripted();

            //--

            /// bind JIT version of this function
            /// NOTE: can fail if the code used to generate the JITed function does not match our current code
            bool bindJITFunction(uint64_t codeHash, TFunctionJittedWrapperPtr jitPtr);

            //--

            /// run function, works for both native and scripted functions, totally transparent
            void run(const IFunctionStackFrame* parentFrame, void* context, const FunctionCallingParams& params) const;

        private:
            FunctionPointer m_functionPtr; // function pointer (native functions)
            TFunctionWrapperPtr m_functionWrapperPtr; // function wrapper (for calling native fuctions from scripts)
            IFunctionCodeBlock* m_functionCode; // function code (script functions)
            TFunctionJittedWrapperPtr m_functionJittedCode; // in case function was JITTed

            //--

            const IType* m_parent; // class
            StringID m_name; // name of the function

            FunctionFlags m_flags; // function flags

            FunctionParamType m_returnType; // type returned by the function

            typedef Array<FunctionParamType> TParamTypes;
            TParamTypes m_paramTypes; // type of parameters accepted by function

            uint32_t m_paramsDataBlockSize = 0;
            uint32_t m_paramsDataBlockAlignment = 1;

            void calculateDataBlockSize();
        };

    } // rtti
} // base