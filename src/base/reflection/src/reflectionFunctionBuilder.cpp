/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: reflection #]
***/

#include "build.h"
#include "reflectionFunctionBuilder.h"

#include "base/object/include/rttiFunction.h"

BEGIN_BOOMER_NAMESPACE(base::reflection)

FunctionBuilder::FunctionBuilder(const char* name)
    : m_name(name)
    , m_isConst(false)
    , m_isStatic(false)
    , m_functionWrapperPtr(nullptr)
{
}

FunctionBuilder::~FunctionBuilder()
{
}

void FunctionBuilder::submit(rtti::IClassType* targetClass)
{
    ASSERT(m_functionPtr);
    ASSERT(m_functionWrapperPtr != nullptr);

    auto func = new rtti::Function(targetClass, StringID(m_name.c_str()));
    func->setupNative(m_returnType, m_paramTypes, m_functionPtr, m_functionWrapperPtr, m_isConst, m_isStatic);

    if (targetClass == nullptr)
        RTTI::GetInstance().registerGlobalFunction(func);
    else
        targetClass->addFunction(func);
}

END_BOOMER_NAMESPACE(base::reflection)
