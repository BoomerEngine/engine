/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: actions #]
***/

#include "build.h"
#include "action.h"
#include "actionHistory.h"

BEGIN_BOOMER_NAMESPACE()

//----

IActionHistoryNotificationHandler::~IActionHistoryNotificationHandler()
{}

//---

ActionHistory::ActionHistory()
    : m_operationLock(false)
{}

ActionHistory::~ActionHistory()
{}

void ActionHistory::registerListener(IActionHistoryNotificationHandler* handler)
{
    DEBUG_CHECK(handler);
    DEBUG_CHECK(!m_listeners.contains(handler));
    m_listeners.pushBack(handler);
}

void ActionHistory::unregisterListener(IActionHistoryNotificationHandler* handler)
{
    DEBUG_CHECK(handler);

    auto index = m_listeners.find(handler);
    if (index != INDEX_NONE)
        m_listeners[index] = nullptr;
}

///--

void ActionHistory::clear()
{
    DEBUG_CHECK_EX(!m_operationLock, "Action is during execution");
    m_redoList.reset();
    m_undoList.reset();
}

bool ActionHistory::duringExecution() const
{
    return m_operationLock;
}

bool ActionHistory::hasUndo() const
{
    return !m_undoList.empty();
}

bool ActionHistory::hasRedo() const
{
    return !m_redoList.empty();
}

StringBuf ActionHistory::undoActionName() const
{
    if (!hasUndo())
        return StringBuf::EMPTY();

    const auto& topAction = m_undoList.back();
    return topAction->description();
}

StringBuf ActionHistory::redoActionName() const
{
    if (!hasRedo())
        return StringBuf::EMPTY();

    const auto& topAction = m_redoList.back();
    return topAction->description();
}

void ActionHistory::notifyActionExecuted(IAction* action, bool valid, bool redo)
{
    for (auto* handler : m_listeners)
        if (handler != nullptr)
            handler->handleActionExecuted(action, valid, redo);

    m_listeners.removeUnorderedAll(nullptr);
}

void ActionHistory::notifyActionUndo(IAction* action, bool valid)
{
    for (auto* handler : m_listeners)
        if (handler != nullptr)
            handler->handleActionUndone(action, valid);

    m_listeners.removeUnorderedAll(nullptr);
}

bool ActionHistory::execute(const ActionPtr& action)
{
    DEBUG_CHECK_EX(!m_operationLock, "Action is during execution");

    // null action, allow
    if (!action)
        return true;

    // begin the protected area
    m_operationLock = true;

    // execute the action
    if (!action->execute())
    {
        m_operationLock = false;
        TRACE_ERROR("Action '{}' failed to execute", action->description());
        notifyActionExecuted(action, false, false);
        return false; // action failed to execute
    }

    // we are adding new action to the undo stack, clear redo actions
    m_redoList.reset();

    // try to merge with current top undo action
    if (!m_undoList.empty())
    {
        auto topUndoAction = m_undoList.back();
        if (action->tryMerge(*topUndoAction))
            // if action merge was successful replace than we will be replacing current undo action on the stack
            m_undoList.popBack();
    }

    // push the new action to the top of the undo stack
    m_undoList.pushBack(action);

    // end the protected area
    m_operationLock = false;

    // done
    notifyActionExecuted(action, true, false);
    return true;
}

bool ActionHistory::undo()
{
    DEBUG_CHECK_EX(!m_operationLock, "Action is during execution");

    // there may be nothing to undo any way
    if (m_undoList.empty())
        return true;

    // being protected area
    m_operationLock = true;

    // get the last action from the undo list
    auto topAction = m_undoList.back();
    m_undoList.popBack();
    TRACE_INFO("Undo action '{}'", topAction->description());

    // undo the action
    if (!topAction->undo())
    {
        TRACE_WARNING("Failed to undo '{}'", topAction->description());

        // put action back into the undo list (it may work in a moment once something gets loaded, etc)
        notifyActionUndo(topAction, false);
        m_undoList.pushBack(topAction);
        m_operationLock = false;
        return false;
    }

    // end protected area
    m_operationLock = false;

    // move action to redo queue
    m_redoList.pushBack(topAction);

    // done
    notifyActionUndo(topAction, true);
    return true;
}

bool ActionHistory::redo()
{
    DEBUG_CHECK_EX(!m_operationLock, "Action is during execution");

    // there may be nothing to redo any way
    if (m_redoList.empty())
        return true;

    // being protected area
    m_operationLock = true;

    // get the last action from the redo list
    auto topAction = m_redoList.back();
    m_redoList.popBack();
    TRACE_INFO("Redo action '{}'", topAction->description());

    // redo the action
    if (!topAction->execute())
    {
        TRACE_WARNING("Failed to redo '{}'", topAction->description());

        // put action back into the redo list (it may work in a moment once something gets loaded, etc)
        m_redoList.pushBack(topAction);
        m_operationLock = false;
        notifyActionExecuted(topAction, false, true);
        return false;
    }

    // end protected area
    m_operationLock = false;

    // move action to undo queue
    m_undoList.pushBack(topAction);

    // done
    notifyActionExecuted(topAction, true, true);
    return true;
}

//----

END_BOOMER_NAMESPACE()

