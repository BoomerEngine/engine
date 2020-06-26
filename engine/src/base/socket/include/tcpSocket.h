/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: tcp #]
***/

#pragma once

#include "baseSocket.h"
#include "base/system/include/timing.h"

namespace base
{
    namespace socket
    {
        namespace tcp
        {
            //--

            /// connection stats
            struct BASE_SOCKET_API ConnectionStats
            {
                NativeTimePoint startTime;
                uint64_t totalDataSent = 0;
                uint64_t totalDataReceived = 0;

                void print(IFormatStream& f) const;
            };

            //---

            /// Raw TCP socket wrapper
            class BASE_SOCKET_API RawSocket : public BaseSocket
            {
            public:
                INLINE RawSocket() = default;
                INLINE RawSocket(RawSocket&& other) = default;
                INLINE RawSocket& operator=(RawSocket&& other) = default;

                //---

                // create listener and bind it to given port
                bool listen(const Address& address, Address* outLocalAddress = nullptr);

                // connect to given address
                bool connect(const Address& address, Address* outLocalAddress = nullptr);

                // accept incoming connection
                bool accept(const RawSocket& listener, Address* outSourceAddress = nullptr);

                // Receive data from the socket
                int receive(void* data, int size);

                // Send data through the socket
                int send(const void* data, int size);
            };

            //---

        } // tcp
    } // socket
} // base