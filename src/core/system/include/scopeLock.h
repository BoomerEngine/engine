/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading #]
***/

#pragma once

#include "algorithms.h"
#include "mutex.h"

BEGIN_BOOMER_NAMESPACE()

/// Scope lock interface for generic lock
template< typename T = Mutex >
class ScopeLock : public NoCopy
{
public:
    //! Constructor, lock critical section
    INLINE ScopeLock(const T& lock)
        : m_lock(const_cast<T*>(&lock))
        , m_locked(true)
    {
        m_lock->acquire();
    }

    //! Move constructor
    INLINE ScopeLock(ScopeLock<T>&& other)
        : m_lock(other.m_lock)
        , m_locked(other.m_locked)
    {
        other.m_lock = nullptr;
        other.m_locked = false;
    }

    //! Destructor, unlock critical section
    INLINE ~ScopeLock()
    {
        release();
    }

    //! Assing a new lock
    INLINE ScopeLock& operator=(ScopeLock<T>&& other)
    {
        if (this != &other)
        {
            release();

            m_lock = other.m_lock;
            m_locked = other.m_locked;
            other.m_lock = nullptr;
            other.m_locked = false;
        }

        return *this;
    }

    //! Manually release lock
    INLINE void release()
    {
        if (m_locked)
        {
            m_locked = false;
            m_lock->release();
        }
    }

    //! Manually reacquire the lock
    INLINE void aquire()
    {
        if (!m_locked)
        {
            m_locked = true;
            m_lock->acquire();
        }
    }

private:
    T* m_lock; //!< Lock object that is locked and unlocked
    bool m_locked; //!< Are we locked by this scope lock ?
};

/// Scope lock interface for generic lock
template< typename T >
ScopeLock<T> CreateLock(const T& lock)
{
    return ScopeLock<T>(lock);
}

END_BOOMER_NAMESPACE()

