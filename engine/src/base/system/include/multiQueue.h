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

    /// A set of multiple queues (masked) a job may be picked up by any matching mask
    class BASE_SYSTEM_API IMultiQueue : public base::NoCopy
    {
    public:
        ///! close queue, all jobs are discarded
        virtual void close() = 0;

        ///! post a job to the queue with a specific mask of possible receivers
        virtual void push(void* jobData, uint32_t queueMask) = 0;

        ///! pop a job from the queue for given mask, returns false if all queues were empty (we are shutting down)
        virtual void* pop(uint32_t queueMask) = 0;

        //! get all items from queue (slow)
        virtual void inspect(const TQueueInspectorFunc& inspector) = 0;

        //--

        //! Create a job queue
        static IMultiQueue* Create(uint32_t numInternalQueues);

    protected:
        virtual ~IMultiQueue();
    };

    //-----------------------------------------------------------------------------

} // base