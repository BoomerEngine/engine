/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\process\winapi #]
* [#platform: windows #]
***/

#pragma once

#include "process.h"
#include "core/system/include/atomic.h"

#include <Windows.h>

BEGIN_BOOMER_NAMESPACE_EX(process)

namespace prv
{

    namespace helper
    {
        class StdHandleReader;
    }

    /// WinAPI based process
    class WinProcess : public IProcess
    {
    public:
        static WinProcess* Create(const ProcessSetup& setup);

        //--

        WinProcess();
        virtual ~WinProcess();

        virtual bool wait(uint32_t timeoutMS) override;
        virtual void terminate() override;
        virtual ProcessID id() const override;
        virtual bool isRunning() const override;
        virtual bool exitCode(int& outExitCode) const override;

    private:
        HANDLE m_hProcess;
        ProcessID m_id;

        helper::StdHandleReader* m_stdReader;
    };

} // prv

END_BOOMER_NAMESPACE_EX(process)
