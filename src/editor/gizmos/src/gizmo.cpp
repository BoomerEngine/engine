/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: gizmos #]
***/

#include "build.h"
#include "gizmo.h"

BEGIN_BOOMER_NAMESPACE(ed)

//---

namespace config
{
    base::ConfigProperty<float> cvGizmoHitTest("Editor.Gizmo", "HitDistance", 10.0f);
} // config

//---

IGizmoActionContext::~IGizmoActionContext()
{}

//---

IGizmo::IGizmo()
{}

float IGizmo::CalcDistanceToSegment(const base::Vector2& p, const base::Vector2& a, const base::Vector2& b)
{
    auto dir = b - a;
    auto len = dir.length();
    if (len >= SMALL_EPSILON)
    {
        auto n = dir.normalized();
        auto d = base::Dot(n, p - a);

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
    return config::cvGizmoHitTest.get();
}

//--
    
END_BOOMER_NAMESPACE(ed)
