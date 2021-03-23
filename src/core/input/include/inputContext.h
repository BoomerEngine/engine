/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input #]
***/

#pragma once

#include "core/containers/include/queue.h"
#include "inputStructures.h"

BEGIN_BOOMER_NAMESPACE()

///---

/// native window message
struct NativeWindowEventWinApi
{
    uint64_t m_window;
    uint32_t m_message;
    uint64_t m_lParam;
    uint64_t m_wParam;

    uint64_t returnValue = 0;
    bool processed = false;
};

struct NativeWindowEventX11
{
    void* m_display;
    void* m_screen;
    void* m_inputContext;
    uint64_t m_window;
    const void* m_message;
};

/// input context - contains device instances that produce input
class CORE_INPUT_API IInputContext : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(IInputContext, IObject);

public:
    // drop all events from the queue
    void clear();

    // inject input event at the end of the queue
    void inject(const EventPtr& evt);

    // get pending event 
    EventPtr pull();

    //--

    // reset input state, usually done when window is deactivated
    virtual void resetInput() = 0;

    // process internal state
    virtual void processState() = 0;

    // process a message
    virtual void processMessage(const void* msg) = 0;

    // request capture of mouse to given window, capture mode 1=normal, 2=hide cursor (only delta values are sent then in MouseMove)
    virtual void requestCapture(int captureMode) = 0;

    //--

    // create platform input context
    static ContextPtr CreateNativeContext(uint64_t nativeWindow, uint64_t nativeDisplay, bool gameMode);

protected:
    IInputContext();

    SpinLock m_eventQueueLock;
    Queue<EventPtr> m_eventQueue;
};

///---

END_BOOMER_NAMESPACE()
