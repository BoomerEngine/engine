/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input\generic #]
***/

#include "build.h"
#include "inputContext.h"
#include "inputStructures.h"
#include "inputGenericKeyboard.h"

BEGIN_BOOMER_NAMESPACE(base::input)

//--

GenericKeyboard::GenericKeyboard(IContext* owner, DeviceID id)
    : m_keyRepeatDelay(0.333f)
    , m_keyRepeatPeriod(0.033f)
    , m_context(owner)
    , m_id(id)
{
    memzero(&m_pressedKeys, sizeof(m_pressedKeys));
}

void GenericKeyboard::repeatAndDelay(float delay, float repeatRate)
{
    m_keyRepeatDelay = NativeTimeInterval(delay);
    m_keyRepeatPeriod = NativeTimeInterval(repeatRate ? 1.0f / repeatRate : 1.0f);
}

KeyMask GenericKeyboard::currentKeyMask() const
{
    KeyMask mask;

    if (m_pressedKeys[(uint16_t)KeyCode::KEY_LEFT_ALT])
        mask |= KeyMaskBit::LEFT_ALT;
    if (m_pressedKeys[(uint16_t)KeyCode::KEY_RIGHT_ALT])
        mask |= KeyMaskBit::RIGHT_ALT;
    if (m_pressedKeys[(uint16_t)KeyCode::KEY_LEFT_CTRL])
        mask |= KeyMaskBit::LEFT_CTRL;
    if (m_pressedKeys[(uint16_t)KeyCode::KEY_RIGHT_CTRL])
        mask |= KeyMaskBit::RIGHT_CTRL;
    if (m_pressedKeys[(uint16_t)KeyCode::KEY_LEFT_SHIFT])
        mask |= KeyMaskBit::LEFT_SHIFT;
    if (m_pressedKeys[(uint16_t)KeyCode::KEY_RIGHT_SHIFT])
        mask |= KeyMaskBit::RIGHT_SHIFT;

    return mask;
}

void GenericKeyboard::keyDown(KeyCode keyCode)
{
    auto& keyState = m_pressedKeys[(uint16_t)keyCode];
    if (!keyState)
    {
        auto keyMask  = currentKeyMask();
        auto evt  = base::RefNew<KeyEvent>(DeviceType::Keyboard, m_id, keyCode, true, false, keyMask);
        m_context->inject(evt);

        // insert the pressed key to the repeat list
        auto nextRepeat  = NativeTimePoint::Now() + m_keyRepeatDelay;
        m_repeatKey = RepeatKey(keyCode, keyMask, nextRepeat);
        keyState = true;
    }
}

void GenericKeyboard::keyUp(KeyCode keyCode)
{
    auto& keyState = m_pressedKeys[(uint16_t)keyCode];
    if (keyState)
    {
        auto evt  = base::RefNew<KeyEvent>(DeviceType::Keyboard, m_id, keyCode, false, false, currentKeyMask());
        m_context->inject(evt);

        m_repeatKey.reset();
        keyState = false;
    }
}

void GenericKeyboard::charDown(KeyScanCode scanCode)
{
    auto evt  = base::RefNew<CharEvent>(m_id, scanCode, false, currentKeyMask());
    m_context->inject(evt);
}

void GenericKeyboard::reset()
{
    // send the release events to the owning windows
    for (uint32_t i = 0; i < (uint16_t)KeyCode::KEY_MAX; ++i)
    {
        auto owningWindowId = m_pressedKeys[i];
        if (0 != owningWindowId)
        {
            auto evt = base::RefNew<KeyEvent>(DeviceType::Keyboard, m_id, (KeyCode)i, false, false, KeyMask());
            m_context->inject(evt);

            m_pressedKeys[i] = false;
        }
    }
        
    m_repeatKey.reset();
}

void GenericKeyboard::update()
{
    auto currentTime = NativeTimePoint::Now();
    if (m_repeatKey.valid())
    {
        uint32_t maxRepeat = m_repeatKey.m_maxRepeat;
        auto distToNextKeyPress = (m_repeatKey.m_nextRepeatTime - currentTime).toSeconds();
        if (distToNextKeyPress <= 0.0)
        {
            while (m_repeatKey.m_nextRepeatTime < currentTime)
            {
                m_repeatKey.m_nextRepeatTime = m_repeatKey.m_nextRepeatTime + m_keyRepeatPeriod;
                m_repeatKey.m_maxRepeat += 1;

                auto evt = base::RefNew<KeyEvent>(DeviceType::Keyboard, m_id, m_repeatKey.m_keyCode, true, true, m_repeatKey.m_keyMask);
                m_context->inject(evt);
            }

            // handle the overflow case
            if (maxRepeat == 0)
                m_repeatKey.m_nextRepeatTime = currentTime + m_keyRepeatPeriod;
        }
    }
}

//--

END_BOOMER_NAMESPACE(base::input)