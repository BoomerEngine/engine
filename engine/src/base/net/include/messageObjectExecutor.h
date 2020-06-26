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
#include "base/containers/include/queue.h"

namespace base
{
    namespace net
    {

        //--

        /// message dispatcher via function interface on objects
        class BASE_NET_API MessageObjectExecutor : public NoCopy
        {
        public:
            MessageObjectExecutor(ClassType contextObjectClass = nullptr);
            ~MessageObjectExecutor();

            /// check if given message type is supported by given object type
            bool checkMessageSupport(ClassType objectType, Type messageType);

            /// queue message for execution
            void queueMessage(Message* message);

            /// execute all queued messages, clean the message queue
            void executeQueuedMessges(IObject* contextObject);

        private:
            ClassType m_contextObjectClass;

            struct ClassEntry
            {
                ClassType m_classes;
                HashMap<ClassType, const rtti::Function*> m_messageFunctions;
            };

            HashMap<ClassType, ClassEntry*> m_classMap;
            SpinLock m_classMapLock;

            Queue<Message*> m_queue;
            SpinLock m_queueLock;

            //--

            void buildSupportedMessageClassesList(ClassType objectClass, HashMap<ClassType, const rtti::Function*>& outFunctions) const;

            const ClassEntry* typeInfoForClass(ClassType classType);

            void executeMessage(IObject* contextObject, const Message* message);
        };

        //--

    } // msg
} // base