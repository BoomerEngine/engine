/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: actions #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

//---

/// notification handler for action history, called after each action is executed
/// NOTE: main use is the UI update
class BASE_OBJECT_API IActionHistoryNotificationHandler : public NoCopy
{
public:
    virtual ~IActionHistoryNotificationHandler();

    /// action was executed
    virtual void handleActionExecuted(const IAction* action, bool result, bool redo) = 0;

    /// action was undone
    virtual void handleActionUndone(const IAction* action, bool result) = 0;
};

/// undo/redo action history
class BASE_OBJECT_API ActionHistory : public IReferencable
{
public:
    ActionHistory();
    virtual ~ActionHistory();

    ///--

    // register handler
    void registerListener(IActionHistoryNotificationHandler* handler);

    // unregister handler
    void unregisterListener(IActionHistoryNotificationHandler* handler);

    ///--

    // clear current action history, all actions are released
    void clear();

    /// are we during action execution ? (does not distinguish undo/redo)
    bool duringExecution() const;

    /// do we have any action to undo ?
    bool hasUndo() const;

    /// do we have any action to redo ?
    bool hasRedo() const;

    //--

    /// get description for the top undo action
    StringBuf undoActionName() const;

    /// get description for the top redo action
    StringBuf redoActionName() const;

    //--

    /// execute given action, if the execution was successful the action is added on top of the undo stack
    /// NOTE: the action pointer's ownership will be transfer to the action list
    /// NOTE: executing new action clear the redo stack
    bool execute(const ActionPtr& action);

    /// undo the action from the top of the undo stack, returns true on success
    /// action is moved to the redo stack
    bool undo();

    /// redo the action from the top of the redo stack
    bool redo();

    //---

private:
    // listeners
    typedef Array<IActionHistoryNotificationHandler*> TListeners;
    TListeners m_listeners;

    // action queues
    typedef Array<ActionPtr> TActionQueue;
    TActionQueue m_undoList;
    TActionQueue m_redoList;

    // lock to state when we are performing operation
    bool m_operationLock;

    //--

    void notifyActionExecuted(IAction* action, bool valid, bool redo);
    void notifyActionUndo(IAction* action, bool valid);
};

//---

END_BOOMER_NAMESPACE(base)
