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

#include "base/input/include/inputStructures.h"
#include "base/image/include/image.h"

namespace ui
{
    //--

    class ButtonInputAction : public MouseInputAction
    {
    public:
        ButtonInputAction(ui::Button* button, const ElementArea& buttonArea)
            : MouseInputAction(button, base::input::KeyCode::KEY_MOUSE0)
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

        virtual InputActionResult onMouseMovement(const base::input::MouseMovementEvent& evt, const ElementWeakPtr& hoverStack) override
        {
            if (auto button = m_button.lock())
            {
                bool isPresed = m_buttonArea.contains(evt.absolutePosition());
                button->pressedState(isPresed);
            }

            return InputActionResult();
        }

    private:
        base::RefWeakPtr<ui::Button> m_button;
        ElementArea m_buttonArea;
    };

    //--

    RTTI_BEGIN_TYPE_CLASS(Button);
        RTTI_METADATA(ElementClassNameMetadata).name("Button");
    RTTI_END_TYPE();

    Button::Button(ButtonMode m)
        : m_mode(m)
        , OnClick(this, "OnClick"_id)
    {
        hitTest(HitTestState::Enabled);
        layoutMode(LayoutMode::Horizontal);
        mode(m);
    }

    Button::Button(base::StringView<char> txt, ButtonMode m)
        : m_mode(m)
        , OnClick(this, "OnClick"_id)
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
        allowFocusFromClick(!m_mode.test(ui::ButtonModeBit::NoFocus) && !m_mode.test(ui::ButtonModeBit::NoMouse));
        allowFocusFromKeyboard(!m_mode.test(ui::ButtonModeBit::NoKeyboard));
    }

    bool Button::handleKeyEvent(const base::input::KeyEvent& evt)
    {
        if (!m_mode.test(ui::ButtonModeBit::NoKeyboard))
        {
            if (evt.pressed())
            {
                if (evt.keyCode() == base::input::KeyCode::KEY_SPACE)
                {
                    pressedState(true);
                    return true;
                }
            }
            else if (evt.released())
            {
                if (evt.keyCode() == base::input::KeyCode::KEY_SPACE && pressed())
                {
                    inputActionFinished();
                    return true;
                }
            }
        }

        return TBaseClass::handleKeyEvent(evt);
    }

    InputActionPtr Button::handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt)
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
                    return base::CreateSharedPtr<ButtonInputAction>(this, area);
                }
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
        OnClick(m_toggled);
    }

    bool Button::handleTemplateProperty(base::StringView<char> name, base::StringView<char> value)
    {
        if (name == "text" || name == "label")
        {
            if (auto label = findChildByName<TextLabel>("ButtonText"))
            {
                label->text(value);
            }
            else
            {
                createNamedChild<TextLabel>("ButtonText"_id, value);
            }
            return true;
        }
        else if (name == "mode")
        {
            if (value == "Click")
            {
                m_mode -= ButtonModeBit::EVENT_MASK;
                m_mode |= ButtonModeBit::EventOnClick;
                return true;
            }
            else if (value == "ClickRelease")
            {
                m_mode -= ButtonModeBit::EVENT_MASK;
                m_mode |= ButtonModeBit::EventOnClickRelease;
                return true;
            }
            else if (value == "ClickReleaseAnywhere")
            {
                m_mode -= ButtonModeBit::EVENT_MASK;
                m_mode |= ButtonModeBit::EventOnClickReleaseAnywhere;
                return true;
            }
            else if (value == "DoubleClick")
            {
                m_mode -= ButtonModeBit::EVENT_MASK;
                m_mode |= ButtonModeBit::EventOnDoubleClick;
                return true;
            }
        }
        else if (name == "toggle")
        {
            m_mode |= ButtonModeBit::Toggle;

            if (value == "auto")
                m_mode |= ButtonModeBit::AutoToggle;
            return true;
        }
        else if (name == "onClick")
        {
            if (auto actionName = base::StringID(value))
            {
                bind("OnClick"_id) = [this, actionName]()
                {
                    runAction(actionName);
                };
            }
            return true;
        }

        return TBaseClass::handleTemplateProperty(name, value);
    }

    //--

} // ui
