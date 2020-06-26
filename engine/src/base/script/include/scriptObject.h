/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#pragma once

#include "base/object/include/object.h"

namespace base
{
    namespace script
    {

        //----

        class ScriptedClass;
        class ScriptedClassProperty;

        //----

        // scripted class type that derives from native type
		class BASE_SCRIPT_API ScriptedObject : public IObject
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ScriptedObject, IObject);

        public:
            ScriptedObject();
            virtual ~ScriptedObject();

            // get object class, dynamic, returns true scripted class
            virtual ClassType cls() const override final { return m_scriptedClass; }

            // get internal data buffer
            INLINE const void* scriptedData() const { return m_scriptPropertiesData; }
            INLINE void* scriptedData() { return m_scriptPropertiesData; }

        private:
            ClassType m_scriptedClass;
            void* m_scriptPropertiesData;

            friend class ScriptedClassProperty;
            friend class ScriptedClass;
        };

        ///---

    } // script
} // base