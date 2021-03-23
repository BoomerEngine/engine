/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: misc #]
***/

#pragma once

#include "uiElement.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

/// result of input action processing
struct ENGINE_UI_API InputActionResult
{
public:
    enum class ResultCode : uint8_t
    {
        Continue, // continue current action
        Exit, // exit current action
        Replace, // switch to new action
    };

        INLINE InputActionResult()
            : m_code(ResultCode::Continue)
        {}

        INLINE InputActionResult(std::nullptr_t)
            : m_code(ResultCode::Exit)
        {}

        INLINE InputActionResult(const InputActionPtr& actionPtr)
            : m_code(actionPtr ? ResultCode::Replace : ResultCode::Exit)
            , m_ptr(actionPtr)
        {}

        INLINE ResultCode resultCode() const { return m_code; }

        INLINE const InputActionPtr& nextAction() const { return m_ptr; }

private:
    ResultCode m_code;
    InputActionPtr m_ptr;
};

/// An transient input action handler used to interact with UI controls
/// Input actions are created with direct mouse interactions
/// When input action is created all input events are first routed to the action which has a priority
/// The input handler can be externally canceled
class ENGINE_UI_API IInputAction : public IReferencable
{
    RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IInputAction);

public:
    IInputAction(IElement* ptr);
    virtual ~IInputAction();

    // get element associated with the action
    INLINE ElementPtr element() const { return m_element.lock(); }

    ///--

    /// Canceled - called when something happens that makes this input action no longer valid
    /// NOTE: this is also called when mouse capture gets lost
    virtual void onCanceled();

    /// Update mouse cursor
    virtual void onUpdateCursor(input::CursorType& outCursorType);

    /// Update the window refresh policy while using this action
    /// Allows action to adjust the global UI rendering mode to optimize quality of user interaction
    virtual void onUpdateRedrawPolicy(WindowRedrawPolicy& outRedrawPolicy);

    /// Render stuff related to this action
    /// We assume that the input action is always dynamic so we allow a dynamic content to be directly rendered
    virtual void onRender(canvas::Canvas& canvas);

    /// Render 3d stuff, subject to owner implementation
    virtual void onRender3D(rendering::FrameParams& frame);

    ///---

    /// Handle key event
    virtual InputActionResult onKeyEvent(const input::KeyEvent& evt);

    /// Handle axis event
    virtual InputActionResult onAxisEvent(const input::AxisEvent& evt);

    /// Handle mouse click
    virtual InputActionResult onMouseEvent(const input::MouseClickEvent& evt, const ElementWeakPtr& hoveredElement);

    /// Handle mouse movement
    virtual InputActionResult onMouseMovement(const input::MouseMovementEvent& evt, const ElementWeakPtr& hoveredElement);

    /// Update internal stuff related to this input action, called once per tick
    virtual InputActionResult onUpdate(float dt);

    ///---

    /// Check if we allow the update of the hover element
    virtual bool allowsHoverTracking() const;

    /// Does this action require full mouse capture (hidden cursor, delta mode only)
    virtual bool fullMouseCapture() const;

    ///--

    /// special input action to return to indicate that the event was consumed but no further action will be taken (ie. no input action)
    static RefPtr<IInputAction> CONSUME();

private:
    ElementWeakPtr m_element;
};

// Mouse based input action
// Automatically exists when user releases button used to create the action
class ENGINE_UI_API MouseInputAction : public IInputAction
{
public:
    MouseInputAction(IElement* ptr, const input::KeyCode mouseCode, bool cancelWithEscape = true);

    virtual void onFinished();

    virtual InputActionResult onKeyEvent(const input::KeyEvent& evt) override final;
    virtual InputActionResult onMouseEvent(const input::MouseClickEvent& evt, const ElementWeakPtr& hoverStack) override final;

private:
    input::KeyCode m_controllingButton;
    bool m_escapeCanCancel;
};

END_BOOMER_NAMESPACE_EX(ui)
