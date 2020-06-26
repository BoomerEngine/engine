/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: messages #]
***/

#include "build.h"
#include "messagePool.h"

namespace base
{
    namespace net
    {

        //--

        Message::Message(Type messageType, const ObjectWeakPtr& target, MessagePool* pool, void* payload)
            : m_type(messageType)
            , m_target(target)
            , m_pool(pool)
            , m_data(payload)
        {}

        Message::~Message()
        {
            ASSERT(m_pool == nullptr);
        }

        void Message::release()
        {
            if (m_pool != nullptr)
            {
                if (m_type)
                {
                    m_type->destruct(m_data);
                    m_type = nullptr;
                }

                m_pool = nullptr;
                MemDelete(this); // TODO: release to pool
            }
        }

        //--

        MessagePool::MessagePool()
        {}

        MessagePool::~MessagePool()
        {}

        Message* MessagePool::allocate(Type messageType, IObject* target)
        {
            // we only support classes for now
            if (!messageType || messageType->metaType() != rtti::MetaType::Class)
                return nullptr;

            auto messageClass = messageType.toClass();

            // compute size of the required payload, NOTE: we need to attach the payload with proper alignment
            auto dataAlignment  = std::max<uint32_t>(alignof(Message), messageClass->alignment());
            auto dataOffset  = Align<uint32_t>(sizeof(Message), messageClass->alignment());
            auto dataSize  = dataOffset + messageClass->size();
            auto data  = MemAlloc(POOL_NET, dataSize, dataAlignment);
            if (!data)
                return nullptr; // OOM tha we can handle somehow, also prevents from sending messages of VERY large classes

            // prevent unsafe data leakage
            memzero(data, dataSize);

            // initialize the payload
            auto payloadPtr = OffsetPtr(data, dataOffset);
            messageClass->construct(payloadPtr);

            // make into a message
            return new (data) Message(messageClass, target, this, payloadPtr);
        }

        //--

    } // msg
} // base

