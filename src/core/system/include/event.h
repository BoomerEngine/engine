/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//-----------------------------------------------------------------------------

/// Synchronization event
class CORE_SYSTEM_API Event : public NoCopy
{
public:
    Event(bool manualReset = false);
    ~Event();

    //! Triggers the event so any waiting threads are activated
    void trigger();

    //! Resets the event to an idle state
    void reset();

    //! Waits for the event to be triggered with timeout
    bool wait(uint32_t waitTime);

    //! Waits for the event to be triggered (forver)
    void waitInfinite();

private:
    void* m_event;          // Internal handle
};

//-----------------------------------------------------------------------------

END_BOOMER_NAMESPACE()
