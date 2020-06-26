/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"
#include "scriptClass.h"
#include "scriptObject.h"

namespace base
{
    namespace script
    {

        //---

        RTTI_BEGIN_TYPE_CLASS(ScriptedObject);
        RTTI_END_TYPE();

        ScriptedObject::ScriptedObject()
        {
            m_scriptedClass = ScriptedObject::GetStaticClass();
            m_scriptPropertiesData = nullptr;
        }

        ScriptedObject::~ScriptedObject()
        {
            //TRACE_INFO("Script: Destructor of scripted object '{}'", m_scriptedClass->name());
			if (m_scriptedClass && m_scriptedClass->scripted())
				static_cast<const ScriptedClass*>(m_scriptedClass.ptr())->destroyScriptedObject(this);
        }

        //---

    } // script
} // base
