/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//----------

INLINE NativeTimePoint::NativeTimePoint()
    : m_value(0)
{}

INLINE NativeTimePoint::NativeTimePoint(TValue time)
    : m_value(time)
{}

INLINE NativeTimePoint::NativeTimePoint(const NativeTimePoint& other)
    : m_value(other.m_value)
{}

INLINE NativeTimePoint& NativeTimePoint::operator=(const NativeTimePoint& val)
{
    m_value = val.m_value;
    return *this;
}

INLINE bool NativeTimePoint::operator==(const NativeTimePoint& val) const
{
    return m_value == val.m_value;
}

INLINE bool NativeTimePoint::operator!=(const NativeTimePoint& val) const
{
    return m_value != val.m_value;
}

INLINE bool NativeTimePoint::operator<=(const NativeTimePoint& val) const
{
    return m_value <= val.m_value;
}

INLINE bool NativeTimePoint::operator>=(const NativeTimePoint& val) const
{
    return m_value >= val.m_value;
}

INLINE bool NativeTimePoint::operator<(const NativeTimePoint& val) const
{
    return m_value < val.m_value;
}

INLINE bool NativeTimePoint::operator>(const NativeTimePoint& val) const
{
    return m_value > val.m_value;
}

INLINE NativeTimeInterval NativeTimePoint::operator-(const NativeTimePoint& end) const
{
    return NativeTimeInterval((NativeTimeInterval::TDelta)m_value - (NativeTimeInterval::TDelta)end.m_value);
}

INLINE NativeTimePoint NativeTimePoint::operator-(const NativeTimeInterval& diff) const
{
    return NativeTimePoint(m_value - diff.m_delta);
}

INLINE NativeTimePoint NativeTimePoint::operator+(const NativeTimeInterval& diff) const
{
    return NativeTimePoint(m_value + diff.m_delta);
}

INLINE NativeTimePoint& NativeTimePoint::operator-=(const NativeTimeInterval& diff)
{
    m_value -= diff.m_delta;
    return *this;
}

INLINE NativeTimePoint& NativeTimePoint::operator+=(const NativeTimeInterval& diff)
{
    m_value += diff.m_delta;
    return *this;
}

INLINE NativeTimePoint NativeTimePoint::operator-(const double diff) const
{
    return NativeTimePoint(m_value - NativeTimeInterval(diff).m_delta);
}

INLINE NativeTimePoint NativeTimePoint::operator+(const double diff) const
{
    return NativeTimePoint(m_value + NativeTimeInterval(diff).m_delta);
}

INLINE NativeTimePoint& NativeTimePoint::operator-=(const double diff)
{
    m_value -= NativeTimeInterval(diff).m_delta;
    return *this;
}

INLINE NativeTimePoint& NativeTimePoint::operator+=(const double diff)
{
    m_value += NativeTimeInterval(diff).m_delta;
    return *this;
}

//----------

INLINE NativeTimeInterval::NativeTimeInterval()
    : m_delta(0)
{}

INLINE NativeTimeInterval::NativeTimeInterval(const NativeTimeInterval& other)
    : m_delta(other.m_delta)
{}

INLINE NativeTimeInterval& NativeTimeInterval::operator=(const NativeTimeInterval& other)
{
    m_delta = other.m_delta;
    return *this;
}

INLINE NativeTimeInterval::NativeTimeInterval(TDelta delta)
    : m_delta(delta)
{}

INLINE bool NativeTimeInterval::operator==(const NativeTimeInterval& val) const
{
    return m_delta == val.m_delta;
}

INLINE bool NativeTimeInterval::operator!=(const NativeTimeInterval& val) const
{
    return m_delta != val.m_delta;
}

INLINE bool NativeTimeInterval::operator<=(const NativeTimeInterval& val) const
{
    return m_delta <= val.m_delta;
}

INLINE bool NativeTimeInterval::operator>=(const NativeTimeInterval& val) const
{
    return m_delta >= val.m_delta;
}

INLINE bool NativeTimeInterval::operator<(const NativeTimeInterval& val) const
{
    return m_delta < val.m_delta;
}

INLINE bool NativeTimeInterval::operator>(const NativeTimeInterval& val) const
{
    return m_delta > val.m_delta;
}

INLINE NativeTimeInterval NativeTimeInterval::operator+(const NativeTimeInterval& val) const
{
	return NativeTimeInterval(m_delta + val.m_delta);
}

INLINE NativeTimeInterval& NativeTimeInterval::operator+=(const NativeTimeInterval& val)
{
	m_delta += val.m_delta;
	return *this;
}

INLINE NativeTimeInterval NativeTimeInterval::operator-(const NativeTimeInterval& val) const
{
	return NativeTimeInterval(m_delta - val.m_delta);
}

INLINE NativeTimeInterval& NativeTimeInterval::operator-=(const NativeTimeInterval& val)
{
	m_delta -= val.m_delta;
	return *this;
}

INLINE bool NativeTimeInterval::isZero() const
{
    return m_delta == 0;
}

END_BOOMER_NAMESPACE()
