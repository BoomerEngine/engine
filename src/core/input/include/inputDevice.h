/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input #]
***/

#pragma once

#include "core/math/include/point.h"
#include "core/system/include/timing.h"

BEGIN_BOOMER_NAMESPACE_EX(input)

/// basic concept of input device
class CORE_INPUT_API IDevice
{
    RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IDevice);

public:
    IDevice();
    virtual ~IDevice();

    /// initialize device
    virtual bool initialize(IContext* context) = 0;

    /// reset internal state of this device
    /// this acts as if user release all keys on the device
    virtual void process() = 0;
};

END_BOOMER_NAMESPACE_EX(input)
