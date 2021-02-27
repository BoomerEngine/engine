/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: gizmos #]
***/

#include "build.h"
#include "gizmoHelpers.h"
#include "gizmoReferenceSpace.h"
#include "gizmoGroup.h"
#include "viewportCameraSetup.h"

#include "engine/rendering/include/debug.h"
#include "engine/rendering/include/params.h"
#include "core/input/include/inputStructures.h"
#include "engine/ui/include/uiInputAction.h"
#include "engine/ui/include/uiElement.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---
namespace config
{
    ConfigProperty<float> cvGizmoTransScale("Editor.Gizmo", "TranslationArrowLength", 100.0f);
    ConfigProperty<float> cvGizmoTransWidth("Editor.Gizmo","TranslationLineWidth", 2.0f);
} // config

//---

/// get color for given axis
static Color GetColorForAxis(uint8_t axis)
{
    switch (axis)
    {
        case 0: return Color::RED;
        case 1: return Color::GREEN;
        case 2: return Color::BLUE;
    }

    return Color::WHITE;
}

/// get color for given axis
static Color GetColorForAxisInMode(uint8_t axis, GizmoRenderMode mode)
{
    auto nativeColor = GetColorForAxis(axis);

    switch (mode)
    {
        case GizmoRenderMode::Idle:
            nativeColor *= 0.7f;
            nativeColor.a = 255;
            return nativeColor;

        case GizmoRenderMode::Hover:
            return nativeColor;

        case GizmoRenderMode::Active:
            return Color::YELLOW;

        case GizmoRenderMode::OtherActive:
            nativeColor *= 0.5f;
            nativeColor.a = 255;
            return nativeColor;
    }

    return nativeColor;
}

/// get the arrow directions for given space
static void GetArrowDirections(uint8_t axis, Vector3& n, Vector3& u, Vector3& v)
{
    switch (axis)
    {
        case 0:
        {
            n = Vector3::EX();
            u = Vector3::EY();
            v = Vector3::EZ();
            break;
        }

        case 1:
        {
            n = Vector3::EY();
            u = Vector3::EX();
            v = Vector3::EZ();
            break;
        }

        case 2:
        {
            n = Vector3::EZ();
            u = Vector3::EX();
            v = Vector3::EY();
            break;
        }
    }
}

///--

class GizmoTranslationAxisInputAction : public ui::IInputAction
{
public:
    GizmoTranslationAxisInputAction(const GizmoActionContextPtr& action, uint8_t rawAxis)
        : IInputAction(action->host()->gizmoHost_element())
        , m_referenceAxisIndex(rawAxis)
        , m_action(action)
    {
        DEBUG_CHECK(action->host()->gizmoHost_element());
    }

    virtual void onCanceled() override final
    {
        m_action->revert();
    }

    virtual void onUpdateCursor(input::CursorType& outCursorType) override final
    {
        outCursorType = input::CursorType::SizeAll;
    }

    virtual void onRender3D(rendering::FrameParams& frame) override final
    {
        if (m_lastComputedTransformValid)
        {
            // LINE
            {
                const auto& space = m_action->capturedReferenceSpace();
                auto startPos = space.calcAbsolutePositionForLocal(Vector3::ZERO()).approximate();
                auto endPos = space.calcAbsolutePositionForLocal(m_lastComputedTransform.T).approximate();

                rendering::DebugDrawer dd(frame.geometry.overlay);
                dd.color(Color::WHITE);
                dd.line(startPos, endPos);
            }

            // TEXT
            /*{
                // get the translation vector
                auto localDelta = m_lastComputedTransform.translation();
                auto localLength = localDelta.length();

                // place the text in the mid point
                ScreenCanvas dd(frame);
                auto textPosition = m_transaction->capturedReferenceSpace().calcAbsolutePositionForLocal(localDelta / 2.0f).approximate();

                Vector3 screenPos;
                if (dd.calcScreenPosition(textPosition, screenPos))
                {
                    dd.lineColor(Color::WHITE);
                    dd.alignHorizontal(0);
                    dd.alignVertical(0);
                    dd.textBox(screenPos.x, screenPos.y, TempString("{}", Prec(localLength, 2)));
                }
            }*/
        }
    }

    virtual ui::InputActionResult onKeyEvent(const input::KeyEvent& evt) override final
    {
        if (evt.pressed() && evt.keyCode() == input::KeyCode::KEY_ESCAPE)
            return nullptr;

        return ui::InputActionResult();
    }

    virtual ui::InputActionResult onMouseEvent(const input::MouseClickEvent& evt, const ui::ElementWeakPtr& hoveredElement)
    {
        if (evt.leftReleased())
        {
            // we can only apply transform if it is valid
            if (m_lastComputedTransformValid)
                m_action->apply(m_lastComputedTransform);

            return nullptr;
        }
        else
        {
            // do not allow other events to propagate
            return IInputAction::CONSUME();
        }
    }

    static bool CalcBestReferencePlane(const Vector3& origin, const Vector3& cameraDir, const Vector3& cameraPos, const Vector3& movementAxis, Plane& outPlane)
    {
        // find the orientation that contains the movement axis
        auto dot = Dot(movementAxis, cameraDir);
        if (abs(dot) > (1.0f - SMALL_EPSILON))
            return false;

        // calculate the normal
        auto planeSide = Cross(cameraDir, movementAxis);
        auto planeNormal = Cross(movementAxis, planeSide);
        planeNormal.normalize();

        // calculate the plane
        outPlane = Plane(planeNormal, origin);

        // make sure the camera position is always on the positive side of the plane
        if (outPlane.testPoint(cameraPos) == SIDE_Negative)
            outPlane = -outPlane;

        return true;
    }

    static Vector3 GetAxisVector(uint8_t axis, float delta)
    {
        switch (axis)
        {
            case 0: return Vector3(delta, 0, 0);
            case 1: return Vector3(0, delta, 0);
            case 2: return Vector3(0, 0, delta);
        }

        return Vector3::ZERO();
    }

    virtual ui::InputActionResult onMouseMovement(const input::MouseMovementEvent& evt, const ui::ElementWeakPtr& hoveredElement) override
    {
        static const Vector3 AXIS_VECTORS[3] = {
            Vector3::EX(), Vector3::EY(), Vector3::EZ()
        };

        // camera direction
        const auto cameraPos = m_action->host()->gizmoHost_camera().position();
        const auto cameraDir = m_action->host()->gizmoHost_camera().directionForward();
        auto origin = m_action->capturedReferenceSpace().origin().approximate();

        // calculate the position in viewport
        auto clientPos = evt.absolutePosition() - element()->cachedDrawArea().absolutePosition();

        // get the movement axis
        auto movementAxis = m_action->capturedReferenceSpace().axis(m_referenceAxisIndex);

        // reset valid flag
        m_lastComputedTransformValid = false;

        // build plane that passes through the gizmo for given movement axis
        // NOTE: camera orientation may prevent us from finding this plane
        Plane referencePlane;
        if (CalcBestReferencePlane(origin, cameraDir, cameraPos, movementAxis, referencePlane))
        {
            // calculate ray at pixel
            Vector3 rayDir, rayStart;
            const ui::ViewportCameraSetup viewport(m_action->host()->gizmoHost_camera(), m_action->host()->gizmoHost_viewportSize().x, m_action->host()->gizmoHost_viewportSize().y);
            if (viewport.worldSpaceRayForClientPixel(clientPos.x, clientPos.y, rayStart, rayDir))
            {
                // calculate intersection with drag plane
                Vector3 intersectionPoint;
                float intersectionDistance;
                if (referencePlane.intersect(rayStart, rayDir, 1000.0f, &intersectionDistance, &intersectionPoint))
                {
                    // calculate the projected distance along the axis
                    float movementAlign = Dot(movementAxis, cameraDir);
                    float movementDelta = Dot(movementAxis, intersectionPoint - origin);

                    // remember the first offset as the reference point (since we can click on the gizmo anywhere)
                    if (!m_initialOffsetSaved)
                    {
                        m_initialOffset = movementDelta;
                        m_initialOffsetSaved = true;
                    }

                    // filter and update
                    auto adjustedMovementDelta = movementDelta - m_initialOffset;
                    auto adjustedMovementDeltaTranslation = GetAxisVector(m_referenceAxisIndex, adjustedMovementDelta);

                    Transform computedTransform;
                    if (m_action->filterTranslation(adjustedMovementDeltaTranslation, computedTransform))
                    {
                        m_lastComputedTransformValid = true;
                        m_lastComputedTransform = computedTransform;

                        m_action->preview(computedTransform);
                    }
                }
            }
        }

        // keep alive
        return ui::InputActionResult();
    }

private:
    float m_initialOffset = 0.0f;
    bool m_initialOffsetSaved = false;

    bool m_lastComputedTransformValid = false;
    Transform m_lastComputedTransform;

    GizmoActionContextPtr m_action;

    uint8_t m_referenceAxisIndex = 0;
};

///--

class GizmoTranslateAxis : public IGizmo
{
public:
    GizmoTranslateAxis(uint8_t axis)
        : m_axis(axis)
    {
        // get the arrow setup
        Vector3 an,au,av;
        GetArrowDirections(axis, an, au, av);

        // calculate various points on the arrow
        float len = 1.0f;
        auto head = an * len;
        auto base = head * 0.8f;
        auto u = au * len * 0.05f;
        auto v = av * len * 0.05f;

        // create arrow
        auto& points = m_lines.m_lines;
        points.pushBack(Vector3::ZERO());
        points.pushBack(head);
        points.pushBack(head);
        points.pushBack(base + u + v);
        points.pushBack(head);
        points.pushBack(base + u - v);
        points.pushBack(head);
        points.pushBack(base - u + v);
        points.pushBack(head);
        points.pushBack(base - u - v);

        // create detail in the arrow head
        points.pushBack(base - u + v);
        points.pushBack(base + u + v);
        points.pushBack(base + u + v);
        points.pushBack(base + u - v);
        points.pushBack(base + u - v);
        points.pushBack(base - u - v);
        points.pushBack(base - u - v);
        points.pushBack(base - u + v);
    }

    virtual void transfom(const GizmoReferenceSpace& space, float scaleFactor) override final
    {
        auto lineScale = config::cvGizmoTransScale.get();
        m_lines.transfom(space, scaleFactor * lineScale);
    }

    virtual bool hitTest(Point point, float& outMinHitDistance, const ui::ViewportCameraSetup& viewport, input::CursorType& outCursor) override final
    {
        return m_lines.hitTest(point, viewport, GetHitTestDistance(), outMinHitDistance);
    }

    virtual void render(rendering::FrameParams& frame, GizmoRenderMode mode) override final
    {
        auto lineWidth = config::cvGizmoTransWidth.get();
        auto color = GetColorForAxisInMode(m_axis, mode);

        m_lines.render(frame, color, lineWidth);

        if (mode == GizmoRenderMode::Hover)
        {
            const char* axisName[3] = { "X", "Y", "Z" };
            StringBuf caption = TempString("Move {}", axisName[m_axis]);

            /*ScreenCanvas dd(frame);

            Vector3 screenPos;
            if (dd.calcScreenPosition(localToWorld.position().approximate(), screenPos))
            {
                dd.lineColor(Color::WHITE);
                dd.alignHorizontal(0);
                dd.alignVertical(0);
                dd.textBox(screenPos.x, screenPos.y, caption);
            }*/
        }
    }

    virtual ui::InputActionPtr activate(Point point, const GizmoActionContextPtr& action) override final
    {
        if (action)
            return RefNew<GizmoTranslationAxisInputAction>(action, m_axis);

        return nullptr;
    }

private:
    GizmoLineShape m_lines;
    uint8_t m_axis = 0;
};

//---

GizmoGroupPtr CreateTranslationGizmos(const IGizmoHost* host, uint8_t axisMask /*= 7*/)
{
    DEBUG_CHECK_RETURN_V(host, nullptr);
    DEBUG_CHECK_RETURN_V((axisMask & 7) != 0, nullptr);

    Array<GizmoPtr> gizmos;

    for (uint8_t axis = 0; axis <= 2; ++axis)
        if (axisMask & (1 << axis))
            gizmos.pushBack(RefNew<GizmoTranslateAxis>(axis));

    return RefNew<GizmoGroup>(host, std::move(gizmos));
}

//---
    
END_BOOMER_NAMESPACE_EX(ed)
