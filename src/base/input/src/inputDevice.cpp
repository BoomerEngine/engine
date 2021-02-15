/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input #]
***/

#include "build.h"
#include "inputDevice.h"

namespace base
{
    namespace input
    {
        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IDevice);
        RTTI_END_TYPE();

        IDevice::IDevice()
        {}

        IDevice::~IDevice()
        {}

    } // input
} // base
