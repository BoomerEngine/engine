/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\stack2D #]
***/

#pragma once

#include "xform2D.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// Old OpenGL style "stack" of transforms
class CORE_MATH_API TransformStack2D
{
public:
    INLINE TransformStack2D(); // identity
    INLINE TransformStack2D(const TransformStack2D& other) = default;
    INLINE TransformStack2D& operator=(const TransformStack2D& other) = default;

    // get current transform
    INLINE const XForm2D& transform() { return head.transform; }

    // current head transform classification
    INLINE XForm2DClass transformClass() const { return (XForm2DClass)head.classification; }

    //--

    // reset state and stack to default state
    void clear();

    //--

    // push current transform on stack
    void push();

    // pop previous state from stack, if already empty an identity is "pulled"
    void pop();

    //--

    // Multiply current coordinate system by specified matrix (apply on top)
    INLINE void transform(float a, float b, float c, float d, float e, float f);

    // Multiply current coordinate system by specified matrix (apply on top)
    INLINE void transform(const float* t); // 6 floats

    // Multiply current coordinate system by specified matrix (apply on top)
    void transform(const XForm2D& t); // 6 floats

    // Offset current coordinate system
    void offset(float x, float y);

    // Offset current coordinate system
    INLINE void offset(const Vector2& o);

    // Rotates current coordinate system. Angle is specified in radians.
    void rotate(float angle);

    // Skews the current coordinate system along X axis. Angle is specified in radians.
    void skewX(float angle);

    // Skews the current coordinate system along Y axis. Angle is specified in radians.
    void skewY(float angle);

    // Scales the current coordinate system.
    void scale(float x, float y);

    // Scales the current coordinate system, uniform scale
    void scale(float uniform);

    //--

    // resets current to-level transform to a identity matrix
    void identity();

    // set current level to specific transform
    INLINE void reset(float a, float b, float c, float d, float e, float f);

    // set current level to specific transform
    INLINE void reset(const float* t); // 6 floats

    // set current level to specific transform
    void reset(const XForm2D& t);

    //--


private:
    static const uint32_t DEFAULT_DEPTH = 32; // in 2D it's common to go deep

    struct Entry
    {
        XForm2D transform;
        uint8_t classification = 0;
        uint32_t padding = 0;
    };

    Entry head; // head entry

    InplaceArray<Entry, DEFAULT_DEPTH> stack; // previus "stored" positions
};

//--

END_BOOMER_NAMESPACE()
