/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\stack2D #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//--

void TransformStack2D::transform(float a, float b, float c, float d, float e, float f)
{
    transform(XForm2D(a, b, c, d, e, f));
}

void TransformStack2D::transform(const float* t)
{
    transform(XForm2D(t));
}

void TransformStack2D::offset(const Vector2& o)
{
    offset(o.x, o.y);
}

void TransformStack2D::reset(float a, float b, float c, float d, float e, float f)
{
    reset(XForm2D(a, b, c, d, e, f));
}

void TransformStack2D::reset(const float* t)
{
    reset(XForm2D(t));
}

//--

END_BOOMER_NAMESPACE()
