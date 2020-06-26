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
    //--

    /// axis aligned bounding box
    class BASE_MATH_API Box
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(Box);

    public:
        Vector3 min;
        Vector3 max;

        //--

        INLINE Box();
        INLINE Box(const Vector3 &amin, const Vector3 &amax);
        INLINE Box(const Vector3 &center, float radius);
        INLINE Box(const Vector3 &start, const Vector3 &end, const Vector3 &extents);
        INLINE Box(const Vector3 &start, const Vector3 &end, const Box &extents);
        INLINE Box(const Box &other) = default;
        INLINE Box(Box&& other) = default;
        INLINE Box& operator=(const Box &other) = default;
        INLINE Box& operator=(Box&& other) = default;

        INLINE bool operator==(const Box& other) const;
        INLINE bool operator!=(const Box& other) const;

        INLINE Box operator+(const Vector3& other) const;
        INLINE Box operator-(const Vector3& other) const;
        INLINE Box operator+(float margin) const;
        INLINE Box operator-(float margin) const;
        INLINE Box operator*(float scale) const;
        INLINE Box operator/(float scale) const;

        INLINE Box& operator+=(const Vector3& other);
        INLINE Box& operator-=(const Vector3& other);
        INLINE Box& operator+=(float margin);
        INLINE Box& operator-=(float margin);
        INLINE Box& operator*=(float scale);
        INLINE Box& operator/=(float scale);

        //--

        //! clear bounding box, makes it empty
        INLINE void clear();

        //! is bounding box empty ?
        INLINE bool empty() const;

        //! test if given point is inside this box
        INLINE bool contains(const Vector3 &point) const;

        //! test if given point is inside this box
        INLINE bool contains2D(const Vector2 &point) const;

        //! test if given box is inside this box
        INLINE bool contains(const Box &box) const;

        //! test if given box is inside this box
        INLINE bool contains2D(const Box &box) const;

        //! test if box touches given box
        INLINE bool touches(const Box &other) const;

        //! test if box touches given box
        INLINE bool touches2D(const Box &other) const;

        //! get bounding box corner (valid vertex range 0-7)
        INLINE Vector3 corner(int index) const;

        //! get all of the box corners (8 of them)
        INLINE void corners(Vector3* outCorners) const;

        //! extrude bounding box by given margin
        INLINE void extrude(float margin);

        //! extrude bounding box by given margin
        INLINE Box extruded(float margin) const;

        //! add point to box
        INLINE Box& merge(const Vector3 &point);

        //! add box to box
        INLINE Box& merge(const Box &box);

        //! get box volume
        INLINE float volume() const;

        //! get box size (max - min)
        INLINE Vector3 size() const;

        //! calculate box extents (half size)
        INLINE Vector3 extents() const;

        //! calculate center of the box
        INLINE Vector3 center() const;

        //! get random point inside the box
        Vector3 rand() const;

        //--

        //! Zero box (0,0,0) - (0,0,0)
        static const Box& ZERO();

        //! Invalid box (MAX_INF) - (MIN_INF)
        static const Box& EMPTY();

        //! Unit box (0,0,0)-(1,1,1)
        static const Box& UNIT();
    };

} // base