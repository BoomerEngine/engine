/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\absolute #]
***/

#pragma once

namespace base
{

    //--

    INLINE static void SplitAbsolutePair(double d, float& x, float& e)
    {
        x = (float)d;
        e = (float)(d - (double)x);
    }

    INLINE static double ExtractAbsolutePair(float x, float e)
    {
        return (double)x + (double)e;
    }

    INLINE static void NormalizeAbsolutePair(float& x, float& e)
    {
        SplitAbsolutePair(ExtractAbsolutePair(x, e), x, e);
    }

    //--

    INLINE AbsolutePosition::AbsolutePosition()
        : m_primary(0.0f, 0.0f, 0.0f)
        , m_error(0.0f, 0.0f, 0.0f)
    {}

    INLINE AbsolutePosition::AbsolutePosition(double x, double y, double z)
    {
        SplitAbsolutePair(x, m_primary.x, m_error.x);
        SplitAbsolutePair(y, m_primary.y, m_error.y);
        SplitAbsolutePair(z, m_primary.z, m_error.z);
    }

    INLINE AbsolutePosition::AbsolutePosition(const Vector3& primary, const Vector3& error)
        : m_primary(primary)
        , m_error(error)
    {}

    INLINE void AbsolutePosition::expand(double& x, double& y, double& z) const
    {
        x = ExtractAbsolutePair(m_primary.x, m_error.x);
        y = ExtractAbsolutePair(m_primary.y, m_error.y);
        z = ExtractAbsolutePair(m_primary.z, m_error.z);
    }

    INLINE AbsolutePosition AbsolutePosition::operator+(const Vector3& offset) const
    {
        return AbsolutePosition(*this) += offset;
    }

    INLINE AbsolutePosition& AbsolutePosition::operator+=(const Vector3& offset)
    {
        m_error += offset;
        normalize();
        return *this;
    }

    INLINE AbsolutePosition AbsolutePosition::operator-(const Vector3& offset) const
    {
        return AbsolutePosition(*this) -= offset;
    }

    INLINE AbsolutePosition& AbsolutePosition::operator-=(const Vector3& offset)
    {
        m_error -= offset;
        normalize();
        return *this;
    }

    INLINE const Vector3& AbsolutePosition::approximate() const
    {
        return m_primary;
    }

    INLINE const Vector3& AbsolutePosition::error() const
    {
        return m_error;
    }

    INLINE bool AbsolutePosition::operator==(const AbsolutePosition& other) const
    {
        return (m_primary == other.m_primary) && (m_error == other.m_error);
    }

    INLINE bool AbsolutePosition::operator!=(const AbsolutePosition& other) const
    {
        return !operator==(other);
    }

    INLINE Vector3 AbsolutePosition::operator-(const AbsolutePosition& base) const
    {
        auto primaryDist  = m_primary - base.m_primary;
        auto errorDist  = m_error - base.m_error;
        return primaryDist + errorDist;
    }

    INLINE float AbsolutePosition::distance(const AbsolutePosition& base) const
    {
        return m_primary.distance(base.m_primary);
    }

    INLINE double AbsolutePosition::exactDistance(const AbsolutePosition& base) const
    {
        double ax, ay, az, bx, by, bz;
        expand(ax, ay, az);
        base.expand(bx, by, bz);
        ax = (bx-ax)*(bx-ax);
        ay = (by-ay)*(by-ay);
        az = (bz-az)*(bz-az);
        return std::sqrt(ax + ay + az);
    }

    INLINE void AbsolutePosition::normalize()
    {
        NormalizeAbsolutePair(m_primary.x, m_error.x);
        NormalizeAbsolutePair(m_primary.y, m_error.y);
        NormalizeAbsolutePair(m_primary.z, m_error.z);
    }

    //--

} // base
