/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading\winapi #]
***/

#pragma once

#include "thread.h"
#include "atomic.h"

BEGIN_BOOMER_NAMESPACE(base)

namespace prv
{
    //--

    /// WinAPI based thread
    class WinThread : public NoCopy
    {
    public:
        static void Init(void* ptr, const ThreadSetup& setup);
        static void Close(void* ptr);

        //---

        static DWORD __stdcall StaticEntry(LPVOID lpThreadParameter);
    };

    //--

} // prv

END_BOOMER_NAMESPACE(base)