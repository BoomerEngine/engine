/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: misc #]
***/

#include "build.h"
#include "uiElement.h"
#include "uiInputAction.h"
#include "core/input/include/inputStructures.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IInputAction);
RTTI_END_TYPE();

IInputAction::IInputAction(IElement* ptr)
    : m_element(ptr)
{}

IInputAction::~IInputAction()
{}

void IInputAction::onCanceled()
{}

void IInputAction::onRender(canvas::Canvas& canvas)
{}

void IInputAction::onRender3D(rendering::FrameParams& frame)
{}

void IInputAction::onUpdateCursor(input::CursorType& outCursorType)
{
    outCursorType = input::CursorType::Default;
}

void IInputAction::onUpdateRedrawPolicy(WindowRedrawPolicy& outRedrawPolicy)
{
    // nothing
}

InputActionResult IInputAction::onKeyEvent(const input::KeyEvent& evt)
{
    return InputActionResult();
}

InputActionResult IInputAction::onMouseEvent(const input::MouseClickEvent& evt, const ElementWeakPtr& hoverStack)
{
    return InputActionResult();
}

InputActionResult IInputAction::onMouseMovement(const input::MouseMovementEvent& evt, const ElementWeakPtr& hoverStack)
{
    return InputActionResult();
}

InputActionResult IInputAction::onUpdate(float dt)
{
    return InputActionResult();
}

bool IInputAction::fullMouseCapture() const
{
    return false;
}

bool IInputAction::allowsHoverTracking() const
{
    return false;
}

//--

class ConsumeInputAction : public IInputAction
{
public:
    ConsumeInputAction()
        : IInputAction(nullptr)
    {}

    virtual InputActionResult onKeyEvent(const input::KeyEvent& evt) override
    {
        return nullptr;
    }

    virtual InputActionResult onMouseEvent(const input::MouseClickEvent& evt, const ElementWeakPtr& hoverStack) override
    {
        return nullptr;
    }

    virtual InputActionResult onMouseMovement(const input::MouseMovementEvent& evt, const ElementWeakPtr& hoverStack) override
    {
        return nullptr;
    }

    virtual InputActionResult onUpdate(float dt) override
    {
        return nullptr;
    }
};

class ConsumeActionRegistry : public ISingleton
{
    DECLARE_SINGLETON(ConsumeActionRegistry);

public:
    ConsumeActionRegistry()
    {
        m_consumeAction = RefNew<ConsumeInputAction>();
    }

    INLINE const RefPtr<IInputAction>& action()
    {
        return m_consumeAction;
    }

private:
    RefPtr<IInputAction> m_consumeAction;

    virtual void deinit() override
    {
        m_consumeAction.reset();
    }
};

RefPtr<IInputAction> IInputAction::CONSUME()
{
    return ConsumeActionRegistry::GetInstance().action();
}


//---

MouseInputAction::MouseInputAction(IElement* ptr, input::KeyCode mouseCode, bool cancelWithEscape /*= true*/)
    : IInputAction(ptr)
    , m_controllingButton(mouseCode)
    , m_escapeCanCancel(cancelWithEscape)
{}

InputActionResult MouseInputAction::onKeyEvent(const input::KeyEvent& evt)
{
    if (m_escapeCanCancel && evt.keyCode() == input::KeyCode::KEY_ESCAPE && evt.pressed())
    {
        onCanceled();
        return InputActionResult(nullptr);
    }
    return InputActionResult();
}

void MouseInputAction::onFinished()
{
}

InputActionResult MouseInputAction::onMouseEvent(const input::MouseClickEvent& evt, const ElementWeakPtr& hoverStack)
{
    if (evt.keyCode() == m_controllingButton && evt.type() == input::MouseEventType::Release)
    {
        onFinished();
        return InputActionResult(nullptr);
    }

    return InputActionResult();
}

//---

END_BOOMER_NAMESPACE_EX(ui)
