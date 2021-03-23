/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
***/

#include "build.h"
#include "object.h"

BEGIN_BOOMER_NAMESPACE_EX(rendering)

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IRenderingObject);
RTTI_END_TYPE();

IRenderingObject::~IRenderingObject()
{}

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IRenderingObjectManager);
RTTI_END_TYPE();

IRenderingObjectManager::~IRenderingObjectManager()
{}

void IRenderingObjectManager::renderLock()
{
    auto lock = CreateLock(m_commandLock);

    DEBUG_CHECK_EX(!m_locked, "Object manager already locked for rendering");
    m_locked = true;
}

void IRenderingObjectManager::renderUnlock()
{
    auto lock = CreateLock(m_commandLock);

    DEBUG_CHECK_EX(m_locked, "Object manager already locked for rendering");
    m_locked = false;

    auto bufferedCommands = std::move(m_bufferedCommands);
    for (const auto* cmd : bufferedCommands)
    {
        cmd->func();
        delete cmd;
    }
}

void IRenderingObjectManager::runNowOrBuffer(std::function<void(void)> func)
{
    auto lock = CreateLock(m_commandLock);

    if (m_locked)
    {
        auto* buffered = new BufferedCommand();
        buffered->func = std::move(func);
        m_bufferedCommands.pushBack(buffered);
    }
    else
    {
        func();
    }
}

//---

END_BOOMER_NAMESPACE_EX(rendering)
