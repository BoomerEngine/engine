/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

/// a scope that prints its timing to log
/// used for small profiling, can be picked up the automated tools
class CORE_SYSTEM_API TimedScope
{
public:
    TimedScope(const char* scopeName);
    ~TimedScope();

    /// add timing marker (prints time elapsed so far)
    void printSubtime(const char* text);

private:
    const char* m_name;
    uint64_t m_startTime;
    uint64_t m_prevMarkerTime;
};

/// a timer that starts at the beginning of the scope
/// does not print anything
class CORE_SYSTEM_API ScopeTimer
{
public:
    ScopeTimer();

    /// get time elapsed since the timer started
    double timeElapsed() const;

    /// get time elapsed since the timer started in mili seconds
    float milisecondsElapsed() const;

    //
        
    void print(IFormatStream& f) const;

private:
    uint64_t m_startTime;
};

/// statistics timing collector
class CORE_SYSTEM_API TimingStatistics : public NoCopy
{
public:
    TimingStatistics();

    void clear();

    void update(double value);

    double mean() const;
    double variance() const;

private:
    double bestValue = DBL_MAX;
    double worstValue = -DBL_MAX;

    double currentMean = 0.0;
    double currentVariance = 0.0;
    double currentSum = 0.0;

    uint32_t count = 0;
};

END_BOOMER_NAMESPACE()
