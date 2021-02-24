/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading #]
***/

#pragma once

#include "algorithms.h"

BEGIN_BOOMER_NAMESPACE(base)

//-----------------------------------------------------------------------------

/// Semaphore
class BASE_SYSTEM_API Semaphore : public base::NoCopy
{
public:
    Semaphore(uint32_t initialCount, uint32_t maxCount);
    ~Semaphore();

    //! Release semaphore
    uint32_t release(uint32_t count = 1);

    //! Wait for semaphore, passing 0
    bool wait(uint32_t waitTime = INFINITE_TIME);

    //--

    //! Wait for multiple semaphores, return the index of the signalled semaphore or -1 if timeout happened
    static int WaitMultiple(Semaphore** semaphores, uint32_t numSemaphores, uint32_t waitTime = INFINITE_TIME);

private:
    INLINE Semaphore() {};

    void* m_handle;     // Internal handle
};

//-----------------------------------------------------------------------------

END_BOOMER_NAMESPACE(base)