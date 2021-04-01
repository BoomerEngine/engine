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

#include "core/containers/include/stringBuilder.h"

BEGIN_BOOMER_NAMESPACE()

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

FunctionSignature::FunctionSignature()
{}

FunctionSignature::FunctionSignature(const FunctionParamType & retType, const Array<FunctionParamType>&argTypes, bool isConst, bool isStatic)
    : m_returnType(retType)
    , m_paramTypes(argTypes)
    , m_const(isConst)
    , m_static(isStatic)
{}

void FunctionSignature::print(IFormatStream& f) const
{
    if (m_static)
        f << "static ";

    if (m_returnType.valid())
        f << m_returnType;
    else
        f << "void";
    f << " ";
    f << "(";

    for (uint32_t i = 0; i < m_paramTypes.size(); ++i)
    {
        if (i > 0)
            f << ", ";
        f << m_paramTypes[i];
    }

    f << ")";

    if (m_const)
        f << " const";
}

//---

IFunction::IFunction(const IType* parent, StringID name, const FunctionSignature& signature)
    : m_parent(parent)
    , m_name(name)
    , m_signature(signature)
{}

//---

NativeFunction::NativeFunction(const IType* parent, StringID name, const FunctionSignature& signature, FunctionPointer nativePtr, TFunctionWrapperPtr wrappedPtr)
    : IFunction(parent, name, signature)
    , m_nativePtr(nativePtr)
    , m_wrappedPtr(wrappedPtr)
{}

void NativeFunction::run(const IFunctionStackFrame* parentFrame, void* context, const FunctionCallingParams& params) const
{
    (*m_wrappedPtr)(context, params);
}

//--

MonoFunction::MonoFunction(const IType* parent, StringID name, const FunctionSignature& signature, TFunctionMonoWrapperPtr ptr)
    : IFunction(parent, name, signature)
    , m_monoWrappedPtr(ptr)
{
}

//--

END_BOOMER_NAMESPACE()
