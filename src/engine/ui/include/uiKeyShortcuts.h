/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: misc #]
***/

#pragma once

#include "core/input/include/inputStructures.h"
#include "uiEventFunction.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

/// command key shortcut
struct ENGINE_UI_API KeyShortcut
{
public:
    KeyShortcut();
    KeyShortcut(input::KeyCode key, bool shift = false, bool ctrl = false, bool alt=false);
    explicit KeyShortcut(StringView txt); // will remain empty if passed string is not valid
    KeyShortcut(const char* txt); // will remain empty if passed string is not valid

    //--

    bool operator==(const KeyShortcut& other) const;
    INLINE bool operator!=(const KeyShortcut& other) const { return !operator==(other); }

    INLINE operator bool() const { return key != input::KeyCode::KEY_INVALID; }
    INLINE bool valid() const { return key != input::KeyCode::KEY_INVALID; }

    //--

    INLINE input::KeyCode keyCode() const { return key; }
    INLINE bool shiftRequired() const { return shift; }
    INLINE bool controlRequired() const { return ctrl; }
    INLINE bool altRequired() const { return alt; }

    //--

    bool matches(const input::KeyEvent& evt) const;

    //--

    void print(IFormatStream& str) const;

    static bool Parse(StringView txt, KeyShortcut& outKey);

    //--

private:
    input::KeyCode key;
    uint8_t shift:1;
    uint8_t ctrl:1;
    uint8_t alt:1;
};

/// bindings for key shortcuts
class ENGINE_UI_API KeyShortcutTable : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_UI_ACTIONS)

public:
    KeyShortcutTable(IElement* owner);
    ~KeyShortcutTable();

    ///---

    /// remove all actions
    void clear();

    /// remove key shortcut
    void remove(KeyShortcut name);

    //--

    /// bind a callback to key combination
    EventFunctionBinder bindShortcut(KeyShortcut shortcut);

    ///---

    /// process key event short cut
    /// looks for matching command and if found calls it
    bool processKeyEvent(const input::KeyEvent& evt) const;

    ///---

private:
    struct Shortcut
    {
        RTTI_DECLARE_POOL(POOL_UI_ACTIONS)

    public:
        KeyShortcut shortcut;
        TEventFunction callback;
    };

    Array<Shortcut*> m_shortcuts;
    IElement* m_host = nullptr;
};

END_BOOMER_NAMESPACE_EX(ui)
