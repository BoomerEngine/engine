/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: public #]
***/

#pragma once

#include "fiberSystem.h"
#include "core/containers/include/inplaceArray.h"

BEGIN_BOOMER_NAMESPACE_EX(fibers)

//---

/// a collector of fences that we want to wait on, implements simple helper for WaitForMultipleObjects behavior
class CORE_FIBERS_API WaitList : public NoCopy
{
public:
	WaitList();
    ~WaitList();

    /// wait for all fences to complete, handles fences added in the mean time
    CAN_YIELD void sync();

    /// push a fence to wait for
    void pushFence(WaitCounter fence);

private:
    SpinLock m_lock;
    InplaceArray<WaitCounter, 16> m_fences;
};

//---

END_BOOMER_NAMESPACE_EX(fibers)
