/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: gizmos #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(ed)

//---

/// transform action performed by the gizmos
class EDITOR_GIZMOS_API IGizmoActionContext : public base::IReferencable
{
public:
    virtual ~IGizmoActionContext();

    //--

    /// get the original host
    virtual const IGizmoHost* host() const = 0;

    /// get captured reference space at the start of action
    virtual const GizmoReferenceSpace& capturedReferenceSpace() const = 0;

    //--

    /// revert all object transforms to captured ones, cancels the transform action
    virtual void revert() = 0;

    /// preview the transformation, without making anything final
    virtual void preview(const base::Transform& deltaTransform) = 0;

    /// apply the transformation
    virtual void apply(const base::Transform& deltaTransform) = 0;

    //--

    /// process translation that we want to apply to the objects
    /// NOTE: all applied transformations are always non absolute
    /// NOTE: it's allowed (for snapping) to rotate objects when doing translation, thus the full transform is returned
    /// NOTE: returns true if the movement is allowed or false if it's not
    virtual bool filterTranslation(const base::Vector3& deltaTranslationInSpace, base::Transform& outTransform) const = 0;

    /// process rotation that we want to apply to the objects
    /// NOTE: returns true if the movement is allowed or false if it's not
    virtual bool filterRotation(const base::Angles& rotationAnglesInSpace, base::Transform& outTransform) const = 0;
};

//--

/// gizmo interface
class EDITOR_GIZMOS_API IGizmo : public base::IReferencable
{
public:
    IGizmo();

    //--

    /// align gizmo with given reference space (usually just transforms vertices)
    virtual void transfom(const GizmoReferenceSpace& space, float scaleFactor) = 0;

    /// check collision with this gizmo, uses the captured viewport interface
    virtual bool hitTest(base::Point point, float& outMinHitDistance, const ui::CameraViewportSetup& viewport, base::input::CursorType& outCursor) = 0;

    /// render this gizmo in the given frame, usually uses the debug rendering functionality
    virtual void render(rendering::scene::FrameParams& frame, GizmoRenderMode mode) = 0;

    /// activate this gizmo, creates an input action
    virtual ui::InputActionPtr activate(base::Point point, const GizmoActionContextPtr& action) = 0;

    //--

    // calculate distance to line segment in 2D
    static float CalcDistanceToSegment(const base::Vector2& p, const base::Vector2& a, const base::Vector2& b);

    // get hit test distance for gizmo
    static float GetHitTestDistance();

    //--
};

//--

extern base::AbsoluteTransform TransformByOperation(const base::AbsoluteTransform& original, const GizmoReferenceSpace& referenceSpace, const base::Transform& transform);

//--

END_BOOMER_NAMESPACE(ed)


