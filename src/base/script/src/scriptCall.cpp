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
#include "scriptCall.h"

BEGIN_BOOMER_NAMESPACE(base::script)

//---

ScriptedCallRaw::ScriptedCallRaw(void* context, ClassType classPtr, StringID name)
    : m_context(context)
    , m_function(nullptr)
{
    if (classPtr)
        m_function = classPtr->findFunction(name);
}

ScriptedCallRaw::ScriptedCallRaw(IObject* context, StringID name)
    : m_context(context)
    , m_function(nullptr)
{
    if (context)
        m_function = context->cls()->findFunction(name);
}

END_BOOMER_NAMESPACE(base::script)
