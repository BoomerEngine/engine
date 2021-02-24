/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types\function #]
***/

#include "build.h"
#include "rttiType.h"
#include "rttiFunction.h"

#include "base/containers/include/stringBuilder.h"

BEGIN_BOOMER_NAMESPACE(base::rtti)

//---

IFunctionStackFrame::~IFunctionStackFrame()
{}

void IFunctionStackFrame::throwException(const char* txt) const
{
#ifdef BUILD_RELEASE
#ifdef PLATFORM_GCC
    fprintf(stderr, "Script exception: %s\n", txt);
#endif
#else
    TRACE_ERROR("Script exception: {}", txt);
    print(TRACE_STREAM_ERROR());
#endif
}

void IFunctionStackFrame::print(IFormatStream& f) const
{
    auto className = this->className();
    auto funcName = functionName();

    if (!funcName)
        f.appendf("UnknownFunction()");
    else if (!className)
        f.appendf("{}()", funcName);
    else
        f.appendf("{}.{}()", className, funcName);

    auto file = sourceFile();
    if (file)
        f.appendf(" at {}({})\n", file, sourceLine());
    else

    if (auto parent  = this->parent())
    {
        f.append("\n");
        parent->print(f);
    }
}

void IFunctionStackFrame::dump()
{
    print(TRACE_STREAM_INFO());
}

//---

IFunctionCodeBlock::~IFunctionCodeBlock()
{}

//---

FunctionParamType::FunctionParamType(Type t, FunctionParamFlags flags /*= FunctionParamFlag::Normal*/)
    : m_type(t)
    , m_flags(flags)
{}

void FunctionParamType::print(IFormatStream& f) const
{
    if (m_type)
    {
        if (m_flags.test(FunctionParamFlag::Const))
            f << "const ";
        f << m_type->name();
        if (m_flags.test(FunctionParamFlag::Ref))
            f << "&";
        if (m_flags.test(FunctionParamFlag::Ptr))
            f << "*";
    }
    else
    {
        f << "null";
    }
}

//---
        
Function::Function(const IType* parent, StringID name, bool isScripted)
    : m_parent(parent)
    , m_name(name)
    , m_functionCode(nullptr)
    , m_functionWrapperPtr(nullptr)
    , m_functionJittedCode(nullptr)
{
    if (isScripted)
        m_flags |= FunctionFlag::Scripted;
}

Function::~Function()
{
    if (m_functionCode)
    {
        m_functionCode->release();
        m_functionCode = nullptr;
    }
}

StringBuf Function::fullName() const
{
    auto className = m_parent ? m_parent->name() : StringID();
    auto funcName = name();

    if (!funcName)
        return StringBuf("UnknownFunction");
    else if (!className)
        return StringBuf(funcName.view());
    else
        return StringBuf(TempString("{}::{}()", className, funcName));
}

void Function::print(IFormatStream& f) const
{
    if (m_flags.test(FunctionFlag::Static))
        f << "static ";

    if (m_returnType.valid())
        f << m_returnType;
    else
        f << "void";
    f << " ";
    f << m_name;
    f << "(";

    for (uint32_t i=0; i<m_paramTypes.size(); ++i)
    {
        if (i > 0)
            f << ", ";
        f << m_paramTypes[i];
    }

    f << ")";

    if (m_flags.test(FunctionFlag::Const))
        f << " const";
}

void Function::calculateDataBlockSize()
{
    m_paramsDataBlockSize = 0;
    m_paramsDataBlockAlignment = 1;

    for (auto& param : m_paramTypes)
    {
        auto typeSize = param.m_type->size();
        auto typeAlignment = param.m_type->alignment();
        param.m_offset = Align(m_paramsDataBlockSize, typeAlignment);
        m_paramsDataBlockAlignment = std::max(m_paramsDataBlockAlignment, typeAlignment);
        m_paramsDataBlockSize = param.m_offset + typeSize;
    }
}

void Function::setupNative(const FunctionParamType& retType, const Array<FunctionParamType>& argTypes, const FunctionPointer& functionPointer, TFunctionWrapperPtr functionWrapper, bool isConst, bool isStatic)
{
    ASSERT(functionPointer);
    ASSERT(functionWrapper != nullptr)

    m_returnType = retType;
    m_paramTypes = argTypes;
    m_functionPtr = functionPointer;
    m_functionWrapperPtr = functionWrapper;
    m_flags = FunctionFlag::Native;
    if (isConst) m_flags |= FunctionFlag::Const;
    if (isStatic) m_flags |= FunctionFlag::Static;

    calculateDataBlockSize();
}

void Function::setupScripted(const FunctionParamType& retType, const Array<FunctionParamType>& argTypes, IFunctionCodeBlock* scriptedCode, bool isConst, bool isStatic)
{
    ASSERT(scriptedCode != nullptr);
    ASSERT(!m_functionPtr);
    ASSERT(!m_functionWrapperPtr);
    ASSERT(scripted());

    m_returnType = retType;
    m_paramTypes = argTypes;
    m_functionCode = scriptedCode;
    m_flags = FunctionFlag::Scripted;
    if (isConst) m_flags |= FunctionFlag::Const;
    if (isStatic) m_flags |= FunctionFlag::Static;

    calculateDataBlockSize();
}

bool Function::bindJITFunction(uint64_t codeHash, TFunctionJittedWrapperPtr jitPtr)
{
    // function is native, lol
    if (m_functionWrapperPtr)
    {
        TRACE_ERROR("Unable to bind JIT to native function '{}'", fullName());
        return false;
    }

    // function is not scripted
    if (!m_functionCode)
    {
        TRACE_ERROR("Unable to bind JIT to non-script function '{}'", fullName());
        return false;
    }

    // check code version
    if (m_functionCode->codeHash() != codeHash)
    {
        TRACE_ERROR("JIT code for function '{}' does not match script code", fullName());
        return false;
    }

    // bind
    m_functionJittedCode = jitPtr;
    return true;
}

void Function::cleanupScripted()
{
    ASSERT(scripted());

    m_returnType = FunctionParamType();
    m_paramTypes.clear();

    if (m_functionCode)
    {
        m_functionCode->release();
        m_functionCode = nullptr;
    }

    m_flags -= FunctionFlag::Static;
    m_flags -= FunctionFlag::Const;

    m_paramsDataBlockSize = 0;
    m_paramsDataBlockAlignment = 1;

    m_functionJittedCode = nullptr;
}

//---

// temporary stack frame for JITed function
// NOTE: this is so we can have a better scritped callstack, the JIT functions don't use stack frames
struct JITStackFrame : public IFunctionStackFrame
{
public:
    INLINE JITStackFrame(const IFunctionStackFrame* parentFrame, const Function* func)
        : m_parentFrame(parentFrame)
        , m_func(func)
    {}

    virtual const IFunctionStackFrame* parent() const override final
    {
        return m_parentFrame;
    }

    virtual StringID functionName() const override final
    {
        return m_func->name();
    }

    virtual StringID className() const override final
    {
        return m_func->parent() ? m_func->parent()->name() : StringID::EMPTY();
    }

    virtual StringBuf sourceFile() const override final
    {
        return StringBuf("JIT");
    }

    virtual uint32_t sourceLine() const override final
    {
        return 0;
    }

private:
    const IFunctionStackFrame* m_parentFrame;
    const Function* m_func;
};

void Function::run(const IFunctionStackFrame* parentFrame, void* context, const FunctionCallingParams& params) const
{
    if (m_functionWrapperPtr)
    {
        (*m_functionWrapperPtr)(context, m_functionPtr, params);
    }
    else if (m_functionJittedCode)
    {
        JITStackFrame jitFrame(parentFrame, this);
        (*m_functionJittedCode)(context, &jitFrame, &params);
    }
    else if (m_functionCode)
    {
        m_functionCode->run(parentFrame, context, params);
    }
}

END_BOOMER_NAMESPACE(base::rtti)