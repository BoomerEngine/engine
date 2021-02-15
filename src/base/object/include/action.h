/***
 * Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: actions #]
***/

#pragma once

namespace base
{

    //---

    /// base undo/redo action
    class BASE_OBJECT_API IAction : public IReferencable
    {
    public:
        IAction();
        virtual ~IAction();

        //--

        /// get an ID that can be used for merging, etc
        /// NOTE: this is not needed if action can't be merged
        virtual StringID id() const { return StringID::EMPTY(); }

        /// get human readable description of the action (for UI/logging)
        virtual StringBuf description() const = 0;

        /// perform the action, action is free to fail (in a nondestructive manner)
        virtual bool execute() = 0;

        /// undo execution of this action
        virtual bool undo() = 0;

        /// whenever we have an action of the same class at the top of the stack we call this method to check if the actions can be merged
        /// this is especially true for action that can be created in large amounts (like dragging objects)
        /// NOTE: if the merge was successful the last action will be removed and replaced with this one
        virtual bool tryMerge(const IAction& lastUndoAction) { return false; }

        ///--

        /// get internal action sequence number, usually done once per frame but can be done manually
        /// NOTE: usually only actions with the same sequence number can be merged
        INLINE uint32_t sequenceNumber() const { return m_sequenceNumber; }

        /// bump internal action sequence number
        static void BumpSequenceNumber();

        ///--
        
    private:
        uint32_t m_sequenceNumber;
    };

    //---

} // base
