/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#include "build.h"
#include "output.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu)

//--

RTTI_BEGIN_TYPE_ENUM(OutputClass);
    RTTI_ENUM_OPTION(Offscreen);
    RTTI_ENUM_OPTION(Window);
    RTTI_ENUM_OPTION(HMD);
RTTI_END_TYPE();

///--

INativeWindowCallback::~INativeWindowCallback()
{}

///--

INativeWindowInterface::~INativeWindowInterface()
{}

///--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IOutputObject);
RTTI_END_TYPE();

IOutputObject::IOutputObject(ObjectID id, IDeviceObjectHandler* impl, bool flipped, INativeWindowInterface* window)
    : IDeviceObject(id, impl)
    , m_flipped(flipped)
    , m_window(window)
{}

///--

END_BOOMER_NAMESPACE_EX(gpu)
