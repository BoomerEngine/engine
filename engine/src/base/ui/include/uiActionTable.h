/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: misc #]
***/

#pragma once

#include "base/input/include/inputStructures.h"
#include "uiEventFunction.h"

namespace ui
{

    /// command execution function
    typedef std::function<void(UI_EVENT)> TCommandRun;
    typedef std::function<bool()> TCommandCanRun;

    /// command key shortcut
    struct BASE_UI_API KeyShortcut
    {
    public:
        KeyShortcut();
        KeyShortcut(base::input::KeyCode key, bool shift = false, bool ctrl = false, bool alt=false);
        explicit KeyShortcut(base::StringView txt); // will remain empty if passed string is not valid
        KeyShortcut(const char* txt); // will remain empty if passed string is not valid

        //--

        INLINE operator bool() const { return m_key != base::input::KeyCode::KEY_INVALID; }
        INLINE bool valid() const { return m_key != base::input::KeyCode::KEY_INVALID; }

        //--

        INLINE base::input::KeyCode keyCode() const { return m_key; }
        INLINE bool shiftRequired() const { return m_shift; }
        INLINE bool controlRequired() const { return m_ctrl; }
        INLINE bool altRequired() const { return m_alt; }

        //--

        bool matches(const base::input::KeyEvent& evt) const;

        //--

        void print(base::IFormatStream& str) const;

        //--

    private:
        base::input::KeyCode m_key;
        uint8_t m_shift:1;
        uint8_t m_ctrl:1;
        uint8_t m_alt:1;
    };

    /// action info
    struct ActionInfo : public base::mem::GlobalPoolObject<POOL_ACTIONS>
    {
        base::StringID name;
        base::Variant data; // additional data for command function, passed to all callbacks
        TEventFunction runFunc;
        TEventFunction toggledFunc;
        TEventFunction enabledFunc;
    };

    /// action status bits
    enum class ActionStatusBit : uint8_t
    {
        Defined = FLAG(0),
        Enabled = FLAG(1),
        Toggled = FLAG(2),
    };

    typedef base::DirectFlags<ActionStatusBit> ActionStatusFlags;

    /// bindings for commands, determines actions to be performed when command occurs
    /// also defines conditional evaluators for commands
    class BASE_UI_API ActionTable : public base::NoCopy, public base::mem::GlobalPoolObject<POOL_ACTIONS>
    {
    public:
        ActionTable(IElement* owner);
        ~ActionTable();

        ///---

        /// remove all actions
        void clear();

        /// remove action
        void remove(base::StringID name);

        //--

        /// bind command shortcut
        void bindShortcut(base::StringID name, KeyShortcut shortcut);

        /// add a command binding
        EventFunctionBinder bindCommand(base::StringID name);

        /// add a command filter
        EventFunctionBinder bindFilter(base::StringID name);

        /// add a command toggle checker
        EventFunctionBinder bindToggle(base::StringID name);

        ///---

        /// get action status, requires context for action to evaluate it's state
        ActionStatusFlags status(base::StringID name) const;

        /// find command information
        const ActionInfo* find(base::StringID name) const;

        ///---

        /// run action
        bool run(base::StringID name, IElement* source) const;

        /// process key event short cut
        /// looks for matching command and if found calls it
        bool processKeyEvent(const base::input::KeyEvent& evt) const;

        ///---

    private:
        base::HashMap<base::StringID, ActionInfo*> m_actionMap;

        struct ShortCutMapping
        {
            KeyShortcut shortcut;
            base::StringID name;
        };

        base::Array<ShortCutMapping> m_shortcuts;

        IElement* m_host = nullptr;

        ActionInfo* get(base::StringID name);
    };

} // ui
