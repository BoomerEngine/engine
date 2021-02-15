/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: runtime #]
***/

#pragma once

#include "base/script/include/scriptOpcodes.h"
#include "base/containers/include/hashSet.h"
#include "base/containers/include/queue.h"
#include "base/containers/include/pagedBuffer.h"
#include "base/containers/include/hashSet.h"
#include "base/memory/include/linearAllocator.h"

namespace base
{
    namespace script
    {

        //----

        class ScriptedClass;
        class ScriptedStruct;

        // holder for script managed RTTI objets, mostly functions, classes and enum types
        // NOTE: type objects are NEVER destroyed as this violates the RTTI contract that type pointers never change
        // NOTE: this also means that VERY heavy changes in the scripts may not be reloadable without restart
        class TypeRegistry : public base::NoCopy
        {
        public:
            TypeRegistry();
            ~TypeRegistry();

            /// we have committed to do script reloading, prepare all script types for reloading
            void prepareForReload();

            /// create a global function, may return existing object
            rtti::Function* createFunction(StringID name, ClassType parentClass);

            /// create an enum type
            rtti::EnumType* createEnum(StringID name, uint32_t enumTypeSize);

            /// create a class
            ScriptedClass* createClass(StringID name, ClassType nativeClass);

            /// create a struct
            ScriptedStruct* createStruct(StringID name);

        private:
            Array<rtti::Function*> m_allFunctions;
            HashMap<StringBuf, rtti::Function*> m_functionMap;

            Array<rtti::EnumType*> m_allEnums;
            HashMap<StringID, rtti::EnumType*> m_enumMap;

            Array<ScriptedClass*> m_allClasses;
            HashMap<StringID, ScriptedClass*> m_classMap;

            Array<ScriptedStruct*> m_allStructs;
            HashMap<StringID, ScriptedStruct*> m_structMap;

            HashSet<void*> m_usedStubs;

            static StringBuf BuildFunctionID(StringID name, ClassType parentClass);
        };

        //----

    } // script
} // base

