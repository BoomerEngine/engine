/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "editor_gizmos_glue.inl"

namespace ed
{

    ///---

    enum class GizmoRenderMode : uint8_t
    {
        Idle, // no gizmos are active or selected,
        Hover, // user hovers the mouse over this gizmo but does not select it yet
        Active, // this gizmo has been activated by the user
        OtherActive, // other gizmo is active
    };

    ///---

    enum class GizmoSpace : uint8_t
    {
        World, // use transformations in world space
        Local, // use transformations in local space
        Parent, // use transformations in parent space
        View, // use transformations in view space
    };

    class IGizmo;
    typedef base::RefPtr<IGizmo> GizmoPtr;
    typedef base::RefWeakPtr<IGizmo> GizmoWeakPtr;

    class GizmoGroup;
    typedef base::RefPtr<GizmoGroup> GizmoGroupPtr;

    struct GizmoReferenceSpace;
    
    class IGizmoActionContext;
    typedef base::RefPtr<IGizmoActionContext> GizmoActionContextPtr;

    class IGizmoHost;

    ///---

} // ed
