/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\transform #]
***/

#pragma once

namespace base
{
    //--

    INLINE EulerTransform::EulerTransform()
        : T(Translation::ZERO())
        , R(Rotation::ZERO())
        , S(Scale::ONE())
    {}

    INLINE EulerTransform::EulerTransform(const Translation& pos)
        : T(pos)
        , R(Rotation::ZERO())
        , S(Scale::ONE())
    {}

    INLINE EulerTransform::EulerTransform(const Translation& pos, const Rotation& rot)
        : T(pos)
        , R(rot)
        , S(Scale::ONE())
    {}

    INLINE EulerTransform::EulerTransform(const Translation& pos, const Rotation& rot, const Scale& scale)
        : T(pos)
        , R(rot)
        , S(scale)
    {}

    INLINE EulerTransform& EulerTransform::identity()
    {
        T = Translation::ZERO();
        R = Rotation::ZERO();
        S = Scale::ONE();
        return *this;
    }

    INLINE bool EulerTransform::operator==(const EulerTransform& other) const
    {
        return (T == other.T) && (R == other.R) && (S == other.S);
    }

    INLINE bool EulerTransform::operator!=(const EulerTransform& other) const
    {
        return !operator==(other);
    }

    //--

} // base