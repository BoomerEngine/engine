/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: runtime #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(script)

//---

/// execution context of script function
class CORE_SCRIPT_API StackFrame : public rtti::IFunctionStackFrame
{
public:
    StackFrame(const StackFrame* parent, void* context, const FunctionCodeBlock* code, const rtti::FunctionCallingParams* params, void* localStorage);
    ~StackFrame();

    //--

    // get the parent stack frame, we can build the callstack using this
    INLINE const StackFrame* parent() const override final { return m_parent; }

    // get current code pointer in executed function
    INLINE const uint8_t*& codePtr() { return m_codePtr; }

    // get end of function code
    INLINE const uint8_t* codeEndPtr() const { return m_codeEndPtr; }

    // get the context object the stack frame was initially created
    INLINE void* initialContext() const { return m_contextObject; }

    // get current context object for stack
    INLINE void*& activeContext() { return m_activeContextObject; }

    // get the function we are executing
    // NOTE: function is immutable for execution
    INLINE const FunctionCodeBlock* function() const { return m_codeBlock; }

    // get function locals we are running with
    INLINE uint8_t* locals() const { return m_locals; }

    // get function input arguments we are running with
    INLINE const rtti::FunctionCallingParams* params() const { return m_params; }

    //--

    /// rtti::IFunctionStackFrame
    virtual StringID functionName() const override final;
    virtual StringID className() const override final;
    virtual StringBuf sourceFile() const override final;
    virtual uint32_t sourceLine() const override final;

    //--

    // run execution loop
    void run() noexcept;

    //--

private:
    // hot set:
    const uint8_t* m_codePtr; // current code pointer
    const uint8_t* m_codeEndPtr; // current code pointer
    uint8_t* m_locals; // buffer for local function data (allocated based on the function code requirements)

    // -- cache line

    void* m_activeContextObject; // context object for execution, used in function calling and context var access
    const rtti::FunctionCallingParams* m_params; // function call parameters

    // cold set
    const StackFrame* m_parent; // parent stack frame
    void* m_contextObject; // context object for execution, class less
    const FunctionCodeBlock* m_codeBlock; // function code we are executing
};

//--

END_BOOMER_NAMESPACE_EX(script)
