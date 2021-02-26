/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\rect #]
***/

#include "build.h"

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_STRUCT(Rect);
    RTTI_BIND_NATIVE_COMPARE(Rect);
    RTTI_TYPE_TRAIT().zeroInitializationValid().noConstructor().noDestructor().fastCopyCompare();
    RTTI_PROPERTY(min).editable();
    RTTI_PROPERTY(max).editable();
RTTI_END_TYPE();

//---

Rect& Rect::merge(const Rect &rect)
{
    if (!rect.empty())
    {
        if (empty())
        {
            *this = rect;
        }
        else
        {
            min.x = std::min(min.x, rect.min.x);
            min.y = std::min(min.y, rect.min.y);
            max.x = std::max(max.x, rect.max.x);
            max.y = std::max(max.y, rect.max.y);
        }
    }

    return *this;
}

Rect& Rect::merge(const Point& point)
{
    min.x = std::min(min.x, point.x);
    min.y = std::min(min.y, point.y);
    max.x = std::max(max.x, point.x);
    max.y = std::max(max.y, point.y);
    return *this;
}

Rect& Rect::merge(int x, int y)
{
    min.x = std::min(min.x, x);
    min.y = std::min(min.y, y);
    max.x = std::max(max.x, x);
    max.y = std::max(max.y, y);
    return *this;
}

//--

Rect Min(const Rect& a, const Rect& b)
{
    return Rect(Min(a.min, b.min), Min(a.max, b.max));
}

Rect Max(const Rect& a, const Rect& b)
{
    return Rect(Max(a.min, b.min), Max(a.max, b.max));
}

Rect Clamp(const Rect& a, const Rect& limit)
{
    return Rect(Clamp(a.min, limit.min, limit.max), Clamp(a.max, limit.min, limit.max));
}

Rect Clamp(const Rect& a, int minF, int maxF)
{
    return Rect(Clamp(a.min, minF, maxF), Clamp(a.max, minF, maxF));
}

//--

static Rect EMPTY_R(std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), std::numeric_limits<int>::min(), std::numeric_limits<int>::min());
static Rect ZERO_R(0,0,0,0);
static Rect UNIT_R(0,0,1,1);

const Rect& Rect::EMPTY()
{
    return EMPTY_R;
}

const Rect& Rect::ZERO()
{
    return ZERO_R;
}

const Rect& Rect::UNIT()
{
    return UNIT_R;
}

//--

END_BOOMER_NAMESPACE()
