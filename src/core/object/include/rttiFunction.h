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

BEGIN_BOOMER_NAMESPACE()

//---

class FunctionBuilder;

// abstract function call stack frame
class CORE_OBJECT_API IFunctionStackFrame : public NoCopy
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
struct CORE_OBJECT_API FunctionParamType
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

class FunctionSignatureBuilder;

// function signature - return value and parameters
class CORE_OBJECT_API FunctionSignature
{
public:
    FunctionSignature();
    FunctionSignature(const FunctionSignature& other) = default;
    FunctionSignature& operator=(const FunctionSignature& other) = default;
    FunctionSignature(const FunctionParamType& retType, const Array<FunctionParamType>& argTypes, bool isConst, bool isStatic);

    //--

    /// get type returned by the function
    INLINE FunctionParamType returnType() const { return m_returnType; }

    /// get number of function parameters
    INLINE uint32_t numParams() const { return m_paramTypes.size(); }

    /// get function parameter type
    INLINE const FunctionParamType* params() const { return m_paramTypes.typedData(); }

    /// NOTE: non static functions will require object pointer
    INLINE bool isStatic() const { return m_static; }

    /// is this a const function ?
    INLINE bool isConst() const { return m_const; }

    //--

    /// get function signature as string
    void print(IFormatStream& f) const;

    //--

private:
    FunctionParamType m_returnType; // type returned by the function

    typedef Array<FunctionParamType> TParamTypes;
    TParamTypes m_paramTypes; // type of parameters accepted by function

    bool m_static = false;
    bool m_const = false;

    friend class FunctionSignatureBuilder;
};

//---

// basic function
class CORE_OBJECT_API IFunction : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_RTTI)

public:
    IFunction(const IType* parent, StringID name, const FunctionSignature& signature);

    /// get the name of the property
    INLINE StringID name() const { return m_name; }

    /// get the owner of this property
    INLINE const IType* parent() const { return m_parent; }

    /// get function calling signature
    INLINE const FunctionSignature& signature() const { return m_signature; }

private:
    const IType* m_parent; // class
    StringID m_name; // name of the function

    FunctionSignature m_signature;
};

//---

// callable object's native function, usable in the visual scripting system
class CORE_OBJECT_API NativeFunction : public IFunction
{
    RTTI_DECLARE_POOL(POOL_RTTI)

public:
    NativeFunction(const IType* parent, StringID name, const FunctionSignature& signature, FunctionPointer nativePtr, TFunctionWrapperPtr wrappedPtr);

    ///--

    /// get the native function pointer (for direct calls)
    INLINE const FunctionPointer& nativeFunctionPointer() const { return m_nativePtr; }

    /// get universal function wrapper
    INLINE TFunctionWrapperPtr nativeFunctionWrapper() const { return m_wrappedPtr; }

    //--

    /// run function, works for both native and scripted functions, totally transparent, can be used by scripting languages when running the function directly is out of question
    void run(const IFunctionStackFrame* parentFrame, void* context, const FunctionCallingParams& params) const;

    //--

private:
    FunctionPointer m_nativePtr = nullptr;
    TFunctionWrapperPtr m_wrappedPtr = nullptr;
};

//---

// mono callable function, contains specialized Mono shim that calls the native function pointer
class CORE_OBJECT_API MonoFunction : public IFunction
{
    RTTI_DECLARE_POOL(POOL_RTTI)

public:
    MonoFunction(const IType* parent, StringID name, const FunctionSignature& signature, TFunctionMonoWrapperPtr ptr);

    ///--

    /// get the callable function address to be exported to mono
    INLINE TFunctionMonoWrapperPtr monoFunctionWrapper() const { return m_monoWrappedPtr; }

    //--

private:
    TFunctionMonoWrapperPtr m_monoWrappedPtr = nullptr;
};

//---

END_BOOMER_NAMESPACE()

//--