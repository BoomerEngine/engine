/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: gizmos #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(ed)

//--

/// gizmo host
class EDITOR_GIZMOS_API IGizmoHost : public base::NoCopy
{
public:
    virtual ~IGizmoHost();

    /// get owning UI element
    virtual ui::IElement* gizmoHost_element() const = 0;

    /// do we have selection
    virtual bool gizmoHost_hasSelection() const = 0;

    /// get visual camera
    virtual const rendering::scene::Camera& gizmoHost_camera() const = 0;

    /// get viewport size
    virtual base::Point gizmoHost_viewportSize() const = 0;

    /// calculate reference space
    virtual GizmoReferenceSpace gizmoHost_referenceSpace() const = 0;

    /// being transform action on active selection
    virtual GizmoActionContextPtr gizmoHost_startAction() const = 0;
};

//--

/// group of gizmos that user can choose from with mouse
/// NOTE: gizmo group is always owned by a rendering panel
class EDITOR_GIZMOS_API GizmoGroup : public base::IReferencable
{
public:
    GizmoGroup(const IGizmoHost* host, Array<GizmoPtr>&& gizmos);

    //---

    // hist of the gizmo group
    INLINE const IGizmoHost* host() const { return m_host; }

    //---

    // rendering active state
    void render(rendering::scene::FrameParams& frame);

    // process mouse move event, returns true if we have active hover
    bool updateHover(const base::Point& clientPoint);

    // ask for current cursor, returns true if we have active hover
    bool updateCursor(base::input::CursorType& outCursor);

    // process click
    ui::InputActionPtr activate();

    //--

private:
    const IGizmoHost* m_host = nullptr;

    //--

    Array<GizmoPtr> m_gizmos;
    IGizmo* m_activeGizmo = nullptr;
    IGizmo* m_hoverGizmo = nullptr;

    //--

    Point m_hoverClientPos;
    base::input::CursorType m_hoverCursor;

    //--

    ui::InputActionWeakPtr m_activeInput;

    //--

    IGizmo* findGizmoUnderCursor(const base::Point& point, base::input::CursorType& outCursor) const;
    GizmoRenderMode determineRenderMode(const IGizmo* gizmo) const;

    void alignGizmos();
};

//--

/// create gizmo group for translation operations
extern EDITOR_GIZMOS_API GizmoGroupPtr CreateTranslationGizmos(const IGizmoHost* host, uint8_t axisMask = 7);

//--

END_BOOMER_NAMESPACE(ed)
