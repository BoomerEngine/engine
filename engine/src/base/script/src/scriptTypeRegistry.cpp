/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"
#include "scriptTypeRegistry.h"
#include "scriptObject.h"
#include "scriptClass.h"

namespace base
{
    namespace script
    {

        //--

        TypeRegistry::TypeRegistry()
        {
        }

        TypeRegistry::~TypeRegistry()
        {
            // unbind shit
            prepareForReload();

            // release functions
            m_allFunctions.clearPtr();
            m_functionMap.clear();
            m_allEnums.clearPtr();
            m_enumMap.clear();
            m_allClasses.clearPtr();
            m_classMap.clearPtr();
            m_allStructs.clearPtr();
            m_structMap.clearPtr();
        }

        void TypeRegistry::prepareForReload()
        {
            // cleanup classes
            TRACE_INFO("Cleaning {} script classes", m_allClasses.size());
            for (auto classPtr  : m_allClasses)
                classPtr->clearForReloading();

            // cleanup classes
            TRACE_INFO("Cleaning {} script structures", m_allStructs.size());
            for (auto classPtr  : m_allStructs)
                classPtr->clearForReloading();

            // unbind script functions
            TRACE_INFO("Unbinding {} script functions", m_allFunctions.size());
            for (auto func  : m_allFunctions)
                func->cleanupScripted();

            // cleanup enums from any values
            TRACE_INFO("Cleaning {} script enums", m_allEnums.size());
            for (auto enumPtr  : m_allEnums)
                enumPtr->clear();

            // reset list of used stubs
            m_usedStubs.clear();
        }

        StringBuf TypeRegistry::BuildFunctionID(StringID name, ClassType parentClass)
        {
            if (parentClass == nullptr)
                return StringBuf(name.view());

            return TempString("{}_{}", parentClass->name(), name);
        }

        rtti::Function* TypeRegistry::createFunction(StringID name, ClassType parentClass)
        {
            auto id = BuildFunctionID(name, parentClass);

            // use existing function if possible
            rtti::Function* ret = nullptr;
            if (m_functionMap.find(id, ret))
            {
                // return only once
                if (m_usedStubs.insert(ret))
                    return ret;

                // stub is already in use
                TRACE_ERROR("Scripted function '{}' was already loaded from scripts, seems like we have a duplicate", name);
                return nullptr;
            }

            // create new object
            ret = MemNewPool(POOL_SCRIPTS, rtti::Function, parentClass.ptr(), name, true);

            // register in the parent class
            if (parentClass)
                const_cast<rtti::IClassType*>(parentClass.ptr())->addFunction(ret);
            else
                RTTI::GetInstance().registerGlobalFunction(ret);

            // add to list of all functions
            m_functionMap[id] = ret;
            m_allFunctions.pushBack(ret);
            m_usedStubs.insert(ret);
            return ret;
        }

        rtti::EnumType* TypeRegistry::createEnum(StringID name, uint32_t enumTypeSize)
        {
            // use existing enum type if possible
            rtti::EnumType* ret = nullptr;
            if (m_enumMap.find(name, ret))
            {
                // return only once
                if (m_usedStubs.insert(ret))
                    return ret;

                // stub is already in use
                TRACE_ERROR("Scripted enum '{}' was already loaded from scripts, seems like we have a duplicate", name);
                return nullptr;
            }

            // create new object
			TRACE_INFO("Created scripted enum {}", name);
            ret = MemNewPool(POOL_SCRIPTS, rtti::EnumType, name, enumTypeSize, 0, true);
            RTTI::GetInstance().registerType(ret);

            // add to list of all enums
            m_enumMap[name] = ret;
            m_allEnums.pushBack(ret);
            m_usedStubs.insert(ret);
            return ret;
        }

        ScriptedClass* TypeRegistry::createClass(StringID name, ClassType nativeClass)
        {
            ASSERT(nativeClass != nullptr);
            ASSERT(!nativeClass->isAbstract());
            ASSERT(nativeClass->is(ScriptedObject::GetStaticClass()));

            // use existing enum type if possible
            ScriptedClass* ret = nullptr;
            if (m_classMap.find(name, ret))
            {
                // return only once
                if (m_usedStubs.insert(ret))
                    return ret;

                // stub is already in use
                TRACE_ERROR("Scripted class '{}' was already loaded from scripts, seems like we have a duplicate", name);
                return nullptr;
            }

            // create new object
			TRACE_INFO("Created scripted class {}", name);
			ret = MemNewPool(POOL_SCRIPTS, ScriptedClass, name, nativeClass);
            RTTI::GetInstance().registerType(ret);

            // add to list of all enums
            m_classMap[name] = ret;
            m_allClasses.pushBack(ret);
            m_usedStubs.insert(ret);
            return ret;
        }

        ScriptedStruct* TypeRegistry::createStruct(StringID name)
        {
            // use existing enum type if possible
            ScriptedStruct* ret = nullptr;
            if (m_structMap.find(name, ret))
            {
                // return only once
                if (m_usedStubs.insert(ret))
                    return ret;

                // stub is already in use
                TRACE_ERROR("Scripted struct '{}' was already loaded from scripts, seems like we have a duplicate", name);
                return nullptr;
            }

            // create new object
			TRACE_INFO("Created scripted struct {}", name);
			ret = MemNewPool(POOL_SCRIPTS, ScriptedStruct, name);
            RTTI::GetInstance().registerType(ret);

            // add to list of all enums
            m_structMap[name] = ret;
            m_allStructs.pushBack(ret);
            m_usedStubs.insert(ret);
            return ret;
        }

        //--

    } // script
} // base