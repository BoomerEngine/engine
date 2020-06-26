/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\xform2D #]
***/

#include "build.h"
#include "base/containers/include/stringBuilder.h"

namespace base
{
    //--

    RTTI_BEGIN_TYPE_STRUCT(XForm2D);
        RTTI_BIND_NATIVE_COMPARE(XForm2D);
        RTTI_TYPE_TRAIT().noDestructor().fastCopyCompare(); // we must be constructed to identity

        RTTI_PROPERTY(t);
    RTTI_END_TYPE();

    //--

    XForm2D XForm2D::inverted() const
    {
        auto d = det();
        if (d > -SMALL_EPSILON && d < SMALL_EPSILON)
            return IDENTITY(); // no inverse possible

        XForm2D ret;
        auto invdet = 1.0 / d;
        ret.t[0] = (float)(t[3] * invdet);
        ret.t[2] = (float)(-t[2] * invdet);
        ret.t[4] = (float)(((double)t[2] * t[5] - (double)t[3] * t[4]) * invdet);
        ret.t[1] = (float)(-t[1] * invdet);
        ret.t[3] = (float)(t[0] * invdet);
        ret.t[5] = (float)(((double)t[1] * t[4] - (double)t[0] * t[5]) * invdet);
        return ret;
    }

    //--

    XForm2DClass XForm2D::classify() const
    {
        bool hasTranslation = (t[4] != 0.0f || t[5] != 0.0f);
        bool hasScaleRotation = (t[0] != 0.0f || t[1] != 0.0f || t[2] != 0.0f || t[3] != 1.0f);

        uint8_t ret = 0;
        if (hasTranslation) ret |= XForm2DClass::HasTransform;
        if (hasScaleRotation) ret |= XForm2DClass::HasScaleRotation;
        return (XForm2DClass)ret;
    }

    //--

    Matrix XForm2D::toMatrix() const
    {
        return Matrix(
            t[0], t[2], 0.0f, t[4],
            t[1], t[3], 0.0f, t[5],
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    }

    Matrix33 XForm2D::toMatrix33() const
    {
        return Matrix33(
                t[0], t[2], t[4],
                t[1], t[3], t[5],
                0.0f, 0.0f, 1.0f);
    }

    XForm2D XForm2D::BuildRotation(float cwAngleInRadians)
    {
        XForm2D ret;

        float c = std::cos(cwAngleInRadians);
        float s = std::sin(cwAngleInRadians);
        ret.t[0] = c;
        ret.t[1] = s;
        ret.t[2] = -s;
        ret.t[3] = c;
        return ret;
    }

    XForm2D XForm2D::BuildScale(float uniformScale)
    {
        XForm2D ret;
        ret.t[0] = uniformScale;
        ret.t[3] = uniformScale;
        return ret;
    }

    XForm2D XForm2D::BuildScale(float scaleX, float scaleY)
    {
        XForm2D ret;
        ret.t[0] = scaleX;
        ret.t[3] = scaleY;
        return ret;
    }

    XForm2D XForm2D::BuildTranslation(const Vector2& ofs)
    {
        XForm2D ret;
        ret.t[4] = ofs.x;
        ret.t[5] = ofs.y;
        return ret;
    }

    XForm2D XForm2D::BuildTranslation(float x, float y, float s)
    {
        XForm2D ret;
        ret.t[0] = s;
        ret.t[3] = s;
        ret.t[4] = x;
        ret.t[5] = y;
        return ret;
    }

    XForm2D XForm2D::BuildSkewX(float a)
    {
        XForm2D ret;
        ret.t[0] = 1.0f;
        ret.t[1] = 0.0f;
        ret.t[2] = std::tan(a);
        ret.t[3] = 1.0f;
        ret.t[4] = 0.0f;
        ret.t[5] = 0.0f;
        return ret;
    }

    XForm2D XForm2D::BuildSkewY(float a)
    {
        XForm2D ret;
        ret.t[0] = 1.0f;
        ret.t[1] = std::tan(a);
        ret.t[2] = 0.0f;
        ret.t[3] = 1.0f;
        ret.t[4] = 0.0f;
        ret.t[5] = 0.0f;
        return ret;
    }

    //--

    static XForm2D ZERO_XF(0, 0, 0, 0, 0, 0);
    static XForm2D IDENTITY_XF(1, 0, 0, 1, 0, 0);
    static XForm2D ROTATECW_XF(0, 1, -1, 0, 0, 0);
    static XForm2D ROTATECCW_XF(0, -1, 1, 0, 0, 0);
    static XForm2D FLIPX_XF(-1, 0, 0, 1, 0, 0);
    static XForm2D FLIPY_XF(1, 0, 0, -1, 0, 0);
    static XForm2D FLIP_XF(-1, 0, 0, -1, 0, 0);

    const XForm2D& XForm2D::ZERO() { return ZERO_XF; }
    const XForm2D& XForm2D::IDENTITY() { return IDENTITY_XF; }
    const XForm2D& XForm2D::ROTATECW() { return ROTATECW_XF; }
    const XForm2D& XForm2D::ROTATECCW() { return ROTATECCW_XF; }
    const XForm2D& XForm2D::FLIPX() { return FLIPX_XF; }
    const XForm2D& XForm2D::FLIPY() { return FLIPX_XF; }
    const XForm2D& XForm2D::ONEEIGHTY() { return FLIP_XF; }

    //--

    XForm2D Concat(const XForm2D& a, const XForm2D& b)
    {
        float ret0 = a.t[0] * b.t[0] + a.t[1] * b.t[2];
        float ret2 = a.t[2] * b.t[0] + a.t[3] * b.t[2];
        float ret4 = a.t[4] * b.t[0] + a.t[5] * b.t[2] + b.t[4];
        XForm2D ret;
        ret.t[1] = a.t[0] * b.t[1] + a.t[1] * b.t[3];
        ret.t[3] = a.t[2] * b.t[1] + a.t[3] * b.t[3];
        ret.t[5] = a.t[4] * b.t[1] + a.t[5] * b.t[3] + b.t[5];
        ret.t[0] = ret0;
        ret.t[2] = ret2;
        ret.t[4] = ret4;
        return ret;
    }

    //--

} // base