/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\simple #]
***/

#include "build.h"
#include "uiDragger.h"
#include "uiInputAction.h"
#include "uiTextLabel.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

///---

ConfigProperty<int> cvDraggerDeadZone("Editor", "DraggerDeadZone", 10);
ConfigProperty<int> cvDraggerPixelRatio("Editor", "DraggerPixelStep", 4);

///---

/// input action for dragging
class DraggerInputAction : public IInputAction
{
public:
    DraggerInputAction(Dragger* box, const Point& initialPosition)
        : IInputAction(box)
        , m_box(box)
        , m_pixelDelta(0)
    {}

    virtual InputActionResult onKeyEvent(const InputKeyEvent& evt) override
    {
        // revert back to old value on ESC key
        if (evt.pressed() && evt.keyCode() == InputKey::KEY_ESCAPE)
        {
            m_box->call(EVENT_VALUE_DRAG_CANCELED);
            return nullptr;
        }

        return InputActionResult(); // continue
    }

    virtual InputActionResult onMouseEvent(const InputMouseClickEvent& evt, const ElementWeakPtr& hoverStack) override
    {
        if (evt.leftReleased())
        {
            m_box->call(EVENT_VALUE_DRAG_FINISHED);
            return nullptr;
        }

        return InputActionResult(); // continue
    }

    virtual InputActionResult onMouseMovement(const InputMouseMovementEvent& evt, const ElementWeakPtr& hoverStack) override
    {
        const auto pixelStep = std::max<int>(1, cvDraggerPixelRatio.get());

        int stepScale = 10;
        if (evt.keyMask().isCtrlDown())
            stepScale = 1;
        else if (evt.keyMask().isShiftDown())
            stepScale = 100;

        m_pixelDelta += (int)evt.delta().y;

        int valueDelta = 0;
        if (m_pixelDelta > cvDraggerDeadZone.get())
        {
            const auto steps = (m_pixelDelta - cvDraggerDeadZone.get()) / pixelStep;
            if (steps > 0)
            {
                m_pixelDelta -= steps * pixelStep;
                valueDelta += steps * stepScale;
            }
        }
        else if (m_pixelDelta < -cvDraggerDeadZone.get())
        {
            const auto steps = (cvDraggerDeadZone.get() + m_pixelDelta) / pixelStep;
            if (steps < 0)
            {
                m_pixelDelta -= steps * pixelStep;
                valueDelta += steps * stepScale;
            }
        }

        if (valueDelta)
            m_box->call(EVENT_VALUE_DRAG_STEP, -valueDelta);

        return InputActionResult(); // continue
    }

    virtual bool allowsHoverTracking() const override
    {
        return false;
    }

    virtual bool fullMouseCapture() const override
    {
        return true;
    }

private:
    Dragger* m_box;
    int64_t m_pixelDelta;
};

///---

Dragger::Dragger()
{
    hitTest(true);
    enableAutoExpand(false, false);
}

InputActionPtr Dragger::handleMouseClick(const ElementArea &area, const InputMouseClickEvent &evt)
{
    if (evt.leftClicked())
    {
        call(EVENT_VALUE_DRAG_STARTED);
        return RefNew<DraggerInputAction>(this, evt.absolutePosition());
    }

    return nullptr;
}

bool Dragger::handleMouseWheel(const InputMouseMovementEvent &evt, float delta)
{
    return false;
}

bool Dragger::handleCursorQuery(const ElementArea &area, const Position &absolutePosition, CursorType &outCursorType) const
{
    outCursorType = CursorType::SizeNS;
    return true;
}

RTTI_BEGIN_TYPE_NATIVE_CLASS(Dragger);
    RTTI_METADATA(ElementClassNameMetadata).name("Dragger");
RTTI_END_TYPE();

//--

END_BOOMER_NAMESPACE_EX(ui)
