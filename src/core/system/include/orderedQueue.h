/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading #]
***/

#pragma once

#include "algorithms.h"

BEGIN_BOOMER_NAMESPACE()

//-----------------------------------------------------------------------------

/// A simple ordered queue - jobs are picked up in the "priorityOrder" first and than submission order
class CORE_SYSTEM_API IOrderedQueue : public NoCopy
{
public:
    ///! close queue, all jobs are discarded
    virtual void close() = 0;

    ///! post a job to the queue with a specific order index, the larger indices are picked last
    virtual void push(void* jobData, uint64_t order=0) = 0;

    ///! pop a job from the queue for given mask, returns false if all queues were empty (we are shutting down)
    virtual void* pop() = 0;

    //! get all items from queue (slow)
    virtual void inspect(const TQueueInspectorFunc& inspector) = 0;

    //--

    //! Create a job queue
    static IOrderedQueue* Create();

protected:
    virtual ~IOrderedQueue();
};

//-----------------------------------------------------------------------------

END_BOOMER_NAMESPACE()
