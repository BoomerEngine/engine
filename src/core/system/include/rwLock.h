/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading #]
***/

#pragma once

#include "algorithms.h"
#include "atomic.h"

BEGIN_BOOMER_NAMESPACE()

class RWLock;

//-----------------------------------------------------------------------------

namespace impl
{
/// Read part of RW lock
class RWLockReader : public NoCopy
{
public:
    INLINE RWLockReader(RWLock* internal)
        : m_internal(internal)
    {}

    INLINE void acquire();
    INLINE void release();

private:
    RWLock* m_internal;
};

/// Read part of RW lock
class RWLockWriter : public NoCopy
{
public:
    INLINE RWLockWriter(RWLock* internal)
        : m_internal(internal)
    {}

    INLINE void acquire();
    INLINE void release();

private:
    RWLock* m_internal;
};
}

//-----------------------------------------------------------------------------

/// Simple RW lock
/// TODO: implement
class RWLock : public NoCopy
{
public:
INLINE RWLock()
    : m_reader(this)
    , m_writer(this)
{}

INLINE ~RWLock()
{}

INLINE void acquireRead()
{
    m_lock.acquire();
}

INLINE void releaseRead()
{
    m_lock.release();
}

INLINE void acquireWrite()
{
    m_lock.acquire();
}

INLINE void releaseWrite()
{
    m_lock.release();
}

//--

INLINE impl::RWLockReader& reader()
{
    return m_reader;
}

INLINE impl::RWLockWriter& writer()
{
    return m_writer;
}

private:
Mutex m_lock;
impl::RWLockReader m_reader;
impl::RWLockWriter m_writer;
};

//-----------------------------------------------------------------------------

namespace impl
{

INLINE void RWLockReader::acquire()
{
    m_internal->acquireRead();
}

INLINE void RWLockReader::release()
{
    m_internal->releaseRead();
}

INLINE void RWLockWriter::acquire()
{
    m_internal->acquireWrite();
}

INLINE void RWLockWriter::release()
{
    m_internal->releaseWrite();
}

} // impl

END_BOOMER_NAMESPACE()
