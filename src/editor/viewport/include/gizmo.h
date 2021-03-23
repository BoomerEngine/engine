/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: gizmos #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

/// transform action performed by the gizmos
class EDITOR_VIEWPORT_API IGizmoActionContext : public IReferencable
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
    virtual void preview(const Transform& deltaTransform) = 0;

    /// apply the transformation
    virtual void apply(const Transform& deltaTransform) = 0;

    //--

    /// process translation that we want to apply to the objects
    /// NOTE: all applied transformations are always non absolute
    /// NOTE: it's allowed (for snapping) to rotate objects when doing translation, thus the full transform is returned
    /// NOTE: returns true if the movement is allowed or false if it's not
    virtual bool filterTranslation(const Vector3& deltaTranslationInSpace, Transform& outTransform) const = 0;

    /// process rotation that we want to apply to the objects
    /// NOTE: returns true if the movement is allowed or false if it's not
    virtual bool filterRotation(const Angles& rotationAnglesInSpace, Transform& outTransform) const = 0;
};

//--

/// gizmo interface
class EDITOR_VIEWPORT_API IGizmo : public IReferencable
{
public:
    IGizmo();

    //--

    /// align gizmo with given reference space (usually just transforms vertices)
    virtual void transfom(const GizmoReferenceSpace& space, float scaleFactor) = 0;

    /// check collision with this gizmo, uses the captured viewport interface
    virtual bool hitTest(Point point, float& outMinHitDistance, const ui::ViewportCameraSetup& viewport, CursorType& outCursor) = 0;

    /// render this gizmo in the given frame, usually uses the debug rendering functionality
    virtual void render(rendering::FrameParams& frame, GizmoRenderMode mode) = 0;

    /// activate this gizmo, creates an input action
    virtual ui::InputActionPtr activate(Point point, const GizmoActionContextPtr& action) = 0;

    //--

    // calculate distance to line segment in 2D
    static float CalcDistanceToSegment(const Vector2& p, const Vector2& a, const Vector2& b);

    // get hit test distance for gizmo
    static float GetHitTestDistance();

    //--
};

//--

extern Transform TransformByOperation(const Transform& original, const GizmoReferenceSpace& referenceSpace, const Transform& transform);

//--

END_BOOMER_NAMESPACE_EX(ed)


