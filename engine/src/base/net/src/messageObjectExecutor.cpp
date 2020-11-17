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
        namespace prv
        {

            /// message dispatcher via function interface on objects, basically a fancy way to call a function in a class without a one big "switch"
            class MessageObjectExecutorTypeRegistry : public ISingleton
            {
                DECLARE_SINGLETON(MessageObjectExecutorTypeRegistry);

            public:
                struct ClassEntry : public mem::GlobalPoolObject<POOL_NET_MESSAGES>
                {
                    ClassType m_classes;
                    HashMap<ClassType, const rtti::Function*> m_messageFunctions;
                };

                struct ContextObjectEntry : public mem::GlobalPoolObject<POOL_NET_MESSAGES>
                {
                    ClassType m_contextObject;
                    SpinLock m_classMapLock;
                    HashMap<ClassType, ClassEntry*> m_classMap;
                };

                SpinLock m_contextObjectMapLock;
                HashMap<ClassType, ContextObjectEntry*> m_contextObjectMap;

                //--

                const ClassEntry* typeInfoForClass(ClassType contextObjectClass, ClassType classType);

                void buildSupportedMessageClassesList(ClassType contextObjectClass, ClassType objectClass, HashMap<ClassType, const rtti::Function*>& outFunctions) const;

                bool checkMessageSupport(ClassType contextObjectType, ClassType objectType, Type messageType);

                //--

                virtual void deinit() override;
            };

            void MessageObjectExecutorTypeRegistry::deinit()
            {
                for (auto* contextEntry : m_contextObjectMap.values())
                    contextEntry->m_classMap.clearPtr();
                m_contextObjectMap.clearPtr();
            }

            bool MessageObjectExecutorTypeRegistry::checkMessageSupport(ClassType contextObjectType, ClassType objectType, Type messageType)
            {
                if (!objectType || objectType->metaType() != rtti::MetaType::Class)
                    return false;

                if (!messageType || messageType->metaType() != rtti::MetaType::Class)
                    return false;

                auto classInfo = typeInfoForClass(contextObjectType, objectType);
                if (!classInfo)
                    return false; // class does not support any messages

                return classInfo->m_messageFunctions.contains(messageType.toClass());
            }

            void MessageObjectExecutorTypeRegistry::buildSupportedMessageClassesList(ClassType contextObjectClass, ClassType objectClass, HashMap<ClassType, const rtti::Function*>& outFunctions) const
            {
                HashSet<StringID> checkedFunctionNames;

                // check all functions
                for (auto func : objectClass->allFunctions())
                {
                    // don't bother with base functions for virtual functions
                    if (!checkedFunctionNames.insert(func->name()))
                        continue;

                    // skip functions that are static since there's no context to call them on
                    if (func->isStatic())
                        continue;

                    // if we have a context object to pass we should have two params, otherwise one is ok
                    auto maxParams = contextObjectClass ? 2 : 1;
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
                    if (contextObjectClass != nullptr && func->numParams() == 2)
                    {
                        // second param can be a const ref to shared pointer of the context type
                        auto& contextParam = func->params()[1];
                        if (!contextParam.m_flags.test(rtti::FunctionParamFlag::ConstRef))
                            continue;

                        // first param should be a handle
                        if (contextParam.m_type->metaType() != rtti::MetaType::StrongHandle && contextParam.m_type->metaType() != rtti::MetaType::WeakHandle)
                            continue;

                        // it should match the type we want to pass as a context
                        auto handleType = static_cast<const rtti::IHandleType*>(contextParam.m_type.ptr());
                        if (!contextObjectClass->is(handleType->pointedClass()))
                            continue;
                    }

                    TRACE_INFO("Found message handling function '{}' in class '{}' to handle message type '{}' with context {}", func->name(), objectClass->name(), messageStructType->name(), contextObjectClass);
                    outFunctions[messageStructType] = func;
                }
            }

            const MessageObjectExecutorTypeRegistry::ClassEntry* MessageObjectExecutorTypeRegistry::typeInfoForClass(ClassType contextObjectClass, ClassType classType)
            {
                ContextObjectEntry* contextEntry = nullptr;

                {
                    auto lock = CreateLock(m_contextObjectMapLock);
                    if (!m_contextObjectMap.find(contextObjectClass, contextEntry))
                    {
                        contextEntry = new ContextObjectEntry;
                        contextEntry->m_contextObject = contextObjectClass;
                        m_contextObjectMap[contextObjectClass] = contextEntry;
                    }
                }

                ClassEntry* classEntry = nullptr;

                {
                    auto lock = CreateLock(contextEntry->m_classMapLock);
                    if (!contextEntry->m_classMap.find(classType, classEntry))
                    {
                        classEntry = new ClassEntry;
                        buildSupportedMessageClassesList(contextEntry->m_contextObject, classType, classEntry->m_messageFunctions);
                        contextEntry->m_classMap[classType] = classEntry;
                    }
                }

                return classEntry;
            }

        } // prv

        bool CheckMessageSupport(ClassType contextObjectType, ClassType objectType, Type messageType)
        {
            return prv::MessageObjectExecutorTypeRegistry::GetInstance().checkMessageSupport(contextObjectType, objectType, messageType);
        }

        bool DispatchObjectMessage(IObject* object, Type messageType, const void* messagePayload, IObject* contextObject)
        {
            const auto objectType = object ? object->cls() : nullptr;
            const auto contextObjectType = contextObject ? contextObject->cls() : nullptr;
            if (const auto* classInfo = prv::MessageObjectExecutorTypeRegistry::GetInstance().typeInfoForClass(contextObjectType, objectType))
            {
                const rtti::Function* callFunction = nullptr;
                if (classInfo->m_messageFunctions.find(messageType.toClass(), callFunction))
                {
                    rtti::FunctionCallingParams params;

                    if (contextObjectType && callFunction->numParams() == 2)
                    {
                        ObjectPtr contextObjectPtr(AddRef(contextObject));

                        params.m_argumentsPtr[0] = (void*)messagePayload;
                        params.m_argumentsPtr[1] = (void*)&contextObjectPtr;
                        callFunction->run(nullptr, object, params);
                        return true;
                    }
                    else if (callFunction->numParams() == 1)
                    {
                        params.m_argumentsPtr[0] = (void*)messagePayload;
                        callFunction->run(nullptr, object, params);
                        return true;
                    }
                }
                else
                {
                    TRACE_WARNING("Message: No handler function for message '{}' with context '{}' in object '{}'", messageType, contextObject, objectType);
                }
            }

            return false;
        }

    } // msg
} // base

