/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading #]
***/

#pragma once

#include "algorithms.h"

namespace base
{
    //-----------------------------------------------------------------------------

    /// Critical section
    class BASE_SYSTEM_API Mutex : public base::NoCopy
    {
    public:
        Mutex();
        ~Mutex();

        //! Lock section
        void acquire();

        //! Releases the lock on the critical section
        void release();

        //! Set spin count for critical section
        void spinCount(uint32_t spinCount);

    private:
        uint8_t   m_data[64];
    };

    //-----------------------------------------------------------------------------

} // base
