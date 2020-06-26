/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: messages #]
***/

#pragma once

#include "base/object/include/object.h"
#include "base/system/include/thread.h"
#include "base/system/include/spinLock.h"

namespace base
{
    namespace net
    {

        //--

        /// a message
        struct BASE_NET_API Message : public NoCopy
        {
        public:
            Message(Type messageType, const ObjectWeakPtr& target, MessagePool* pool, void* payload);
            ~Message();

            //--

            /// get message type
            INLINE Type type() const { return m_type; }

            /// get target to execute the message one
            INLINE const ObjectWeakPtr& targetObject() const { return m_target; }

            /// get message payload (data)
            INLINE void* payload() const { return m_data; }

            //--

            /// release message back to pool
            void release();

        private:
            Type m_type;
            ObjectWeakPtr m_target;
            MessagePool* m_pool; // owner
            void* m_data; // payload
        };

        //--

        /// message pool, allocates message
        class BASE_NET_API MessagePool : public NoCopy
        {
        public:
            MessagePool();
            ~MessagePool();

            /// allocate message of given type targeted at given object
            Message* allocate(Type messageType, IObject* target);
        };

        //--

    } // msg
} // base