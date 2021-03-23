/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input\generic #]
***/

#pragma once

#include "inputDevice.h"
#include "inputStructures.h"

BEGIN_BOOMER_NAMESPACE()

/// generic keyboard device
class CORE_INPUT_API GenericKeyboard : public NoCopy
{
public:
    GenericKeyboard(IInputContext* owner, InputDeviceID id = 0);

    // reset state
    void reset();

    // process the repeat key
    void update();

    // set keyboard repeat and delay
    void repeatAndDelay(float delay, float repeatRate);

    // emit a key down event
    void keyDown(InputKey keyCode);

    // emit a key up event
    void keyUp(InputKey keyCode);

    // emit a char
    void charDown(KeyScanCode scanCode);

    // get the control key mask
    InputKeyMask currentKeyMask() const; // get the current SHIFT/CTRL/ALT key mask

private:
    bool m_pressedKeys[(uint16_t)InputKey::KEY_MAX]; // windows owning given pressed keys

    struct RepeatKey
    {
        InputKey m_keyCode;
        InputKeyMask m_keyMask;
        NativeTimePoint m_nextRepeatTime;
        bool m_pressed;
        uint32_t m_maxRepeat;

        INLINE RepeatKey()
            : m_keyCode((InputKey)0)
            , m_maxRepeat(1)
            , m_pressed(false)
        {}

        INLINE RepeatKey(InputKey code, InputKeyMask keyMask, const NativeTimePoint& nextRepeatTime)
            : m_keyCode(code)
            , m_keyMask(keyMask)
            , m_nextRepeatTime(nextRepeatTime)
            , m_maxRepeat(1)
            , m_pressed(true)
        {}

        INLINE void reset()
        {
            m_keyCode = (InputKey)0;
            m_maxRepeat = 1;
            m_pressed = false;
        }

        INLINE bool valid() const
        {
            return m_pressed;
        }
    };

    RepeatKey m_repeatKey;

    NativeTimeInterval m_keyRepeatDelay;
    NativeTimeInterval m_keyRepeatPeriod;

    IInputContext* m_context;
    InputDeviceID m_id;
};

END_BOOMER_NAMESPACE()
