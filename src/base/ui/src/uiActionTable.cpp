/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: misc #]
*/

#include "build.h"
#include "uiElement.h"
#include "uiActionTable.h"

#include "base/containers/include/stringParser.h"
#include "base/containers/include/stringBuilder.h"

BEGIN_BOOMER_NAMESPACE(ui)

//--

class ActionKeyMap : public base::ISingleton
{
    DECLARE_SINGLETON(ActionKeyMap);

public:
    ActionKeyMap()
    {
        m_keys.emplaceBack(base::input::KeyCode::KEY_BACK, "Backspace");
        m_keys.emplaceBack(base::input::KeyCode::KEY_TAB, "Tab");
        m_keys.emplaceBack(base::input::KeyCode::KEY_RETURN, "Enter");
        m_keys.emplaceBack(base::input::KeyCode::KEY_PAUSE, "Pause");
        m_keys.emplaceBack(base::input::KeyCode::KEY_CAPITAL, "Capslock");
        m_keys.emplaceBack(base::input::KeyCode::KEY_ESCAPE, "Esc");
        m_keys.emplaceBack(base::input::KeyCode::KEY_ESCAPE, "Escape");
        m_keys.emplaceBack(base::input::KeyCode::KEY_SPACE, "Space");
        m_keys.emplaceBack(base::input::KeyCode::KEY_PRIOR, "PageUp");
        m_keys.emplaceBack(base::input::KeyCode::KEY_NEXT, "PageDown");
        m_keys.emplaceBack(base::input::KeyCode::KEY_END, "End");
        m_keys.emplaceBack(base::input::KeyCode::KEY_HOME, "Home");
        m_keys.emplaceBack(base::input::KeyCode::KEY_LEFT, "Left");
        m_keys.emplaceBack(base::input::KeyCode::KEY_RIGHT, "Right");
        m_keys.emplaceBack(base::input::KeyCode::KEY_UP, "Up");
        m_keys.emplaceBack(base::input::KeyCode::KEY_DOWN, "Down");
        m_keys.emplaceBack(base::input::KeyCode::KEY_INSERT, "Insert");
        m_keys.emplaceBack(base::input::KeyCode::KEY_DELETE, "Delete");
        m_keys.emplaceBack(base::input::KeyCode::KEY_0, "0");
        m_keys.emplaceBack(base::input::KeyCode::KEY_1, "1");
        m_keys.emplaceBack(base::input::KeyCode::KEY_2, "2");
        m_keys.emplaceBack(base::input::KeyCode::KEY_3, "3");
        m_keys.emplaceBack(base::input::KeyCode::KEY_4, "4");
        m_keys.emplaceBack(base::input::KeyCode::KEY_5, "5");
        m_keys.emplaceBack(base::input::KeyCode::KEY_6, "6");
        m_keys.emplaceBack(base::input::KeyCode::KEY_7, "7");
        m_keys.emplaceBack(base::input::KeyCode::KEY_8, "8");
        m_keys.emplaceBack(base::input::KeyCode::KEY_9, "9");
        m_keys.emplaceBack(base::input::KeyCode::KEY_A, "A");
        m_keys.emplaceBack(base::input::KeyCode::KEY_B, "B");
        m_keys.emplaceBack(base::input::KeyCode::KEY_C, "C");
        m_keys.emplaceBack(base::input::KeyCode::KEY_D, "D");
        m_keys.emplaceBack(base::input::KeyCode::KEY_E, "E");
        m_keys.emplaceBack(base::input::KeyCode::KEY_F, "F");
        m_keys.emplaceBack(base::input::KeyCode::KEY_G, "G");
        m_keys.emplaceBack(base::input::KeyCode::KEY_H, "H");
        m_keys.emplaceBack(base::input::KeyCode::KEY_I, "I");
        m_keys.emplaceBack(base::input::KeyCode::KEY_J, "J");
        m_keys.emplaceBack(base::input::KeyCode::KEY_K, "K");
        m_keys.emplaceBack(base::input::KeyCode::KEY_L, "L");
        m_keys.emplaceBack(base::input::KeyCode::KEY_M, "M");
        m_keys.emplaceBack(base::input::KeyCode::KEY_N, "N");
        m_keys.emplaceBack(base::input::KeyCode::KEY_O, "O");
        m_keys.emplaceBack(base::input::KeyCode::KEY_P, "P");
        m_keys.emplaceBack(base::input::KeyCode::KEY_Q, "Q");
        m_keys.emplaceBack(base::input::KeyCode::KEY_R, "R");
        m_keys.emplaceBack(base::input::KeyCode::KEY_S, "S");
        m_keys.emplaceBack(base::input::KeyCode::KEY_T, "T");
        m_keys.emplaceBack(base::input::KeyCode::KEY_U, "U");
        m_keys.emplaceBack(base::input::KeyCode::KEY_V, "V");
        m_keys.emplaceBack(base::input::KeyCode::KEY_W, "W");
        m_keys.emplaceBack(base::input::KeyCode::KEY_X, "X");
        m_keys.emplaceBack(base::input::KeyCode::KEY_Y, "Y");
        m_keys.emplaceBack(base::input::KeyCode::KEY_Z, "Z");
        m_keys.emplaceBack(base::input::KeyCode::KEY_NUMPAD0, "NumPad0");
        m_keys.emplaceBack(base::input::KeyCode::KEY_NUMPAD1, "NumPad1");
        m_keys.emplaceBack(base::input::KeyCode::KEY_NUMPAD2, "NumPad2");
        m_keys.emplaceBack(base::input::KeyCode::KEY_NUMPAD3, "NumPad3");
        m_keys.emplaceBack(base::input::KeyCode::KEY_NUMPAD4, "NumPad4");
        m_keys.emplaceBack(base::input::KeyCode::KEY_NUMPAD5, "NumPad5");
        m_keys.emplaceBack(base::input::KeyCode::KEY_NUMPAD6, "NumPad6");
        m_keys.emplaceBack(base::input::KeyCode::KEY_NUMPAD7, "NumPad7");
        m_keys.emplaceBack(base::input::KeyCode::KEY_NUMPAD8, "NumPad8");
        m_keys.emplaceBack(base::input::KeyCode::KEY_NUMPAD9, "NumPad9");
        m_keys.emplaceBack(base::input::KeyCode::KEY_NUMPAD_MULTIPLY, "NumPad*");
        m_keys.emplaceBack(base::input::KeyCode::KEY_NUMPAD_ADD, "NumPad+");
        m_keys.emplaceBack(base::input::KeyCode::KEY_NUMPAD_SEPARATOR, "NumPad.");
        m_keys.emplaceBack(base::input::KeyCode::KEY_NUMPAD_SUBTRACT, "NumPad-");
        m_keys.emplaceBack(base::input::KeyCode::KEY_NUMPAD_DECIMAL, "NumPad,");
        m_keys.emplaceBack(base::input::KeyCode::KEY_NUMPAD_DIVIDE, "NumPad/");
        m_keys.emplaceBack(base::input::KeyCode::KEY_F1, "F1");
        m_keys.emplaceBack(base::input::KeyCode::KEY_F2, "F2");
        m_keys.emplaceBack(base::input::KeyCode::KEY_F3, "F3");
        m_keys.emplaceBack(base::input::KeyCode::KEY_F4, "F4");
        m_keys.emplaceBack(base::input::KeyCode::KEY_F5, "F5");
        m_keys.emplaceBack(base::input::KeyCode::KEY_F6, "F6");
        m_keys.emplaceBack(base::input::KeyCode::KEY_F7, "F7");
        m_keys.emplaceBack(base::input::KeyCode::KEY_F8, "F8");
        m_keys.emplaceBack(base::input::KeyCode::KEY_F9, "F9");
        m_keys.emplaceBack(base::input::KeyCode::KEY_F10, "F10");
        m_keys.emplaceBack(base::input::KeyCode::KEY_F11, "F11");
        m_keys.emplaceBack(base::input::KeyCode::KEY_F12, "F12");
        m_keys.emplaceBack(base::input::KeyCode::KEY_RBRACKET, "RBracket");
        m_keys.emplaceBack(base::input::KeyCode::KEY_LBRACKET, "LBracket");
    }

    base::input::KeyCode code(base::StringView name) const
    {
        for (const auto& key : m_keys)
            if (0 == key.m_name.compareWithNoCase(name))
                return key.m_code;

        return base::input::KeyCode::KEY_INVALID;
    }

    const base::StringBuf& name(const base::input::KeyCode code) const
    {
        for (const auto& key : m_keys)
            if (key.m_code == code)
                return key.m_name;

        return base::StringBuf::EMPTY();
    }

private:
    struct Key
    {
        base::input::KeyCode m_code;
        base::StringBuf m_name;

        INLINE Key(base::input::KeyCode code, const base::StringBuf& name)
            : m_code(code)
            , m_name(name)
        {}
    };

    base::Array<Key> m_keys;

    virtual void deinit() override
    {
        m_keys.clear();
    }
};

//--

KeyShortcut::KeyShortcut()
    : m_key(base::input::KeyCode::KEY_INVALID)
    , m_shift(false)
    , m_ctrl(false)
    , m_alt(false)
{}

KeyShortcut::KeyShortcut(const base::input::KeyCode key, bool shift, bool ctrl, bool alt)
    : m_key(key)
    , m_shift(shift)
    , m_ctrl(ctrl)
    , m_alt(alt)
{}

KeyShortcut::KeyShortcut(const char* txt)
    : KeyShortcut(base::StringView(txt))
{}

KeyShortcut::KeyShortcut(base::StringView txt)
{
    bool shift = false, ctrl = false, alt = false;
    base::StringParser parser(txt);
    for (;;)
    {
        base::StringView part;
        if (!parser.parseString(part, "+"))
            break;

        if (part == "Shift")
        {
            shift = true;

            if (!parser.parseKeyword("+"))
                break;
        }
        else if (part == "Ctrl")
        {
            ctrl = true;

            if (!parser.parseKeyword("+"))
                break;
        }
        else if (part == "Alt")
        {
            alt = true;

            if (!parser.parseKeyword("+"))
                break;
        }
        else
        {
            auto keyCode = ActionKeyMap::GetInstance().code(part);
            if (keyCode == base::input::KeyCode::KEY_INVALID)
                break;

            m_key = keyCode;
            m_shift = shift;
            m_ctrl = ctrl;
            m_alt = alt;
        }
    }
}

void KeyShortcut::print(base::IFormatStream& ret) const
{
    if (valid())
    {
        if (m_alt)
            ret.append("Alt+");
        if (m_shift)
            ret.append("Shift+");
        if (m_ctrl)
            ret.append("Ctrl+");
        ret << ActionKeyMap::GetInstance().name(m_key);
    }
}

bool KeyShortcut::matches(const base::input::KeyEvent& evt) const
{
    if (evt.keyCode() != m_key)
        return false;
    if (evt.keyMask().mask().test(base::input::KeyMaskBit::ANY_ALT) != m_alt)
        return false;
    if (evt.keyMask().mask().test(base::input::KeyMaskBit::ANY_SHIFT) != m_shift)
        return false;
    if (evt.keyMask().mask().test(base::input::KeyMaskBit::ANY_CTRL) != m_ctrl)
        return false;
    return true;
}

//--

ActionTable::ActionTable(IElement* owner)
    : m_host(owner)
{}

ActionTable::~ActionTable()
{
    clear();
}

void ActionTable::clear()
{
    m_actionMap.clearPtr();
}

void ActionTable::remove(base::StringID name)
{
    DEBUG_CHECK_EX(name, "Invalid action name");
    if (auto* action = m_actionMap.find(name))
    {
        m_actionMap.remove(name);
        delete action;
    }
}

void ActionTable::bindShortcut(base::StringID name, KeyShortcut shortcut)
{
    DEBUG_CHECK_EX(shortcut, "Invalid shortcut");
    DEBUG_CHECK_EX(name, "Invalid action name");
    if (shortcut && name)
    {
        for (const auto& info : m_shortcuts)
        if (info.shortcut == shortcut && info.name == name)
            return;

        auto& info = m_shortcuts.emplaceBack();
        info.shortcut = shortcut;
        info.name = name;
    }
}

const ActionInfo* ActionTable::find(base::StringID name) const
{
    ActionInfo* ret = nullptr;
    m_actionMap.find(name, ret);
    return ret;
}

ActionInfo* ActionTable::get(base::StringID name)
{
    DEBUG_CHECK_EX(name, "Invalid action name");
    ActionInfo* ret = nullptr;
    if (name)
    {
        if (!m_actionMap.find(name, ret))
        {
            ret = new ActionInfo;
            ret->name = name;
            m_actionMap[name] = ret;
        }
    }
    return ret;
}

EventFunctionBinder ActionTable::bindCommand(base::StringID name)
{
    if (auto* info = get(name))
        return EventFunctionBinder(&info->runFunc);
    return EventFunctionBinder(nullptr);
}

EventFunctionBinder ActionTable::bindFilter(base::StringID name)
{
    if (auto* info = get(name))
        return EventFunctionBinder(&info->enabledFunc);
    return EventFunctionBinder(nullptr);
}

EventFunctionBinder ActionTable::bindToggle(base::StringID name)
{
    if (auto* info = get(name))
        return EventFunctionBinder(&info->toggledFunc);
    return EventFunctionBinder(nullptr);
}

ActionStatusFlags ActionTable::status(base::StringID name) const
{
    ActionStatusFlags ret;

    if (const auto* info = find(name))
    {
        if (info->runFunc)
            ret |= ActionStatusBit::Defined;

        if (info->enabledFunc)
        {
            if (info->enabledFunc(name, m_host, m_host, info->data))
                ret |= ActionStatusBit::Enabled;
        }
        else
        {
            ret |= ActionStatusBit::Enabled;
        }

        if (info->toggledFunc)
        {
            if (info->toggledFunc(name, m_host, m_host, info->data))
                ret |= ActionStatusBit::Toggled;
        }
    }

    return ret;
}

bool ActionTable::run(base::StringID name, IElement* source) const
{
    if (const auto* info = find(name))
    {
        if (info->runFunc)
            return info->runFunc(name, source, m_host, info->data);
    }

    return false;
}

bool ActionTable::processKeyEvent(const base::input::KeyEvent& evt) const
{
    for (const auto& it : m_shortcuts)
        if (it.shortcut.matches(evt))
            return run(it.name, m_host);

    return false;
}

//---    

END_BOOMER_NAMESPACE(ui)
 