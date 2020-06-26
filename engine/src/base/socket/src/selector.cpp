/***
* Boomer Engine v4
* Written by Łukasz "Krawiec" Krawczyk
*
* [#filter: raw #]
***/

#include "build.h"
#include "selector.h"
#include "udpSocket.h"

#if defined(PLATFORM_WINAPI)
    #include <winsock2.h>
#elif defined(PLATFORM_LINUX)
    #include <sys/socket.h>
    #include <sys/ioctl.h>
    #include <sys/poll.h>
#endif

namespace base
{
    namespace socket
    {
        //--

        Selector::Selector()
        {}

        SelectorEvent Selector::wait(SelectorOp op, const SocketType* sockets, uint32_t numSockets, uint32_t timeoutMs)
        {
#if defined(PLATFORM_POSIX)
            // prepare memory for the params for poll()
            m_internalData.reset();
            m_internalData.reserve(sizeof(pollfd) * numSockets);

            // write the socket descriptors
            auto pollList  = (pollfd*) m_internalData.allocateUninitialized(sizeof(pollfd) * numSockets);
            for (uint32_t i=0; i<numSockets; ++i)
            {
                pollList[i].events = (op == SelectorOp::Read) ? POLLIN : POLLOUT;
                pollList[i].fd = sockets[i];
                pollList[i].revents = 0;
            }

            // wait for data
            auto ret = poll(pollList, numSockets, timeoutMs);
            if (ret < 0)
            {
                TRACE_ERROR("poll() error: {}", GetSocketError());
                return SelectorEvent::Error;
            }
            else if (ret == 0)
            {
                return SelectorEvent::Busy;
            }

            // look for data
            result.reset();
            result.reserve(ret);
            for (uint32_t i=0; i<numSockets; ++i)
            {
                if (pollList[i].revents != 0)
                {
                    auto& result = result.emplaceBack();
                    result.socket = pollList[i].fd;

                    if (pollList[i].revents & POLLERR)
                    {
                        TRACE_ERROR("poll(): got POLLERR for {}, Index: {}", result.socket, i);
                        result.error = true;
                    }
                    else if (pollList[i].revents & POLLNVAL)
                    {
                        TRACE_ERROR("poll(): got POLLNVAL for {}, Index: {}", result.socket, i);
                        result.error = true;
                    }
                    else if (pollList[i].revents & POLLHUP)
                    {
                        TRACE_ERROR("poll(): got POLLHUP for {}, Index: {}", result.socket, i);
                        result.error = true;
                    }
                }
            }

            // we have some data
            return SelectorEvent::Ready;
#elif defined(PLATFORM_WINAPI)
            timeval timeout;
            timeout.tv_sec = timeoutMs / 1000;
            timeout.tv_usec = (timeoutMs % 1000) * 1000;

            m_internalData.reset();
            m_internalData.reserve(sizeof(fd_set) * 2);

            auto socketSet  = (fd_set*) m_internalData.allocateUninitialized(sizeof(fd_set));
            FD_ZERO(socketSet);

			auto errorSocketSet  = (fd_set*)m_internalData.allocateUninitialized(sizeof(fd_set));
			FD_ZERO(errorSocketSet);

            uint32_t maxIndex = 0;
            for (uint32_t i=0; i<numSockets; ++i)
            {
                FD_SET(sockets[i], socketSet);
				FD_SET(sockets[i], errorSocketSet);
                maxIndex = std::max<uint32_t>(sockets[i], maxIndex);
            }

            int result = select((int)maxIndex + 1, (op == SelectorOp::Read) ? socketSet : nullptr, (op == SelectorOp::Write) ? socketSet : nullptr, errorSocketSet, &timeout);
            if (result < 0)
            {
                int error = GetSocketError();
                if (WouldBlock(error))
                    return SelectorEvent::Busy;

                TRACE_ERROR("Selector failed with error {}", error);
                return SelectorEvent::Error;
            }
            else if (result == 0)
            {
                return SelectorEvent::Busy;
            }
            else
            {
                m_result.reset();
                m_result.reserve(result);

                for (uint32_t i=0; i<numSockets; ++i)
                {
					if (FD_ISSET(sockets[i], errorSocketSet))
					{
						auto& result = m_result.emplaceBack();
						result.socket = sockets[i];
						result.error = true;
						FD_CLR(sockets[i], socketSet);
					}
                    else if (FD_ISSET(sockets[i], socketSet))
                    {
						auto& result = m_result.emplaceBack();
						result.socket = sockets[i];
						result.error = false;
                        FD_CLR(sockets[i], socketSet);
                    }
                }

				return SelectorEvent::Ready;
            }
#else
            // invalid platform
            return SelectorEvent::Error;
#endif
        }

    } // socket
} // base