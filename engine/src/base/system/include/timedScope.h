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
    /// a scope that prints its timing to log
    /// used for small profiling, can be picked up the automated tools
    class BASE_SYSTEM_API TimedScope
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
    class BASE_SYSTEM_API ScopeTimer
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

}  // base