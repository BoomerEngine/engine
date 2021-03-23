/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\rotation #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//-----------------------------------------------------------------------------

INLINE Angles::Angles()
    : pitch(0.0f)
    , yaw(0.0f)
    , roll(0.0f)
{}

INLINE Angles::Angles(float inPitch, float inYaw, float inRoll)
{
    pitch = inPitch;
    yaw = inYaw;
    roll = inRoll;
}

INLINE bool Angles::isZero() const
{
    return (pitch == 0.0f) && (yaw == 0.0f) && (roll == 0.0f);
}

INLINE bool Angles::isNearZero(float eps) const
{
    return (std::abs(pitch) <= eps) && (std::abs(yaw) <= eps) && (std::abs(roll) <= eps);
}

INLINE Angles Angles::abs() const
{
    return Angles(std::abs(pitch), std::abs(yaw), std::abs(roll));
}

INLINE float Angles::maxValue() const
{
    return std::max(pitch, std::max(yaw, roll));
}

INLINE float Angles::minValue() const
{
    return std::min(pitch, std::min(yaw, roll));
}

INLINE float Angles::sum() const
{
    return pitch + yaw + roll;
}

INLINE uint8_t Angles::smallestAxis() const
{
    float ax = std::abs(roll);
    float ay = std::abs(pitch);
    float az = std::abs(yaw);

    if (ax < ay && ax < az) return 0;
    if (ay < ax && ay < az) return 1;
    return 2;
}

INLINE uint8_t Angles::largestAxis() const
{
    float ax = std::abs(roll);
    float ay = std::abs(pitch);
    float az = std::abs(yaw);

    if (ax > ay && ax > az) return 0;
    if (ay > ax && ay > az) return 1;
    return 2;
}

INLINE bool Angles::operator==(const Angles &other) const
{
    return pitch == other.pitch && yaw == other.yaw && roll == other.roll;
}

INLINE bool Angles::operator!=(const Angles &other) const
{
    return pitch != other.pitch || yaw != other.yaw || roll != other.roll;
}

INLINE Angles Angles::operator+(const Angles &other) const
{
    return Angles(pitch + other.pitch, yaw + other.yaw, roll + other.roll);
}

INLINE Angles& Angles::operator+=(const Angles &other)
{
    pitch = pitch + other.pitch;
    yaw = yaw + other.yaw;
    roll = roll + other.roll;
    return *this;
}

INLINE Angles Angles::operator-() const
{
    return Angles(-pitch, -yaw, -roll);
}

INLINE Angles Angles::operator-(const Angles &other) const
{
    return Angles(pitch - other.pitch, yaw - other.yaw, roll - other.roll);
}

INLINE Angles& Angles::operator-=(const Angles &other)
{
    pitch = pitch - other.pitch;
    yaw = yaw - other.yaw;
    roll = roll - other.roll;
    return *this;
}

INLINE Angles& Angles::operator*=(float value)
{
    pitch = pitch * value;
    yaw = yaw * value;
    roll = roll * value;
    return *this;
}

INLINE Angles Angles::operator*(float value) const
{
    return Angles(pitch*value, yaw*value, roll*value);
}

INLINE Angles& Angles::operator/=(float value)
{
    pitch = pitch / value;
    yaw = yaw / value;
    roll = roll / value;
    return *this;
}

INLINE Angles Angles::operator/(float value) const
{
    return Angles(pitch / value, yaw / value, roll / value);
}

//--

INLINE void Angles::snap(float grid)
{
    pitch = Snap(pitch, grid);
    yaw = Snap(yaw, grid);
    roll = Snap(roll, grid);
}

INLINE Angles Angles::snapped(float grid) const
{
    return Angles(
        Snap(pitch, grid),
        Snap(yaw, grid),
        Snap(roll, grid));
}

INLINE float Angles::dot(const Angles& other) const
{
    return forward() | other.forward();
}

INLINE float Angles::dot(const Vector3& other) const
{
    return forward() | other;
}

//--

END_BOOMER_NAMESPACE()
