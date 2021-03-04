/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: runtime #]
***/

#pragma once

#include "core/containers/include/inplaceArray.h"

BEGIN_BOOMER_NAMESPACE_EX(script)

///---

struct StubFunction;
struct StubClass;
struct StubTypeDecl;
struct StubTypeRef;
struct StubProperty;
struct StubEnum;
class StackFrame;

//---

struct FunctionCodeBreakpointPlacement
{
    uint32_t sourceLine = 0;
    uint32_t codeOffset = 0;
};

//---

struct FunctionLocalVariable
{
    StringID name; // can be nameless
    uint32_t offset = 0;
    Type type = nullptr;
};

//---

// function call param mode
enum class FunctionParamMode : uint8_t
{
    None = 0, // no param, end of list
    Ref = 1, // eval as reference, pass as reference
    SimpleValue = 2, // eval as simple value
    TypedValue = 3, // eval as typed value (type taken from function signature)
};

struct FunctionCallingEncoding
{
    uint64_t value = 0;

    INLINE FunctionParamMode decode()
    {
        auto ret = value & 0xF;
        value >>= 4;
        return (FunctionParamMode)ret;
    }

    INLINE void encode(FunctionParamMode mode)
    {
        value <<= 4;
        value |= ((uint64_t)mode) & 0xF;
    }
};

//---

/// script -> engine stub mapper, usually provided by the script loader
class CORE_SCRIPT_API IFunctionCodeStubResolver : public NoCopy
{
public:
    virtual ~IFunctionCodeStubResolver();

    virtual Type resolveType(const StubTypeDecl* stub) = 0;
    virtual ClassType resolveClass(const StubClass* stub) = 0;
    virtual const EnumType* resolveEnum(const StubEnum* stub) = 0;
    virtual const Property* resolveProperty(const StubProperty* prop) = 0;
    virtual const Function* resolveFunction(const StubFunction* func) = 0;
};

//---

/// unpacked script code for function
class CORE_SCRIPT_API FunctionCodeBlock : public IFunctionCodeBlock
{
public:
    FunctionCodeBlock();
    ~FunctionCodeBlock();

    /// name of the function
    INLINE StringID name() const { return m_name; }

    /// name of the class (if the function is from class)
    INLINE StringID className() const { return m_className; }

    /// get the name of the source file this function was defined at
    /// NOTE: great care is taken so this string is not duplicated between functions
    INLINE const StringBuf& sourceFileName() const { return m_sourceFileName; }

    /// get line this function was defined at
    INLINE uint32_t sourceFileLine() const { return m_sourceFileLine; }

    /// get code buffer for execution
    INLINE const uint8_t* code() const { return m_code.data(); }

    /// get end of code buffer for execution
    INLINE const uint8_t* codeEnd() const { return code() + m_code.size(); }

    /// get size of the local storage for the function
    INLINE const  uint32_t localStorageSize() const { return m_localStorageSize; }

    /// get ID of the compiled code
    virtual uint64_t codeHash() const override final { return m_codeHash; }


    //--

    // disable all breakpoints from function
    void disableAllBreakpoints();

    // check if breakpoint of given line is enabled
    bool checkBreakpoint(uint32_t line) const;

    // toggle breakpoint on given line, assumes we are in the source file of the function, returns true if breakpoint was found
    bool toggleBreakpoint(uint32_t line, bool status);

    // collect all breakpoints in this function (returns line numbers only)
    void collectActiveBreakpoints(Array<uint32_t>& outLines) const;

    //--

    // find code line for given code pointer
    bool findSourceCodeLine(const uint8_t* codePtr, uint32_t& outLine) const;

    //--

    // run this code, creates internal stack frame
    virtual void run(const IFunctionStackFrame* parent, void* context, const FunctionCallingParams& params) const override final;

    //--

    // build function from compiled opcodes
    static FunctionCodeBlock* Create(const StubFunction* func, const StubClass* funcClass, IFunctionCodeStubResolver& stubResolver);

protected:
    struct Cleanup
    {
        uint32_t offset = 0;
        Type type = nullptr;
    };

    uint32_t m_localStorageSize = 0;
    uint32_t m_localStorageAlignment = 1;

    //--

    StringID m_name;
    StringID m_className;
    StringBuf m_sourceFileName;
    uint32_t m_sourceFileLine = 0;

    Buffer m_code;
    uint64_t m_codeHash;

    //---

    Array<FunctionCodeBreakpointPlacement> m_breakpoints;
    Array<FunctionLocalVariable> m_locals;

    //--

    void findBreakpoints();

    virtual void release() override final;
};

///---

END_BOOMER_NAMESPACE_EX(script)
