/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading #]
***/

#pragma once

#include "algorithms.h"
#include "atomic.h"

BEGIN_BOOMER_NAMESPACE()

//-----------------------------------------------------------------------------

/// Simple spin lock
/// NOTE: spin lock can't be acquired between fibers
class CORE_SYSTEM_API SpinLock : public NoCopy
{
public:
    void acquire();
    void release();

private:
    std::atomic<uint32_t> owner = 0;

    static uint32_t GetThreadID();
    static void InternalYield();
};

//-----------------------------------------------------------------------------

END_BOOMER_NAMESPACE()
