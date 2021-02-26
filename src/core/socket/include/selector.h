/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: raw #]
***/

#pragma once

#include "core/containers/include/inplaceArray.h"

BEGIN_BOOMER_NAMESPACE_EX(socket)

//--

enum class SelectorEvent : uint8_t
{
    Ready, // we have so shit ready
    Busy, // we don't have anything ready but we are not dead yet
    Error, // we are dead
};

enum class SelectorOp : uint8_t
{
    Read,
    Write,
};

struct SelectorResult
{
    SocketType socket = 0;
    bool error = false;
};

// helper class to facilitate waiting for active socket
class CORE_SOCKET_API Selector : public NoCopy
{
public:
    Selector();

    // wait for something to become readable, sets the iterator
    SelectorEvent wait(SelectorOp op, const SocketType* sockets, uint32_t numSockets, uint32_t timeoutMs);

    //--

    // iterator to the list of reported sockets
    INLINE ConstArrayIterator<SelectorResult> begin() const { return m_result.begin(); }
    INLINE ConstArrayIterator<SelectorResult> end() const { return m_result.end(); }

private:
    InplaceArray<SelectorResult, 64> m_result;
    InplaceArray<uint8_t, 256> m_internalData;
};

//--

END_BOOMER_NAMESPACE_EX(socket)
