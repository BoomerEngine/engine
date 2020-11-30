/***
* Boomer Engine v4
* Written by Tomasz "RexDex" Jonarski
*
* [#filter: http\curl #]
***/

#pragma once

#include <curl/curl.h>

#include "requestServer.h"

#include "base/system/include/thread.h"
#include "base/system/include/timing.h"
#include "base/socket/include/udpSocket.h"
#include "base/socket/include/address.h"
#include "base/containers/include/pagedBuffer.h"


namespace base
{
    namespace curl
    {

        //---

        /// a "connection" to a server, allows for reusing CURL objects
        class MultiConnection : public http::Connection
        {
        public:
            MultiConnection(RequestQueue* queue, StringView address, StringView protocol);
            virtual ~MultiConnection();

            // return CURL object after it's finished
            void returnRequestObject(CURL* handle);

            /// process a request and call a callback function once it's completed
            virtual void send(StringView url, const http::RequestArgs& params, const http::TRequestResponseFunc& service, http::Method method, uint32_t timeOut) override final;

        private:
            Array<CURL*> m_freeHandles;

            SpinLock m_freeHandlesLock;

            std::atomic<uint32_t> m_totalSentRequests;
            std::atomic<uint32_t> m_numActiveRequests;
            std::atomic<uint32_t> m_maxConcurentRequests;

            StringBuf m_url;

            RequestQueue* m_queue;

            //--
                
            CURL* allocRequestObject();
        };

        //---

        /// a single pending CURL request
        class Request : public base::NoCopy
        {
            RTTI_DECLARE_POOL(POOL_HTTP_REQUEST)

        public:
            Request(CURL* handle, const StringBuf& url, const StringBuf& fields, uint32_t timeout, RefWeakPtr<MultiConnection> owner, http::Method method, const http::TRequestResponseFunc& callback);
            ~Request();

            INLINE CURL* handle() const { return m_handle; }
                
            INLINE const NativeTimePoint& timeout() const { return m_timeoutTime; }


            void signalTimeout();
            void signalFinished(uint32_t code);

        private:
            StringBuf m_url;
            StringBuf m_fields;
            bool m_finished;

            http::Method m_method;

            NativeTimePoint m_timeoutTime;
            NativeTimePoint m_sentTime;

            PagedBuffer m_data;

            CURL* m_handle;

            http::TRequestResponseFunc m_callback;

            RefWeakPtr<MultiConnection> m_owner;

            //--

            static size_t WriteFunc(void* ptr, size_t size, size_t nmemb, Request* self);
        };

        //---

        /// CURL based request service
        class RequestQueue : public base::NoCopy
        {
        public:
            RequestQueue();
            ~RequestQueue();

            /// initialize
            bool init();

            /// add request to list, NOTE: the method takes ownership of the pointer
            bool scheduleRequest(Request* request);

        private:
            CURLM* m_multi;
            Thread m_thread;
            std::atomic<uint32_t> m_requestExit;

            Array<Request*> m_pendingRequests;
            SpinLock m_pendingRequestsLock;

            socket::udp::RawSocket m_wakeupSocket;
            socket::udp::RawSocket m_wakeupSenderSocket;
            socket::Address m_wakeupSocketAddress;

            uint32_t calcMinTimeout() const;

            void wakeupThread();
            void serviceThread();
        };

    } // curl
} // base