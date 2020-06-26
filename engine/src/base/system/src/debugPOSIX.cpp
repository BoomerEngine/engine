/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\debug #]
* [#platform: posix #]
***/

#include "build.h"
#include "debug.h"
#include <unistd.h>
#include <sys/signal.h>

namespace base
{
    namespace debug
    {
            
        void Break()
        {
            raise(SIGTRAP);
        }

        bool GrabCallstack(uint32_t skipInitialFunctions, const void* exptr, Callstack& outCallstack)
        {
            return false;
        }

        bool TranslateSymbolName(uint64_t functionAddress, SymbolName& outSymbolName, SymbolName& outFileName)
        {
            return false;
        }

    } // debug
} // base                        needsFirstJob = false;


