/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: gizmos #]
***/

#pragma once
#
namespace ed
{
    
    /// reference space for the gizmo operations
    struct EDITOR_GIZMOS_API GizmoReferenceSpace
    {
    public:
        GizmoReferenceSpace();
        GizmoReferenceSpace(GizmoSpace space, const AbsolutePosition& rootPoint);
        GizmoReferenceSpace(GizmoSpace space, const AbsoluteTransform& transform);
        GizmoReferenceSpace(const GizmoReferenceSpace& other) = default;
        GizmoReferenceSpace(GizmoReferenceSpace&& other) = default;
        GizmoReferenceSpace& operator=(const GizmoReferenceSpace& other) = default;
        GizmoReferenceSpace& operator=(GizmoReferenceSpace&& other) = default;

        //---

        /// get the space type
        INLINE GizmoSpace space() const { return m_space; }

        /// get the reference space origin
        INLINE const AbsolutePosition& origin() const { return m_transform.position();}

        /// get the reference transform
        INLINE const AbsoluteTransform& absoluteTransform() const { return m_transform; }

        /// get transform axis
        INLINE const Vector3& axis(uint32_t index) const { ASSERT(index<3); return m_axes[index]; }

        //---

        /// compute a absolute space location for given position in the reference frame
        AbsolutePosition calcAbsolutePositionForLocal(const Vector3& localPosition) const;

        /// apply transform in this gizmo space
        AbsoluteTransform transform(const AbsoluteTransform& original, const Transform& transform) const;

        //---

    private:
        /// reference root position for the space (origin)
        AbsoluteTransform m_transform;

        /// axes (X,Y,Z)
        Vector3 m_axes[3];

        /// type of reference space
        GizmoSpace m_space;

        //--

        void cacheAxes();
    };

} // ed
