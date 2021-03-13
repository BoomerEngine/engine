/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "core/system/include/timing.h"
#include "localServiceContainer.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// wrapper that hosts a command that is executing in the background (it's own fiber)
class CORE_APP_API CommandHost : public IReferencable, public IProgressTracker
{
public:
    CommandHost(IProgressTracker* progress = nullptr);
    virtual ~CommandHost(); // will wait for the command to finish, there's no other way

    //--

    // is the command still running ?
    INLINE bool running() const { return m_command; }

    //--

    /// start command - by convention parameters are passed as a "commandline" object
    bool start(const CommandLine& cmdLine);

    /// request command to stop
    void cancel();

    /// update command state, dispatch messages (should be called periodically)
    /// NOTE: this function will return false once we are done
    bool update();

    //--

private:
    // command itself
    CommandPtr m_command;

    // signal that is flagged once command finishes
    FiberSemaphore m_finishedSignal;
    std::atomic_bool m_finishedFlag = false;

    // local cancellation flag
    std::atomic_bool m_cancelation = false;

    // external progress interface
    IProgressTracker* m_externalProgressTracker = nullptr;

    //--

    virtual bool checkCancelation() const override;
    virtual void reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text) override;
};

//--

END_BOOMER_NAMESPACE()
