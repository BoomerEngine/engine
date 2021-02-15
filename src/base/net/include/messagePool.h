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
        struct BASE_NET_API Message : public IReferencable
        {
        public:
            Message(Type messageType, void* payload);
            ~Message();

            //--

            /// get message type
            INLINE Type type() const { return m_type; }

            /// get message payload (data)
            INLINE void* payload() const { return m_data; }

            //--

            /// dump the message content (all fields) into a stream (mostly used for debugging)
            void print(IFormatStream& f) const;

            /// dispatch message on given object (call a message handler function)
            /// optionally we may specify the connection object that should be used to respond to the message RIGHT AWAY
            /// NOTE: this function makes no assumption over the thread safety of the dispatch, it's up to the caller to know this, in principle it's possible to process messages fully ansychronously
            bool dispatch(IObject* object, IObject* context = nullptr) const;

            //--

            // allocate message of given type
            static MessagePtr AllocateFromPool(Type messageType);

            //--

        private:
            Type m_type;
            void* m_data; // payload

            virtual void dispose() override final;
        };

        //--

    } // msg
} // base