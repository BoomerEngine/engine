/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: gizmos #]
***/

#include "build.h"
#include "gizmo.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

static ConfigProperty<float> cvGizmoHitTest("Editor.Gizmo", "HitDistance", 10.0f);

//---

IGizmoActionContext::~IGizmoActionContext()
{}

//---

IGizmo::IGizmo()
{}

float IGizmo::CalcDistanceToSegment(const Vector2& p, const Vector2& a, const Vector2& b)
{
    auto dir = b - a;
    auto len = dir.length();
    if (len >= SMALL_EPSILON)
    {
        auto n = dir.normalized();
        auto d = n | (p - a);

        if (d <= 0.0f)
        {
            return p.distance(a);
        }
        else if (d >= len)
        {
            return p.distance(b);
        }
        else
        {
            auto proj = a + n * d;
            return p.distance(proj);
        }
    }
    else
    {
        return p.distance(a);
    }
}

float IGizmo::GetHitTestDistance()
{
    return cvGizmoHitTest.get();
}

//--
    
END_BOOMER_NAMESPACE_EX(ed)
