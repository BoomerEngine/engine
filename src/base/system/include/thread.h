/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading #]
***/

#pragma once

#ifdef Yield
#undef Yield
#endif

BEGIN_BOOMER_NAMESPACE(base)

//-----------------------------------------------------------------------------

/// Thread priority
enum class ThreadPriority : uint8_t
{
    Normal,
    AboveNormal,
    BelowNormal,
};

//-----------------------------------------------------------------------------

/// Thread function
typedef std::function<void()> TThreadFunc;

//-----------------------------------------------------------------------------

// thread setup
struct BASE_SYSTEM_API ThreadSetup : public base::NoCopy
{
    const char* m_name;  // debug name of the thread

    ThreadPriority m_priority; //  thread priority
    uint32_t m_stackSize; // thead stack size
    uint32_t m_affinity; // affinity to CPU cores on the system

    TThreadFunc m_function; // thread function to run

    ThreadSetup();
};

//-----------------------------------------------------------------------------

/// This is the base thread interface for all runnable thread classes
class BASE_SYSTEM_API Thread : public base::NoCopy
{
public:
    Thread();
    Thread(Thread&& other);
    Thread& operator=(Thread&& other);
    ~Thread(); // NOTE: closes automatically

    //! initialize thread
    void init(const ThreadSetup& setup);

    //! close this thread, waits for the thread to finish, allows to call init again
    void close();

protected:
    uint64_t m_systemThreadHandle;
};

//--

//! Get ID of current thread
extern BASE_SYSTEM_API ThreadID GetCurrentThreadID();

//! Get number of cores in the system
extern BASE_SYSTEM_API uint32_t GetNumberOfCores();

//! Change name of current thread
extern BASE_SYSTEM_API void SetThreadName(const char* name);

//! Wait some time, time is in ms
extern BASE_SYSTEM_API void Sleep(uint32_t ms);

//! Yield current thread
extern BASE_SYSTEM_API void Yield();

//--

END_BOOMER_NAMESPACE(base)