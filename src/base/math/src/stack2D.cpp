/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\stack2D #]
***/

#include "build.h"

namespace base
{
    //--

    void TransformStack2D::clear()
    {
        head = Entry();
        stack.reset();
    }

    void TransformStack2D::push()
    {
        stack.emplaceBack(head);
    }

    void TransformStack2D::pop()
    {
        if (!stack.empty())
        {
            head = stack.back();
            stack.popBack();
        }
        else
        {
            head = Entry();
        }
    }

    void TransformStack2D::transform(const XForm2D& t)
    {
        head.transform *= t;
        head.classification = head.transform.classify();
    }

    void TransformStack2D::offset(float x, float y)
    {
        if (head.classification != 0 || x != 0.0f || y != 0.0f) // we only' care to unsettle the full identity
        {
            head.transform.t[4] += x;
            head.transform.t[5] += y;
            head.classification |= XForm2DClass::HasTransform;
        }
    }

    void TransformStack2D::rotate(float angle)
    {
        if ((head.classification & XForm2DClass::HasScaleRotation) || angle != 0.0f)
        {
            head.transform *= XForm2D::BuildRotation(angle);
            head.classification = head.transform.classify();
        }
    }

    void TransformStack2D::skewX(float angle)
    {
        head.transform *= XForm2D::BuildSkewX(angle);
        head.classification = XForm2DClass::Full;
    }

    void TransformStack2D::skewY(float angle)
    {
        head.transform *= XForm2D::BuildSkewY(angle);
        head.classification = XForm2DClass::Full;
    }

    void TransformStack2D::scale(float x, float y)
    {
        if ((head.classification & XForm2DClass::HasScaleRotation) || x != 1.0f || y != 1.0f)
        {
            head.transform.t[0] *= x;
            head.transform.t[1] *= x;
            head.transform.t[2] *= y;
            head.transform.t[3] *= y;
            head.classification |= XForm2DClass::HasScaleRotation;
        }
    }

    void TransformStack2D::scale(float u)
    {
        if ((head.classification & XForm2DClass::HasScaleRotation) || u != 1.0f)
        {
            head.transform.t[0] *= u;
            head.transform.t[1] *= u;
            head.transform.t[2] *= u;
            head.transform.t[3] *= u;
            head.classification |= XForm2DClass::HasScaleRotation;
        }
    }

    void TransformStack2D::identity()
    {
        head.transform.identity();
        head.classification = 0;
    }

    void TransformStack2D::reset(const XForm2D& t)
    {
        head.transform = t;
        head.classification = t.classify();
    }

    //--

} // base