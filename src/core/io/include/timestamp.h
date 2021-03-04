/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

#include "core/containers/include/stringBuf.h"

BEGIN_BOOMER_NAMESPACE()

/// TimeStamp for file
class CORE_IO_API TimeStamp
{
public:
    INLINE TimeStamp()
        : m_timecode(0)
    {}

    explicit INLINE TimeStamp(uint64_t timeCode)
        : m_timecode(timeCode)
    {
    }

    INLINE bool operator==(const TimeStamp &stamp) const
    {
        return m_timecode == stamp.m_timecode;
    }

    INLINE bool operator!=(const TimeStamp &stamp) const
    {
        return m_timecode != stamp.m_timecode;
    }

    INLINE bool operator<(const TimeStamp &stamp) const
    {
        return m_timecode < stamp.m_timecode;
    }

    INLINE bool operator>(const TimeStamp &stamp) const
    {
        return m_timecode > stamp.m_timecode;
    }

    INLINE bool operator<=(const TimeStamp &stamp) const
    {
        return m_timecode <= stamp.m_timecode;
    }

    INLINE bool operator>=(const TimeStamp &stamp) const
    {
        return m_timecode >= stamp.m_timecode;
    }

    INLINE uint64_t hash() const
    {
        return m_timecode;
    }

    INLINE uint64_t value() const
    {
        return m_timecode;
    }

    INLINE bool empty() const
    {
        return m_timecode == 0;
    }

    //--

    //! Convert to display string (human readable)
    StringBuf toDisplayString() const;

    //! Convert to safe string (no spaces)
    StringBuf toSafeString() const;

    //--

    //! Get current date/time
    static TimeStamp GetNow();

    //! Get date/time from seconds/nano seconds
    static TimeStamp GetFromFileTime(uint64_t seconds, uint64_t nanoSeconds);

    //---

    // print as string
    void print(IFormatStream& f) const;

private:
    uint64_t  m_timecode;
};

END_BOOMER_NAMESPACE()
