/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\box #]
***/

#pragma once

namespace base
{

    //---

    INLINE Box::Box()
    {
        min = Vector3::INF();
        max = -Vector3::INF();
    }

    INLINE Box::Box(const Vector3 &amin, const Vector3 &amax)
        : min(amin)
        , max(amax)
    {}

    INLINE Box::Box(const Vector3 &center, float radius)
    {
        min = center - radius;
        max = center + radius;
    }

    INLINE Box::Box(const Vector3 &start, const Vector3 &end, const Vector3 &extents)
    {
        min = Min(start - extents, end - extents);
        max = Max(start + extents, end + extents);
    }

    INLINE Box::Box(const Vector3 &start, const Vector3 &end, const Box &extents)
    {
        min = Min(start + extents.min, end + extents.min);
        max = Max(start + extents.max, end + extents.max);
    }

    INLINE void Box::clear()
    {
        min = Vector3::INF();
        max = -Vector3::INF();
    }

    INLINE bool Box::empty() const
    {
        return (min.x > max.x) || (min.y > max.y) || (min.z > max.z);
    }

    INLINE bool Box::contains(const Vector3 &point) const
    {
        if (point.x < min.x || point.x > max.x) return false;
        if (point.y < min.y || point.y > max.y) return false;
        if (point.z < min.z || point.z > max.z) return false;
        return true;
    }

    INLINE bool Box::contains2D(const Vector2 &point) const
    {
        if (point.x < min.x || point.x > max.x) return false;
        if (point.y < min.y || point.y > max.y) return false;
        return true;
    }

    INLINE bool Box::contains(const Box &box) const
    {
        if (box.min.x < min.x || box.max.x > max.x) return false;
        if (box.min.y < min.y || box.max.y > max.y) return false;
        if (box.min.z < min.z || box.max.z > max.z) return false;
        return true;
    }

    INLINE bool Box::contains2D(const Box &box) const
    {
        if (box.min.x < min.x || box.max.x > max.x) return false;
        if (box.min.y < min.y || box.max.y > max.y) return false;
        return true;
    }

    INLINE bool Box::touches(const Box &other) const
    {
        if (min.x > other.max.x || max.x < other.min.x) return false;
        if (min.y > other.max.y || max.y < other.min.y) return false;
        if (min.z > other.max.z || max.z < other.min.z) return false;
        return true;
    }

    INLINE bool Box::touches2D(const Box &other) const
    {
        if (min.x > other.max.x || max.x < other.min.x) return false;
        if (min.y > other.max.y || max.y < other.min.y) return false;
        return true;
    }

    INLINE Vector3 Box::corner(int index) const
    {
        float x = (index & 1) ? min.x : max.x;
        float y = (index & 2) ? min.y : max.y;
        float z = (index & 4) ? min.z : max.z;
        return Vector3(x, y, z);
    }

    INLINE void Box::corners(Vector3* outCorners) const
    {
        outCorners[0] = Vector3(min.x, min.y, min.z);
        outCorners[1] = Vector3(max.x, min.y, min.z);
        outCorners[2] = Vector3(min.x, max.y, min.z);
        outCorners[3] = Vector3(max.x, max.y, min.z);
        outCorners[4] = Vector3(min.x, min.y, max.z);
        outCorners[5] = Vector3(max.x, min.y, max.z);
        outCorners[6] = Vector3(min.x, max.y, max.z);
        outCorners[7] = Vector3(max.x, max.y, max.z);
    }

    INLINE void Box::extrude(float margin)
    {
        min -= margin;
        max += margin;
    }

    INLINE Box Box::extruded(float margin) const
    {
        return Box(min - margin, max + margin);
    }

    INLINE Box& Box::merge(const Vector3 &point)
    {
        min = Min(min, point);
        max = Max(max, point);
        return *this;
    }

    INLINE Box& Box::merge(const Box &box)
    {
        min = Min(min, box.min);
        max = Max(max, box.max);
        return *this;
    }

    INLINE float Box::volume() const
    {
        return empty() ? 0.0f : (max.x - min.x) * (max.y - min.y) * (max.z - min.z);
    }

    INLINE Vector3 Box::size() const
    {
        return max - min;
    }

    INLINE Vector3 Box::extents() const
    {
        return (max - min) / 2.0f;
    }

    INLINE Vector3 Box::center() const
    {
        return (min + max) / 2.0f;
    }

    INLINE bool Box::operator==(const Box& other) const
    {
        return (min == other.min) && (max == other.max);
    }

    INLINE bool Box::operator!=(const Box& other) const
    {
        return !operator==(other);
    }

    INLINE Box Box::operator+(const Vector3& other) const
    {
        return Box(min + other, max + other);
    }

    INLINE Box Box::operator-(const Vector3& other) const
    {
        return Box(min - other, max - other);
    }

    INLINE Box Box::operator+(float margin) const
    {
        return Box(min + margin, max + margin);
    }

    INLINE Box Box::operator-(float margin) const
    {
        return Box(min - margin, max - margin);
    }

    INLINE Box Box::operator*(float scale) const
    {
        return Box(min * scale, max * scale);
    }

    INLINE Box Box::operator/(float scale) const
    {
        return Box(min / scale, max / scale);
    }

    INLINE Box& Box::operator+=(const Vector3& other)
    {
        min += other;
        max += other;
        return *this;
    }

    INLINE Box& Box::operator-=(const Vector3& other)
    {
        min -= other;
        max -= other;
        return *this;
    }

    INLINE Box& Box::operator+=(float margin)
    {
        min += margin;
        max += margin;
        return *this;
    }

    INLINE Box& Box::operator-=(float margin)
    {
        min -= margin;
        max -= margin;
        return *this;
    }

    INLINE Box& Box::operator*=(float scale)
    {
        min *= scale;
        max *= scale;
        return *this;
    }

    INLINE Box& Box::operator/=(float scale)
    {
        min /= scale;
        max /= scale;
        return *this;
    }

} // base