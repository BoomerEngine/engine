/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: messages #]
***/

#include "build.h"
#include "messagePool.h"
#include "messageObjectExecutor.h"

namespace base
{
    namespace net
    {

        //--

        Message::Message(Type messageType, void* payload)
            : m_type(messageType)
            , m_data(payload)
        {}

        Message::~Message()
        {
            if (m_type)
            {
                m_type->destruct(m_data);
                m_type = nullptr;
            }
        }

        void Message::print(IFormatStream& f) const
        {
            f.append("Message '{}': ", m_type);
            m_type->printToText(f, m_data);
        }

        void Message::dispose()
        {
            // TODO: pooling
            IReferencable::dispose();
        }

        bool Message::dispatch(IObject* object, IObject* context) const
        {
            return DispatchObjectMessage(object, m_type, m_data, context);
        }

        MessagePtr Message::AllocateFromPool(Type messageType)
        {
            // we only support classes for now
            if (!messageType || messageType->metaType() != rtti::MetaType::Class)
                return nullptr;

            auto messageClass = messageType.toClass();

            // compute size of the required payload, NOTE: we need to attach the payload with proper alignment
            auto dataAlignment  = std::max<uint32_t>(alignof(Message), messageClass->alignment());
            auto dataOffset = Align<uint32_t>(sizeof(Message), messageClass->alignment());
            auto dataSize = dataOffset + messageClass->size();
            auto data = mem::GlobalPool<POOL_NET_MESSAGE>::Alloc(dataSize, dataAlignment);
            if (!data)
                return nullptr; // OOM that we can handle somehow, also prevents from sending messages of VERY large classes

            // prevent unsafe data leakage
            memzero(data, dataSize);

            // initialize the payload
            auto payloadPtr = OffsetPtr(data, dataOffset);
            messageClass->construct(payloadPtr);

            // make into a message
            return NoAddRef(new (data) Message(messageClass, payloadPtr));
        }

        //--

    } // msg
} // base

