/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\simple #]
***/

#pragma once

#include "uiElement.h"

BEGIN_BOOMER_NAMESPACE(ui)

//--

DECLARE_UI_EVENT(EVENT_CLICKED, bool)
DECLARE_UI_EVENT(EVENT_CLICKED_SECONDARY, bool)

//--

class ButtonInputAction;

/// button mode flags
enum class ButtonModeBit : uint16_t
{
    Repeat = FLAG(0), // repeat OnClick event every couple of ms (around 100)
    NoMouse = FLAG(1), // do not allow mouse clicks (but button can still be pressed by keyboard)
    NoKeyboard = FLAG(2), // do not allow keyboard "clicks" (but we can still click it by mouse)
    NoFocus = FLAG(3), // do not take focus on the button
    Toggle = FLAG(4), // this is a toggle button that has two states - normal and toggled
    AutoToggle = FLAG(5), // automatically toggle the button state 

    EventOnClick = FLAG(6), // reports event on click (mouse down event), no release needed
    EventOnClickRelease = FLAG(7), // reports event on click + release on same item (this is what the majority of Button are using)
    EventOnClickReleaseAnywhere = FLAG(8), // reports event on click + release even if the release event is not done while hovering the item
    EventOnDoubleClick = FLAG(9), // report event on double-click 

    SecondEventOnMiddleClick = FLAG(10), // hacky flag that a second even should be generated on middle click

    EVENT_MASK = EventOnClick | EventOnClickRelease | EventOnClickReleaseAnywhere | EventOnDoubleClick
};

typedef base::DirectFlags<ButtonModeBit> ButtonMode;

/// a simple button
class BASE_UI_API Button : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(Button, IElement);

public:
    Button(ButtonMode mode = ButtonModeBit::EventOnClickRelease);
    Button(base::StringView text, ButtonMode mode = ButtonModeBit::EventOnClickRelease);

    ///---

    /// get button mode
    INLINE ButtonMode mode() const { return m_mode; }

    /// change button mode
    void mode(ButtonMode mode);

    ///---

    /// is the button pressed currently ?
    INLINE bool pressed() const { return m_pressed; }

    /// is this button toggled ?
    INLINE bool toggled() const { return m_toggled; }

    /// enable/disable toggled state
    void toggle(bool flag);
        
    ///---

protected:
    // IElement
    virtual InputActionPtr handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt) override;
    virtual bool handleKeyEvent(const base::input::KeyEvent& evt) override;

    ButtonMode m_mode;
    bool m_pressed = false;
    bool m_toggled = false;
        
    void pressedState(bool isPressed);
    void inputActionFinished();

    virtual void clicked();
    virtual void clickedSecondary();

    friend class ButtonInputAction;
};

END_BOOMER_NAMESPACE(ui)