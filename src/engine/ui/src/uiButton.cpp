/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\simple #]
***/

#include "build.h"
#include "uiButton.h"
#include "uiGeometryBuilder.h"
#include "uiInputAction.h"
#include "uiTextLabel.h"

#include "core/input/include/inputStructures.h"
#include "core/image/include/image.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

class ButtonInputAction : public MouseInputAction
{
public:
    ButtonInputAction(Button* button, const ElementArea& buttonArea)
        : MouseInputAction(button, input::KeyCode::KEY_MOUSE0)
        , m_button(button)
        , m_buttonArea(buttonArea)
    {
        if (auto button = m_button.lock())
            button->pressedState(true);
    }

    virtual ~ButtonInputAction()
    {
        if (auto button = m_button.lock())
            button->pressedState(false);
    }

    virtual void onFinished()
    {
        if (auto button = m_button.lock())
            button->inputActionFinished();
    }

    virtual InputActionResult onMouseMovement(const input::MouseMovementEvent& evt, const ElementWeakPtr& hoverStack) override
    {
        if (auto button = m_button.lock())
        {
            bool isPresed = m_buttonArea.contains(evt.absolutePosition());
            button->pressedState(isPresed);
        }

        return InputActionResult();
    }

private:
    RefWeakPtr<Button> m_button;
    ElementArea m_buttonArea;
};

//--

RTTI_BEGIN_TYPE_CLASS(Button);
    RTTI_METADATA(ElementClassNameMetadata).name("Button");
RTTI_END_TYPE();

Button::Button(ButtonMode m)
    : m_mode(m)
{
    hitTest(HitTestState::Enabled);
    layoutMode(LayoutMode::Horizontal);
    allowFocusFromKeyboard(true);
    allowFocusFromClick(true);
    mode(m);
}

Button::Button(StringView txt, ButtonMode m)
    : m_mode(m)
{
    hitTest(HitTestState::Enabled);
    layoutMode(LayoutMode::Horizontal);
    createChild<TextLabel>(txt);
    mode(m);
}

void Button::toggle(bool flag)
{
    if (flag != m_toggled)
    {
        m_toggled = flag;
        if (flag)
            addStyleClass("toggled"_id);
        else
            removeStyleClass("toggled"_id);
    }
}

void Button::mode(ButtonMode mode)
{
    m_mode = mode;
    allowFocusFromClick(!m_mode.test(ButtonModeBit::NoFocus) && !m_mode.test(ButtonModeBit::NoMouse));
    allowFocusFromKeyboard(!m_mode.test(ButtonModeBit::NoKeyboard));
}

bool Button::handleKeyEvent(const input::KeyEvent& evt)
{
    if (!m_mode.test(ButtonModeBit::NoKeyboard))
    {
        if (evt.pressed())
        {
            if (evt.keyCode() == input::KeyCode::KEY_SPACE)
            {
                if (m_mode.test(ButtonModeBit::EventOnClick))
                    clicked();
                else
                    pressedState(true);
                return true;
            }
        }
        else if (evt.released())
        {
            if (evt.keyCode() == input::KeyCode::KEY_SPACE && pressed())
            {
                inputActionFinished();
                return true;
            }
        }
    }

    return TBaseClass::handleKeyEvent(evt);
}

InputActionPtr Button::handleMouseClick(const ElementArea& area, const input::MouseClickEvent& evt)
{
    if (isEnabled())
    {
        if (evt.leftDoubleClicked())
        {
            if (m_mode.test(ButtonModeBit::EventOnDoubleClick))
                clicked();

            return IInputAction::CONSUME();
        }
        else if (evt.leftClicked())
        {
            if (m_mode.test(ButtonModeBit::EventOnClick))
            {
                clicked();
                return IInputAction::CONSUME();
            }
            else if (m_mode.test(ButtonModeBit::EventOnClickRelease) || m_mode.test(ButtonModeBit::EventOnClickReleaseAnywhere))
            {
                return RefNew<ButtonInputAction>(this, area);
            }
        }
        else if (evt.midClicked())
        {
            clickedSecondary();
            return IInputAction::CONSUME();
        }
    }
    return InputActionPtr();
}

void Button::pressedState(bool flag)
{
    if (m_pressed != flag)
    {
        m_pressed = flag;

        if (flag)
            addStylePseudoClass("pressed"_id);
        else
            removeStylePseudoClass("pressed"_id);
    }
}

void Button::inputActionFinished()
{
    if (m_pressed || m_mode.test(ButtonModeBit::EventOnClickReleaseAnywhere))
    {
        pressedState(false);
        clicked();
    }
}

void Button::clicked()
{
    // auto toggle
    if (m_mode.test(ButtonModeBit::AutoToggle) && m_mode.test(ButtonModeBit::Toggle))
        toggle(!toggled());

    // pass to handler
    call(EVENT_CLICKED, m_toggled);
}

void Button::clickedSecondary()
{
    call(EVENT_CLICKED_SECONDARY, m_toggled);
}

//--

END_BOOMER_NAMESPACE_EX(ui)
