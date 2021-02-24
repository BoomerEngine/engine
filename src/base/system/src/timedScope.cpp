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

BEGIN_BOOMER_NAMESPACE(base)

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

TimingStatistics::TimingStatistics()
{
}

void TimingStatistics::clear()
{
    bestValue = DBL_MAX;
    worstValue = -DBL_MAX;
    currentMean = 0.0;
    currentVariance = 0.0;
    currentSum = 0.0;
    count = 0;
}

void TimingStatistics::update(double value)
{
    if (count == 0)
    {
        bestValue = value;
        worstValue = value;
        count = 1;
    }
    else if (count == 1)
    {
        bestValue = std::min(value, bestValue);
        worstValue = std::max(value, worstValue);
        count = 2;
    }
    else
    {
        if (value > bestValue)
            std::swap(value, bestValue);
        else if (value < worstValue)
            std::swap(value, worstValue);

        if (count == 2)
        {
            currentMean = value;
            currentVariance = 0.0;
            currentSum = 0.0;
        }
        else
        {
            auto termCount = count - 2;
            auto prevMean = currentMean;
            currentMean += (value - currentMean) / (double)termCount;
            currentSum += (value - currentMean) * (value - prevMean);
            currentVariance = std::sqrt(currentSum / (double)termCount);
        }

        count += 1;
    }
}

double TimingStatistics::mean() const
{
    if (count == 0)
        return 0.0;
    else if (count == 1)
        return bestValue;
    else if (count == 2)
        return (bestValue + worstValue) * 0.5;
    else
        return currentMean;
}

double TimingStatistics::variance() const
{
    return currentVariance;
}

//--

END_BOOMER_NAMESPACE(base)
