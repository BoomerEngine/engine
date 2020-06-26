/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

namespace base
{
    class NativeTimeInterval;

    // a high-precision time in-engine time representation, should not be saved
    // native time intervals can be converted to a floating time interval
    class BASE_SYSTEM_API NativeTimePoint
    {
    public:
        typedef uint64_t TValue;

        INLINE NativeTimePoint();
        explicit INLINE NativeTimePoint(TValue time);
        INLINE NativeTimePoint(const NativeTimePoint& other);
        INLINE NativeTimePoint& operator=(const NativeTimePoint& val);

        // compare
        INLINE bool operator==(const NativeTimePoint& val) const;
        INLINE bool operator!=(const NativeTimePoint& val) const;
        INLINE bool operator<=(const NativeTimePoint& val) const;
        INLINE bool operator>=(const NativeTimePoint& val) const;
        INLINE bool operator<(const NativeTimePoint& val) const;
        INLINE bool operator>(const NativeTimePoint& val) const;

        // is this a valid time ?
        INLINE bool valid() const { return m_value != 0; }

        // clear value
        INLINE void clear() { m_value = 0; }

        // test for validity
        INLINE operator bool() const { return m_value != 0; }

        // get the raw value
        INLINE TValue rawValue() const { return m_value; }

        // compute difference between two native times, returned value is in seconds
        INLINE NativeTimeInterval operator-(const NativeTimePoint& end) const;

        // move the time point by interval
        INLINE NativeTimePoint operator-(const NativeTimeInterval& diff) const;
        INLINE NativeTimePoint operator+(const NativeTimeInterval& diff) const;
        INLINE NativeTimePoint operator-(const double diff) const;
        INLINE NativeTimePoint operator+(const double diff) const;

        // move the time point by interval (self)
        INLINE NativeTimePoint& operator-=(const NativeTimeInterval& diff);
        INLINE NativeTimePoint& operator+=(const NativeTimeInterval& diff);
        INLINE NativeTimePoint& operator-=(const double diff);
        INLINE NativeTimePoint& operator+=(const double diff);

        // get time elapsed from given time point till now
        NativeTimeInterval timeTillNow() const;

        // reset time to now
        void resetToNow();

        // check if this time has been reached
        bool reached() const;

        // get the value of the native time now
        static NativeTimePoint Now();

        //--

        // print, prints as a time interval from now
        void print(IFormatStream& f) const;

    public:
        TValue m_value;
    };

    /// a high precision interval between two points in time
    class BASE_SYSTEM_API NativeTimeInterval
    {
    public:
        typedef int64_t TDelta;

        INLINE NativeTimeInterval();
        INLINE NativeTimeInterval(const NativeTimeInterval& other);
        INLINE NativeTimeInterval& operator=(const NativeTimeInterval& other);
        explicit INLINE NativeTimeInterval(TDelta val);
        explicit NativeTimeInterval(double val);

        // compare directly
        INLINE bool operator==(const NativeTimeInterval& val) const;
        INLINE bool operator!=(const NativeTimeInterval& val) const;
        INLINE bool operator<=(const NativeTimeInterval& val) const;
        INLINE bool operator>=(const NativeTimeInterval& val) const;
        INLINE bool operator<(const NativeTimeInterval& val) const;
        INLINE bool operator>(const NativeTimeInterval& val) const;

        // is the interval "zero" ?
        INLINE bool isZero() const;

        // get raw value
        INLINE TDelta rawValue() const { return m_delta; }

        // convert to double (for seconds)
        double toSeconds() const;

        // convert to float for ms values in debug mode
        float toMiliSeconds() const;

        //--

        // print, prints as a time interval
        void print(IFormatStream& f) const;

    public:
        TDelta  m_delta;
    };

} // base

#include "timing.inl"
