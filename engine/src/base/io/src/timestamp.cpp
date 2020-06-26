/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#include "build.h"
#include "timestamp.h"

#ifdef PLATFORM_WINDOWS
    #include <Windows.h>
#elif defined(PLATFORM_POSIX)
    #include <time.h>
#endif

namespace base
{
    namespace io
    {

        StringBuf TimeStamp::toDisplayString() const
        {
#ifdef PLATFORM_WINDOWS

            // Convert to file time
            SYSTEMTIME stUTC, stLocal;

            // Convert the last-write time to local time.
            FileTimeToSystemTime((FILETIME*)&m_timecode, &stUTC);
            SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

            // Build a string showing the date and time.
            return TempString("{}/{}/{} {}:{}:{}", stLocal.wYear, stLocal.wMonth, stLocal.wDay, stLocal.wHour, stLocal.wMinute, stLocal.wSecond);
#elif defined(PLATFORM_POSIX)
            // get the time64_t
            time_t t = m_timecode / 10000000ULL - 11644473600ULL;

            // get string representation
            tm timeData;
            auto timeDataPtr  = gmtime_r(&t, &timeData);

            // build string
            return TempString("{}/{}/{} {}:{}:{}", timeDataPtr->tm_year + 1900, timeDataPtr->tm_mon + 1, timeDataPtr->tm_mday, timeDataPtr->tm_hour, timeDataPtr->tm_min, timeDataPtr->tm_sec);
#else

            // Unable to convert on other platforms
            return "Unknown";

#endif
        }

        StringBuf TimeStamp::toSafeString() const
        {
#ifdef PLATFORM_WINDOWS

            // Convert to file time
            SYSTEMTIME stUTC, stLocal;

            // Convert the last-write time to local time.
            FileTimeToSystemTime((FILETIME*)&m_timecode, &stUTC);
            SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

            // Build a string showing the date and time.
            return TempString("{}_{}_{}-{}_{}_{}", stLocal.wYear, stLocal.wMonth, stLocal.wDay, stLocal.wHour, stLocal.wMinute, stLocal.wSecond);

#elif defined(PLATFORM_POSIX)

            // get the time64_t
            time_t t = m_timecode / 10000000ULL - 11644473600ULL;

            // get string representation
            tm timeData;
            auto timeDataPtr  = gmtime_r(&t, &timeData);

            // build string
            return TempString("{}_{}_{}-{}_{}_{}", timeDataPtr->tm_year + 1900, timeDataPtr->tm_mon + 1, timeDataPtr->tm_mday, timeDataPtr->tm_hour, timeDataPtr->tm_min, timeDataPtr->tm_sec);

#else

            // Unable to convert on other platforms
            return "Unknown";

#endif
        }

        TimeStamp TimeStamp::GetNow()
        {
#ifdef PLATFORM_WINDOWS
            SYSTEMTIME stUTC;
            GetSystemTime(&stUTC);

            FILETIME fileTime;
            SystemTimeToFileTime(&stUTC, &fileTime);

            ULARGE_INTEGER ull;
            ull.LowPart = fileTime.dwLowDateTime;
            ull.HighPart = fileTime.dwHighDateTime;

            uint64_t val = *(uint64_t*)&fileTime;
            return TimeStamp(val);
#elif defined(PLATFORM_POSIX)
            time_t t;
            time(&t);

            uint64_t val = (uint64_t)t * 10000000 + 116444736000000000;
            return TimeStamp(val);
#else
    #error "Unimplemented"
#endif
        }

        TimeStamp TimeStamp::GetFromFileTime(uint64_t seconds, uint64_t nanoSeconds)
        {
            uint64_t val = (uint64_t)seconds * 10000000 + 116444736000000000;
            return TimeStamp(val);
        }

    } // io

} // base
