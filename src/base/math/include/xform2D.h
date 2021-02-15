/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\xform2D #]
***/

#pragma once

namespace base
{
    /// 2D XForm "flag"
    enum XForm2DClass : uint8_t
    {
        Identity = 0, // full identity case
        HasTransform = 1, // we only translation set to non-zero
        HasScaleRotation = 2, // we have inner 2x2 matrix part set to non-identity
        Full = 3, // all members should be considered
    };

    /// 2D XForm (affine transformation)
    /// supports scaling, rotation and offset, compact form NOT optimized for SSE
    class BASE_MATH_API XForm2D
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(XForm2D);

    public:
        float t[6];

        INLINE XForm2D(); // resets to identity
        INLINE XForm2D(const float* data); // requires 6 floats
        INLINE XForm2D(float t00, float t01, float t10, float t11, float tx, float ty);
        INLINE XForm2D(float tx, float ty); // position only
		INLINE XForm2D(float tx, float ty, float s); // position + uniform scale
		INLINE XForm2D(float tx, float ty, float sx, float sy); // position + scale
        INLINE XForm2D(const XForm2D& other) = default;
        INLINE XForm2D(XForm2D&& other) = default;
        INLINE XForm2D& operator=(const XForm2D& other) = default;
        INLINE XForm2D& operator=(XForm2D&& other) = default;

        INLINE bool operator==(const XForm2D& other) const;
        INLINE bool operator!=(const XForm2D& other) const;

        INLINE XForm2D operator~() const;

        INLINE XForm2D operator*(const XForm2D& other) const;
        INLINE XForm2D& operator*=(const XForm2D& other);

        INLINE XForm2D operator+(const Vector2& delta) const;
        INLINE XForm2D& operator+=(const Vector2& delta);
        INLINE XForm2D operator-(const Vector2& delta) const;
        INLINE XForm2D& operator-=(const Vector2& delta);

        //--

        // reset to identity
        INLINE XForm2D& identity();

        // get the translation part of the XForm
        INLINE const Vector2& translation() const;

        // change the translational part
        INLINE XForm2D& translation(const Vector2& t);

        // change the translational part
        INLINE XForm2D& translation(float x, float y);

        // get the determinant of the XForm, specified how areas are scaled
        // NOTE: if det<0 than there's a winding flip
        INLINE double det() const;

        // get inversion of this XForm
        XForm2D inverted() const;

        //---

        // transform point by the Xform
        INLINE Vector2 transformPoint(const Vector2& pos) const;

        // transform vector by the Xform
        INLINE Vector2 transformVector(const Vector2& pos) const;

        // directly transform X coordinate
        INLINE float transformX(float x, float y) const;

        // directly transform Y coordinate
        INLINE float transformY(float x, float y) const;

        //---

        // build a rotation XForm
        static XForm2D BuildRotation(float cwAngleInRadians);

		// build a rotation XForm around a given position (pivoted)
		static XForm2D BuildRotationAround(float cwAngleInRadians, float tx, float ty);

        // buiild a scaling Xform
        static XForm2D BuildScale(float uniformScale);

        // buiild a scaling Xform
        static XForm2D BuildScale(float scaleX, float scaleY);

        // build a translation Xform (waste of cycles)
        static XForm2D BuildTranslation(const Vector2& ofs);

        // build a translation Xform (waste of cycles)
        static XForm2D BuildTranslation(float x, float y, float s = 1.0f);

        // build a skew Xform
        static XForm2D BuildSkewX(float a);

        // build a skew Xform
        static XForm2D BuildSkewY(float a);

        //---

        // convert to a full matrix
        // NOTE: the translation is put in the proper place in the last column
        Matrix toMatrix() const;

        // convert to a full matrix
        // NOTE: the translation is put in the last column
        Matrix33 toMatrix33() const;

        // convert rotation part to a quaternion
        Quat toQuat() const;

        // convert to transform
        Transform toTransform() const;

        //---

        // "classify" transform based on the amount of potential work
        XForm2DClass classify() const;

        //---

        static const XForm2D& ZERO();
        static const XForm2D& IDENTITY();
        static const XForm2D& ROTATECW();
        static const XForm2D& ROTATECCW();
        static const XForm2D& FLIPX();
        static const XForm2D& FLIPY();
        static const XForm2D& ONEEIGHTY();
    };

    //--

} // base