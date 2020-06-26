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
#include "base/containers/include/hashSet.h"
#include "base/object/include/rttiClassType.h"
#include "base/object/include/rttiHandleType.h"
#include "base/object/include/rttiFunction.h"
#include "base/object/include/rttiFunctionPointer.h"

namespace base
{
    namespace net
    {

        //---

        MessageObjectExecutor::MessageObjectExecutor(ClassType contextObjectClass)
            : m_contextObjectClass(contextObjectClass)
        {}

        MessageObjectExecutor::~MessageObjectExecutor()
        {
            m_classMap.clearPtr();

            while (!m_queue.empty())
            {
                auto message  = m_queue.top();
				m_queue.pop();
                message->release();
            }
        }

        bool MessageObjectExecutor::checkMessageSupport(ClassType objectType, Type messageType)
        {
            if (!objectType || objectType->metaType() != rtti::MetaType::Class)
                return false;

            if (!messageType || messageType->metaType() != rtti::MetaType::Class)
                return false;

            auto classInfo  = typeInfoForClass(objectType);
            if (!classInfo)
                return false; // class does not support any messages

            return classInfo->m_messageFunctions.contains(messageType.toClass());
        }

        void MessageObjectExecutor::queueMessage(Message* message)
        {
            auto lock = CreateLock(m_queueLock);
            m_queue.push(message);
        }

        void MessageObjectExecutor::executeQueuedMessges(IObject* contextObject)
        {
            PC_SCOPE_LVL0(ExecuteQueuedMessges);

            Queue<Message*> messages;
            {
                auto lock = CreateLock(m_queueLock);
                messages = std::move(m_queue);
            }

            while (!messages.empty())
            {
                auto message  = messages.top();
				messages.pop();
                executeMessage(contextObject, message);
                message->release();
            }
        }

        void MessageObjectExecutor::buildSupportedMessageClassesList(ClassType objectClass, HashMap<ClassType, const rtti::Function*>& outFunctions) const
        {
            HashSet<StringID> checkedFunctionNames;

            // check all functions
            for (auto func  : objectClass->allFunctions())
            {
                // don't bother with base functions for virtual functions
                if (!checkedFunctionNames.insert(func->name()))
                    continue;

                // skip functions that are static since there's no context to call them on
                if (func->isStatic())
                    continue;

                // if we have a context object to pass we should have two params, otherwise one is ok
                auto maxParams = (m_contextObjectClass != nullptr) ? 2 : 1;
                if (func->numParams() < 1 || func->numParams() > maxParams)
                    continue;

                // first param should be a const ref to a structure
                auto& dataParam = func->params()[0];
                if (!dataParam.m_flags.test(rtti::FunctionParamFlag::ConstRef))
                    continue;

                // it should also be a struct
                if (dataParam.m_type->metaType() != rtti::MetaType::Class)
                    continue;

                // OK, if it's a struct it can be a message function
                auto messageStructType = dataParam.m_type.toClass();
                if (messageStructType->is(IObject::GetStaticClass()))
                    continue; // we don't support objects

                // validate optional context param
                if (m_contextObjectClass != nullptr && func->numParams() == 2)
                {
                    // second param can be a const ref to shared pointer of the context type
                    auto& contextParam = func->params()[1];
                    if (!contextParam.m_flags.test(rtti::FunctionParamFlag::ConstRef))
                        continue;

                    // first param should be a handle
                    if (contextParam.m_type->metaType() != rtti::MetaType::StrongHandle && contextParam.m_type->metaType() != rtti::MetaType::WeakHandle)
                        continue;

                    // it should match the type we want to pass as a context
                    auto handleType  = static_cast<const rtti::IHandleType*>(contextParam.m_type.ptr());
                    if (handleType->pointedClass() != m_contextObjectClass)
                        continue;
                }

                TRACE_INFO("Found message handling function '{}' in class '{}' to handle message type '{}'", func->name(), objectClass->name(), messageStructType->name());
                outFunctions[messageStructType] = func;
            }
        }

        const MessageObjectExecutor::ClassEntry* MessageObjectExecutor::typeInfoForClass(ClassType classType)
        {
            auto lock = CreateLock(m_classMapLock);

            MessageObjectExecutor::ClassEntry* ret = nullptr;
            if (m_classMap.find(classType, ret))
                return ret;

            ret = MemNewPool(POOL_NET, ClassEntry);
            buildSupportedMessageClassesList(classType, ret->m_messageFunctions);
            m_classMap[classType] = ret;

            return ret;
        }

        void MessageObjectExecutor::executeMessage(IObject* contextObject, const Message* message)
        {
            if (auto targetObject = message->targetObject().lock())
            {
                auto classType  = targetObject->cls();
                auto classInfo  = typeInfoForClass(classType);

                const rtti::Function* callFunction = nullptr;
                if (classInfo->m_messageFunctions.find(message->type().toClass(), callFunction))
                {
                    rtti::FunctionCallingParams params;

                    if (m_contextObjectClass != nullptr && callFunction->numParams() == 2)
                    {
                        params.m_argumentsPtr[0] = (void*)message->payload();
                        params.m_argumentsPtr[1] = (void*)&contextObject;
                        callFunction->run(nullptr, targetObject.get(), params);
                    }
                    else if (callFunction->numParams() == 1)
                    {
                        params.m_argumentsPtr[0] = (void*)message->payload();
                        callFunction->run(nullptr, targetObject.get(), params);
                    }
                }
            }
        }

    } // msg
} // base

