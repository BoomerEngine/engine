/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#include "build.h"
#include "timing.h"

BEGIN_BOOMER_NAMESPACE()

//-----

NativeTimePoint NativeTimePoint::Now()
{
    NativeTimePoint ret;
#if defined(PLATFORM_WINDOWS)
    QueryPerformanceCounter((LARGE_INTEGER*)&ret.m_value);
#elif defined(PLATFORM_LINUX)
    timespec sp;
    clock_gettime(CLOCK_MONOTONIC_RAW, &sp);
    ret.value = sp.tv_nsec  + sp.tv_sec * 1000000000LLU;
#endif
    return ret;
}

NativeTimeInterval NativeTimePoint::timeTillNow() const
{
    return Now() - *this;
}

void NativeTimePoint::print(IFormatStream& f) const
{
    f << TimeInterval(timeTillNow().toSeconds());
}

void NativeTimePoint::resetToNow()
{
    *this = Now();
}

bool NativeTimePoint::reached() const
{
    return Now() > *this;
}

//-----

#if defined(PLATFORM_WINDOWS)
INLINE static NativeTimeInterval::TDelta GetFreq()
{
    NativeTimeInterval::TDelta ret = 1;
    QueryPerformanceFrequency((LARGE_INTEGER*)&ret);
    return ret;
}
#elif defined(PLATFORM_LINUX)
INLINE static NativeTimeInterval::TDelta GetFreq()
{
    return NativeTimeInterval::TDelta(1000000000LLU);
}
#endif

NativeTimeInterval::NativeTimeInterval(double delta)
{
    m_delta = (TDelta)(delta  * (double)GetFreq());
}

double NativeTimeInterval::toSeconds() const
{
    return (double)m_delta / (double)GetFreq();
}

float NativeTimeInterval::toMiliSeconds() const
{
    return (float)(toSeconds() * 1000.0);
}

void NativeTimeInterval::print(IFormatStream& f) const
{
    f << TimeInterval(toSeconds());
}

END_BOOMER_NAMESPACE()
