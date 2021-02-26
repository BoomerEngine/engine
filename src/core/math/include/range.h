/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\range #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//--

// value range, special class is moslty used so we can have a nice UI
class CORE_MATH_API Range
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(Range);

public:
    float min;  //!< minimum value
    float max;  //!< maximum value

    //--

    INLINE Range();
    INLINE Range(float a, float b);
    INLINE Range(const Range &other) = default;
    INLINE Range(Range&& other) = default;
    INLINE Range& operator=(const Range &other) = default;
    INLINE Range& operator=(Range&& other) = default;

    INLINE bool operator==(const Range& other) const;
    INLINE bool operator!=(const Range& other) const;

    //--

    //! returns true if the range is empty (max <= min)
    INLINE bool empty() const;

    //! get range length (max - min)
    INLINE float length() const;

    //! given a fraction of the range (0-1) map that back to the value
    INLINE float lerp(float frac) const;

    //! given a value map that value to the range, returns 0-1 if value was in range
    INLINE float frac(float value) const;

    //! given a value map that value to the range, always clamps the result to 0-1 range
    INLINE float fracClamped(float value) const;

    //! clamp value to this range
    INLINE float clamp(float value) const;

    //! expend range to contain given value
    INLINE void extend(float value);

    //! shift range by value
    INLINE void shift(float value);

    //! returns true if given value is inside the range
    INLINE bool contains(float value) const;

    //--

    static const Range& ZERO();
    static const Range& UNIT();
    static const Range& PLUS_MINUS_ONE();
    static const Range& PLUS_MINUS_HALF();
    static const Range& EMPTY();
};

END_BOOMER_NAMESPACE()
