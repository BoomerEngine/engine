/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#include "build.h"
#include "timing.h"
#include "timedScope.h"

namespace base
{

    //---

    TimedScope::TimedScope(const char* scopeName)
    {
        m_name = scopeName;
        (NativeTimePoint&)m_startTime = NativeTimePoint::Now();
        m_prevMarkerTime = m_startTime;
    }

    TimedScope::~TimedScope()
    {
        printSubtime("END");
    }

    void TimedScope::printSubtime(const char* text)
    {
        auto time = NativeTimePoint::Now();
        auto deltaSicneLast = (time - (NativeTimePoint&)m_prevMarkerTime).toMiliSeconds();
        auto deltaSinceStart = (time - (NativeTimePoint&)m_startTime).toMiliSeconds();
        (NativeTimePoint&)m_prevMarkerTime = time;

        TRACE_INFO("[{}]: %10.3fms  %10.3fms  '{}'", m_name, deltaSinceStart, deltaSicneLast, text);
    }

    //---

    ScopeTimer::ScopeTimer()
    {
        (NativeTimePoint&)m_startTime = NativeTimePoint::Now();
    }

    double ScopeTimer::timeElapsed() const
    {
        auto time = NativeTimePoint::Now();
        return (time - (NativeTimePoint&)m_startTime).toSeconds();
    }

    float ScopeTimer::milisecondsElapsed() const
    {
        auto time = NativeTimePoint::Now();
        return (time - (NativeTimePoint&)m_startTime).toMiliSeconds();
    }

    void ScopeTimer::print(IFormatStream& f) const
    {
        f << TimeInterval(timeElapsed());
    }

    //--

}
