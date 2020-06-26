/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#include "build.h"
#include "renderingOutput.h"

namespace rendering
{
    //--

    RTTI_BEGIN_TYPE_ENUM(DriverOutputClass);
        RTTI_ENUM_OPTION(Offscreen);
        RTTI_ENUM_OPTION(NativeWindow);
        RTTI_ENUM_OPTION(Fullscreen);
        RTTI_ENUM_OPTION(HMD);
    RTTI_END_TYPE();

    ///--

    IDriverNativeWindowCallback::~IDriverNativeWindowCallback()
    {}

    ///--

    IDriverNativeWindowInterface::~IDriverNativeWindowInterface()
    {}

    ///--
    

} // rendering
