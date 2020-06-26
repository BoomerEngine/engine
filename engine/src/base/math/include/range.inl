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

    INLINE Range::Range()
        : min(0)
        , max(0)
    {}

    INLINE Range::Range(float a, float b)
        : min(a)
        , max(b)
    {}

    INLINE bool Range::empty() const
    {
        return max <= min;
    }

    INLINE float Range::length() const
    {
        return (max >= min) ? (max - min) : 0.0f;
    }

    INLINE float Range::rand() const
    {
        return RandRange(min, max);
    }

    INLINE float Range::lerp(float frac) const
    {
        return min + frac * (max - min);
    }

    INLINE float Range::frac(float value) const
    {
        return (value - min) / (max - min);
    }

    INLINE float Range::fracClamped(float value) const
    {
        if (value <= min)
            return 0.0f;
        else if (value >= max)
            return 1.0f;
        else
            return (value - min) / (max - min);
    }

    INLINE float Range::clamp(float value) const
    {
        return std::clamp(value, min, max);
    }

    INLINE void Range::extend(float value)
    {
        min = std::min(min, value);
        max = std::max(max, value);
    }

    INLINE void Range::shift(float value)
    {
        min += value;
        max += value;
    }

    INLINE bool Range::operator==(const Range& other) const
    {
        return (min == other.min) && (max == other.max);
    }

    INLINE bool Range::operator!=(const Range& other) const
    {
        return (min != other.min) || (max != other.max);
    }

    INLINE bool Range::contains(float value) const
    {
        return (value >= min) && (value <= max);
    }

    //--
    
} // base