/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: public #]
***/

#pragma once

#include "fiberSystem.h"

BEGIN_BOOMER_NAMESPACE_EX(fibers)

///---

/// a fiber synchronizer, contains internal fence
class CORE_FIBERS_API SyncPoint : public NoCopy
{
public:
    SyncPoint(const char* name);
    ~SyncPoint();

    /// wait for previous fence to complete and acquire new fence
    /// NOTE: if we managed to call release() before calling acquire there's no wait
    CAN_YIELD void acquire();

    /// wait for the previous fence to complete but do not acquire new fence
    void release();

private:
    Mutex m_lock;
    const char* m_name;
    WaitCounter m_fence;
    bool m_completed;
};

///---

END_BOOMER_NAMESPACE_EX(fibers)
