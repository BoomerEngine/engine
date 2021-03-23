/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input #]
***/

#include "build.h"
#include "inputContextNull.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(InputContextNull);
RTTI_END_TYPE();

InputContextNull::InputContextNull(uint64_t nativeWindow, uint64_t nativeDisplay)
{
}

void InputContextNull::processState()
{}

void InputContextNull::resetInput()
{}

void InputContextNull::processMessage(const void* msg)
{}

//--

END_BOOMER_NAMESPACE()
