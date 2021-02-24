/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input #]
***/

#include "build.h"
#include "inputContextNull.h"

BEGIN_BOOMER_NAMESPACE(base::input)

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(ContextNull);
RTTI_END_TYPE();

ContextNull::ContextNull(uint64_t nativeWindow, uint64_t nativeDisplay)
{
}

void ContextNull::processState()
{}

void ContextNull::resetInput()
{}

void ContextNull::processMessage(const void* msg)
{}

//--

END_BOOMER_NAMESPACE(base::input)