/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\range #]
***/

#pragma once

namespace base
{
    //--

    // value range
    class BASE_MATH_API Range
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

        //! get random value from range
        INLINE float rand() const;

        //! interpolate between min and max
        INLINE float lerp(float frac) const;

        //! given a value get the interpolant for the range
        INLINE float frac(float value) const;

        //! given a value get the interpolant for the range clamped to 0-1
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

    extern BASE_MATH_API Range Snap(const Range &a, float grid);
    extern BASE_MATH_API Range Lerp(const Range &a, const Range &b, float frac);
    extern BASE_MATH_API Range Min(const Range &a, const Range &b);
    extern BASE_MATH_API Range Max(const Range &a, const Range &b);
    extern BASE_MATH_API Range Clamp(const Range &a, const Range &minV, const Range &maxV);
    extern BASE_MATH_API Range Clamp(const Range &a, float minF, float maxF);

} // base