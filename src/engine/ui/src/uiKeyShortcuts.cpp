/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: misc #]
*/

#include "build.h"
#include "uiElement.h"
#include "uiKeyShortcuts.h"

#include "core/containers/include/stringParser.h"
#include "core/containers/include/stringBuilder.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

class KeyNameMap : public ISingleton
{
    DECLARE_SINGLETON(KeyNameMap);

public:
    KeyNameMap()
    {
        m_keys.emplaceBack(InputKey::KEY_BACK, "Backspace");
        m_keys.emplaceBack(InputKey::KEY_TAB, "Tab");
        m_keys.emplaceBack(InputKey::KEY_RETURN, "Enter");
        m_keys.emplaceBack(InputKey::KEY_PAUSE, "Pause");
        m_keys.emplaceBack(InputKey::KEY_CAPITAL, "Capslock");
        m_keys.emplaceBack(InputKey::KEY_ESCAPE, "Esc");
        m_keys.emplaceBack(InputKey::KEY_ESCAPE, "Escape");
        m_keys.emplaceBack(InputKey::KEY_SPACE, "Space");
        m_keys.emplaceBack(InputKey::KEY_PRIOR, "PageUp");
        m_keys.emplaceBack(InputKey::KEY_NEXT, "PageDown");
        m_keys.emplaceBack(InputKey::KEY_END, "End");
        m_keys.emplaceBack(InputKey::KEY_HOME, "Home");
        m_keys.emplaceBack(InputKey::KEY_LEFT, "Left");
        m_keys.emplaceBack(InputKey::KEY_RIGHT, "Right");
        m_keys.emplaceBack(InputKey::KEY_UP, "Up");
        m_keys.emplaceBack(InputKey::KEY_DOWN, "Down");
        m_keys.emplaceBack(InputKey::KEY_INSERT, "Insert");
        m_keys.emplaceBack(InputKey::KEY_DELETE, "Delete");
        m_keys.emplaceBack(InputKey::KEY_0, "0");
        m_keys.emplaceBack(InputKey::KEY_1, "1");
        m_keys.emplaceBack(InputKey::KEY_2, "2");
        m_keys.emplaceBack(InputKey::KEY_3, "3");
        m_keys.emplaceBack(InputKey::KEY_4, "4");
        m_keys.emplaceBack(InputKey::KEY_5, "5");
        m_keys.emplaceBack(InputKey::KEY_6, "6");
        m_keys.emplaceBack(InputKey::KEY_7, "7");
        m_keys.emplaceBack(InputKey::KEY_8, "8");
        m_keys.emplaceBack(InputKey::KEY_9, "9");
        m_keys.emplaceBack(InputKey::KEY_A, "A");
        m_keys.emplaceBack(InputKey::KEY_B, "B");
        m_keys.emplaceBack(InputKey::KEY_C, "C");
        m_keys.emplaceBack(InputKey::KEY_D, "D");
        m_keys.emplaceBack(InputKey::KEY_E, "E");
        m_keys.emplaceBack(InputKey::KEY_F, "F");
        m_keys.emplaceBack(InputKey::KEY_G, "G");
        m_keys.emplaceBack(InputKey::KEY_H, "H");
        m_keys.emplaceBack(InputKey::KEY_I, "I");
        m_keys.emplaceBack(InputKey::KEY_J, "J");
        m_keys.emplaceBack(InputKey::KEY_K, "K");
        m_keys.emplaceBack(InputKey::KEY_L, "L");
        m_keys.emplaceBack(InputKey::KEY_M, "M");
        m_keys.emplaceBack(InputKey::KEY_N, "N");
        m_keys.emplaceBack(InputKey::KEY_O, "O");
        m_keys.emplaceBack(InputKey::KEY_P, "P");
        m_keys.emplaceBack(InputKey::KEY_Q, "Q");
        m_keys.emplaceBack(InputKey::KEY_R, "R");
        m_keys.emplaceBack(InputKey::KEY_S, "S");
        m_keys.emplaceBack(InputKey::KEY_T, "T");
        m_keys.emplaceBack(InputKey::KEY_U, "U");
        m_keys.emplaceBack(InputKey::KEY_V, "V");
        m_keys.emplaceBack(InputKey::KEY_W, "W");
        m_keys.emplaceBack(InputKey::KEY_X, "X");
        m_keys.emplaceBack(InputKey::KEY_Y, "Y");
        m_keys.emplaceBack(InputKey::KEY_Z, "Z");
        m_keys.emplaceBack(InputKey::KEY_NUMPAD0, "NumPad0");
        m_keys.emplaceBack(InputKey::KEY_NUMPAD1, "NumPad1");
        m_keys.emplaceBack(InputKey::KEY_NUMPAD2, "NumPad2");
        m_keys.emplaceBack(InputKey::KEY_NUMPAD3, "NumPad3");
        m_keys.emplaceBack(InputKey::KEY_NUMPAD4, "NumPad4");
        m_keys.emplaceBack(InputKey::KEY_NUMPAD5, "NumPad5");
        m_keys.emplaceBack(InputKey::KEY_NUMPAD6, "NumPad6");
        m_keys.emplaceBack(InputKey::KEY_NUMPAD7, "NumPad7");
        m_keys.emplaceBack(InputKey::KEY_NUMPAD8, "NumPad8");
        m_keys.emplaceBack(InputKey::KEY_NUMPAD9, "NumPad9");
        m_keys.emplaceBack(InputKey::KEY_NUMPAD_MULTIPLY, "NumPad*");
        m_keys.emplaceBack(InputKey::KEY_NUMPAD_ADD, "NumPad+");
        m_keys.emplaceBack(InputKey::KEY_NUMPAD_SEPARATOR, "NumPad.");
        m_keys.emplaceBack(InputKey::KEY_NUMPAD_SUBTRACT, "NumPad-");
        m_keys.emplaceBack(InputKey::KEY_NUMPAD_DECIMAL, "NumPad,");
        m_keys.emplaceBack(InputKey::KEY_NUMPAD_DIVIDE, "NumPad/");
        m_keys.emplaceBack(InputKey::KEY_F1, "F1");
        m_keys.emplaceBack(InputKey::KEY_F2, "F2");
        m_keys.emplaceBack(InputKey::KEY_F3, "F3");
        m_keys.emplaceBack(InputKey::KEY_F4, "F4");
        m_keys.emplaceBack(InputKey::KEY_F5, "F5");
        m_keys.emplaceBack(InputKey::KEY_F6, "F6");
        m_keys.emplaceBack(InputKey::KEY_F7, "F7");
        m_keys.emplaceBack(InputKey::KEY_F8, "F8");
        m_keys.emplaceBack(InputKey::KEY_F9, "F9");
        m_keys.emplaceBack(InputKey::KEY_F10, "F10");
        m_keys.emplaceBack(InputKey::KEY_F11, "F11");
        m_keys.emplaceBack(InputKey::KEY_F12, "F12");
        m_keys.emplaceBack(InputKey::KEY_RBRACKET, "RBracket");
        m_keys.emplaceBack(InputKey::KEY_LBRACKET, "LBracket");
    }

    InputKey code(StringView name) const
    {
        for (const auto& key : m_keys)
            if (0 == key.m_name.compareWithNoCase(name))
                return key.m_code;

        return InputKey::KEY_INVALID;
    }

    const StringBuf& name(const InputKey code) const
    {
        for (const auto& key : m_keys)
            if (key.m_code == code)
                return key.m_name;

        return StringBuf::EMPTY();
    }

private:
    struct Key
    {
        InputKey m_code;
        StringBuf m_name;

        INLINE Key(InputKey code, const StringBuf& name)
            : m_code(code)
            , m_name(name)
        {}
    };

    Array<Key> m_keys;

    virtual void deinit() override
    {
        m_keys.clear();
    }
};

//--

RTTI_BEGIN_CUSTOM_TYPE(KeyShortcut);
    RTTI_BIND_NATIVE_CTOR_DTOR(KeyShortcut);
    RTTI_BIND_NATIVE_COPY(KeyShortcut);
    RTTI_BIND_NATIVE_COMPARE(KeyShortcut);
    RTTI_BIND_NATIVE_PRINT_PARSE(KeyShortcut);
    //RTTI_BIND_CUSTOM_BINARY_SERIALIZATION(&prv::WriteBinary, &prv::ReadBinary);
    //RTTI_BIND_CUSTOM_XML_SERIALIZATION(&prv::WriteXML, &prv::ReadXML);
RTTI_END_TYPE();

KeyShortcut::KeyShortcut()
    : key(InputKey::KEY_INVALID)
    , shift(false)
    , ctrl(false)
    , alt(false)
{}

KeyShortcut::KeyShortcut(const InputKey key, bool shift, bool ctrl, bool alt)
    : key(key)
    , shift(shift)
    , ctrl(ctrl)
    , alt(alt)
{}

KeyShortcut::KeyShortcut(const char* txt)
    : KeyShortcut(StringView(txt))
{}

KeyShortcut::KeyShortcut(StringView txt)
{
    Parse(txt, *this);
}

bool KeyShortcut::operator==(const KeyShortcut& other) const
{
    return (key == other.key) && (alt == other.alt) && (shift == other.shift) && (ctrl == other.ctrl);
}

void KeyShortcut::print(IFormatStream& ret) const
{
    if (valid())
    {
        if (alt)
            ret.append("Alt+");
        if (shift)
            ret.append("Shift+");
        if (ctrl)
            ret.append("Ctrl+");
        ret << KeyNameMap::GetInstance().name(key);
    }
}

bool KeyShortcut::Parse(StringView txt, KeyShortcut& outKey)
{
    bool shift = false, ctrl = false, alt = false;

    StringParser parser(txt);
    while (parser.parseWhitespaces())
    {
        StringView part;
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
            auto keyCode = KeyNameMap::GetInstance().code(part);
            if (keyCode == InputKey::KEY_INVALID)
                break;

            outKey.key = keyCode;
            outKey.shift = shift;
            outKey.ctrl = ctrl;
            outKey.alt = alt;
            return true;
        }
    }

    return false;
}

bool KeyShortcut::matches(const InputKeyEvent& evt) const
{
    if (evt.keyCode() != key)
        return false;
    if (evt.keyMask().mask().test(InputKeyMaskBit::ANY_ALT) != alt)
        return false;
    if (evt.keyMask().mask().test(InputKeyMaskBit::ANY_SHIFT) != shift)
        return false;
    if (evt.keyMask().mask().test(InputKeyMaskBit::ANY_CTRL) != ctrl)
        return false;
    return true;
}

//--

KeyShortcutTable::KeyShortcutTable(IElement* owner)
    : m_host(owner)
{}

KeyShortcutTable::~KeyShortcutTable()
{
    clear();
}

void KeyShortcutTable::clear()
{
    m_shortcuts.clearPtr();
}

void KeyShortcutTable::remove(KeyShortcut key)
{
    for (auto i : m_shortcuts.indexRange())
    {
        if (m_shortcuts[i]->shortcut == key)
        {
            delete m_shortcuts[i];
            m_shortcuts.erase(i);
            break;
        }    
    }
}

EventFunctionBinder KeyShortcutTable::bindShortcut(KeyShortcut key)
{
    DEBUG_CHECK_RETURN_EX_V(key, "Invalid shortcut key", nullptr);

    for (auto* entry : m_shortcuts)
        if (entry->shortcut == key)
            return EventFunctionBinder(&entry->callback);

    auto* entry = new Shortcut;
    entry->shortcut = key;
    m_shortcuts.pushBack(entry);

    return EventFunctionBinder(&entry->callback);
}

bool KeyShortcutTable::processKeyEvent(const InputKeyEvent& evt) const
{
    for (const auto* entry : m_shortcuts)
        if (entry->shortcut.matches(evt))
            return entry->callback(StringID(), m_host, m_host, nullptr);

    return false;
}

//---

END_BOOMER_NAMESPACE_EX(ui)
 