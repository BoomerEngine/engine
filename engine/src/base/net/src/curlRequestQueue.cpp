/***
* Boomer Engine v4
* Written by Tomasz "RexDex" Jonarski
*
* [#filter: http\curl #]
***/

#include "build.h"
#include "requestArguments.h"
#include "curlRequestQueue.h"
#include "base/system/include/thread.h"

#include <curl/multi.h>

namespace base
{
    namespace curl
    {

        //---

        base::ConfigProperty<int> cvDefaultRequestTimeout("Engine.CURL", "RequestTimeout", 5000);
        base::ConfigProperty<int> cvDefaultLoopTimeout("Engine.CURL", "LoopTimeout", 1000);

        mem::PoolID POOL_REQUEST_DATA("Engine.HTTP.RequestData");

        //---

        MultiConnection::MultiConnection(RequestQueue* queue, StringView<char> address, StringView<char> protocol)
            : Connection(address, protocol)
            , m_totalSentRequests(0)
            , m_numActiveRequests(0)
            , m_queue(queue)
        {
            StringBuilder txt;
            if (protocol)
                txt << protocol << "://";
            else
                txt << "http://";

            txt << address;

            if (!address.endsWith("/"))
                txt << "/";

            m_url = txt.toString();
        }

        MultiConnection::~MultiConnection()
        {
            TRACE_INFO("Closing request connection to '{}', {} total request(s) sent, {} max concurent request(s)", 
                address(), m_totalSentRequests.load(), m_maxConcurentRequests.load());

            for (auto curl  : m_freeHandles)
                curl_easy_cleanup(curl);
        }

        CURL* MultiConnection::allocRequestObject()
        {
            // reuse handle
            {
                auto lock  = CreateLock(m_freeHandlesLock);
                if (!m_freeHandles.empty())
                {
                    auto ret  = m_freeHandles.back();
                    AtomicMax(m_maxConcurentRequests, ++m_numActiveRequests);
                    m_freeHandles.popBack();
                    return ret;
                }
            }

            // create new handle
            auto ret  = curl_easy_init();
            if (ret)
            {
                curl_easy_setopt(ret, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
                curl_easy_setopt(ret, CURLOPT_NOSIGNAL, 1);
                curl_easy_setopt(ret, CURLOPT_TCP_KEEPALIVE, 1);
                curl_easy_setopt(ret, CURLOPT_TCP_KEEPIDLE, 30);
                curl_easy_setopt(ret, CURLOPT_TCP_KEEPINTVL, 5);
                curl_easy_setopt(ret, CURLOPT_SSL_VERIFYPEER, 0L);
                curl_easy_setopt(ret, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
                AtomicMax(m_maxConcurentRequests, ++m_numActiveRequests);
            }

            return ret;
        }

        void MultiConnection::returnRequestObject(CURL* handle)
        {
            if (handle)
            {
                auto lock  = CreateLock(m_freeHandlesLock);
                m_freeHandles.pushBack(handle);

                --m_numActiveRequests;
            }
        }

        void MultiConnection::send(StringView<char> url, const http::RequestArgs& params, const http::TRequestResponseFunc& callback, http::Method method, uint32_t timeOut)
        {
            auto handle  = allocRequestObject();
            if (!handle)
            {
                http::RequestResult ret;
                ret.scheduleTime.resetToNow();
                ret.completionTime.resetToNow();
                ret.code = 500;

                if (callback)
                    callback(ret);
                return;
            }

            ++m_totalSentRequests;

            auto santitizedUrl  = url.beforeFirstOrFull("?");
            if (santitizedUrl.beginsWith("/"))
                santitizedUrl = santitizedUrl.subString(1);
            if (santitizedUrl.endsWith("/"))
                santitizedUrl = santitizedUrl.leftPart(santitizedUrl.length() - 1);

            StringBuilder fullUrl, paramsText;

            fullUrl << m_url;
            fullUrl << santitizedUrl;

            if (method == http::Method::POST)
            {
                paramsText << params;
                TRACE_INFO("CURL POST: '{}' '{}'", fullUrl, params);
            }
            else
            {
                fullUrl << params;
                TRACE_INFO("CURL GET: '{}'", fullUrl);
            }

            auto request = MemNew(Request, handle, fullUrl.toString(), paramsText.toString(), timeOut, this, method, callback);
            m_queue->scheduleRequest(request);
        }

        //---

        //static mem::PageAllocator RequestPageAllocator(POOL_REQUEST_DATA);

        Request::Request(CURL* handle, const StringBuf& url, const StringBuf& fields, uint32_t timeout, RefWeakPtr<MultiConnection> owner, http::Method method, const http::TRequestResponseFunc& callback)
            : m_url(url)
            , m_fields(fields)
            , m_finished(false)
            , m_callback(callback)
            , m_data(POOL_REQUEST_DATA)
            , m_owner(owner)
            , m_method(method)
            , m_handle(handle)
        {
            // setup timeout
            auto validTimeout  = std::clamp<uint32_t>(timeout ? timeout : cvDefaultRequestTimeout.get(), 0, 60000);
            m_sentTime.resetToNow();
            m_timeoutTime = timeout ? (m_sentTime + NativeTimeInterval(timeout / 1000.0)) : NativeTimePoint();

            // use the specified timeout or, if not specified the default one
            curl_easy_setopt(m_handle, CURLOPT_TIMEOUT_MS, timeout);
            curl_easy_setopt(m_handle, CURLOPT_WRITEFUNCTION, &Request::WriteFunc);
            curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, this);
            curl_easy_setopt(m_handle, CURLOPT_PRIVATE, this);
            curl_easy_setopt(m_handle, CURLOPT_URL, m_url.c_str());
            curl_easy_setopt(m_handle, CURLOPT_POSTFIELDS, m_fields.c_str());

            // request type
            if (method == http::Method::POST)
                curl_easy_setopt(m_handle, CURLOPT_CUSTOMREQUEST, "POST");
            else if (method == http::Method::GET)
                curl_easy_setopt(m_handle, CURLOPT_CUSTOMREQUEST, "GET");
            else if (method == http::Method::DEL)
                curl_easy_setopt(m_handle, CURLOPT_CUSTOMREQUEST, "DELETE");
        }

        Request::~Request()
        {
            ASSERT(m_finished);

            if (auto owner  = m_owner.lock())
                owner->returnRequestObject(m_handle);
            else
                curl_easy_cleanup(m_handle);
        }

        void Request::signalTimeout()
        {
            ASSERT(!m_finished);
            m_finished = true;

            if (m_callback)
            {
                http::RequestResult result;
                result.scheduleTime = m_sentTime;
                result.completionTime.resetToNow();
                result.code = 404;
                m_callback(result);
            }
        }

        void Request::signalFinished(uint32_t code)
        {
            ASSERT(!m_finished);
            m_finished = true;

            if (m_callback)
            {
                http::RequestResult result;
                result.scheduleTime = m_sentTime;
                result.completionTime.resetToNow();
                result.code = code;
                result.data = m_data.toBuffer();
                m_callback(result);
            }
        }

        size_t Request::WriteFunc(void *ptr, size_t size, size_t nmemb, Request* self)
        {
            self->m_data.write(ptr, size * nmemb);
            return size * nmemb;
        }

        //---

        RequestQueue::RequestQueue()
            : m_requestExit(0)
            , m_multi(nullptr)
        {
        }

        RequestQueue::~RequestQueue()
        {
            // close the thread
            TRACE_INFO("Closing CURL thread...");
            m_requestExit.exchange(1);
            wakeupThread();
            m_thread.close();

            // destroy any outstanding requests
            for (auto request  : m_pendingRequests)
            {
                curl_multi_remove_handle(m_multi, request->handle());
                request->signalTimeout();
                MemDelete(request);
            }

            // destroy the CURL handler
            TRACE_INFO("Closing CURL handler...");
            curl_multi_cleanup(m_multi);
            m_multi = nullptr;
        }

        bool RequestQueue::init()
        {
            ASSERT_EX(m_multi == nullptr, "Already initialized");

            // create the extra "wakeup" socket
            m_wakeupSocketAddress = socket::Address::Local4(0);
            socket::Address actualWakeAddres, actualSenderAddress;
            if (!m_wakeupSocket.open(m_wakeupSocketAddress, &actualWakeAddres))
            {
                TRACE_ERROR("Failed to create internal loopback socket");
                return false;
            }
            if (!m_wakeupSenderSocket.open(m_wakeupSocketAddress, &actualSenderAddress))
            {
                TRACE_ERROR("Failed to create internal loopback sender socket");
                return false;
            }
            
            // info
            TRACE_SPAM("Created internal CURL loopback socket on port {} and sender on port {}", actualWakeAddres.port(), actualSenderAddress.port());

            // socket opened
            m_wakeupSocket.blocking(false);
            m_wakeupSocket.bufferSize(512, 512);
            m_wakeupSenderSocket.blocking(false);
            m_wakeupSenderSocket.bufferSize(512, 512);
            m_wakeupSocketAddress = actualWakeAddres;

            // create CURL BS
            m_multi = curl_multi_init();
            if (!m_multi)
            {
                TRACE_ERROR("Unable to initialize CURL");
                return false;
            }

            // create the servicing thread for the CURL
            ThreadSetup setup;
            setup.m_name = "CURLServiceThread";
            setup.m_priority = ThreadPriority::AboveNormal; // TBD
            setup.m_function = [this]() { serviceThread(); };
            m_thread.init(setup);

            return true;
        }

        void RequestQueue::wakeupThread()
        {
            uint32_t code = 0xDEAD;
            if (sizeof(code) != m_wakeupSenderSocket.send(&code, sizeof(code), m_wakeupSocketAddress))
            {
                TRACE_WARNING("Failed to send wakeup signal via the socket");
            }
        }

        bool RequestQueue::scheduleRequest(Request* request)
        {
            ASSERT_EX(m_multi != nullptr, "Not initialized");

            // add to internal list
            {
                auto lock = CreateLock(m_pendingRequestsLock);
                m_pendingRequests.pushBack(request);
            }

            // start CURL request by adding it to multi handle
            curl_multi_add_handle(m_multi, request->handle());

            // make sure we start processing as soon as possible
            wakeupThread();

            // request was scheduled
            return true;
        }

        uint32_t RequestQueue::calcMinTimeout() const
        {
            auto lock = CreateLock(m_pendingRequestsLock);

            auto minTimeout = cvDefaultLoopTimeout.get();

            for (auto request  : m_pendingRequests)
            {
                if (request->timeout().valid())
                {
                    auto timeLeft = std::max<int>(0, -(int)request->timeout().timeTillNow().toMiliSeconds());
                    minTimeout = std::min(minTimeout, timeLeft);
                }
            }

            return std::min<uint32_t>(50, minTimeout);
        }

        void RequestQueue::serviceThread()
        {
            uint32_t numTotalRequestsServiced = 0;
            TRACE_INFO("Started CURL servicing thread");

            // run for as long as required
            while (0 == m_requestExit.exchange(0))
            {
                // perform processing
                int numStillRunning = 0;
                auto ret = curl_multi_perform(m_multi, &numStillRunning);
                if (ret == CURLM_CALL_MULTI_PERFORM)
                {
                    // we were requested to run again
                    continue;
                }
                else if (ret != CURLM_OK)
                {
                    TRACE_ERROR("CURL error: {}", ret);
                    Sleep(1000); // TODO: better error handing
                    continue;
                }

                // process results
                {
                    struct CURLMsg *m = nullptr;
                    do
                    {
                        int msgq = 0;
                        m = curl_multi_info_read(m_multi, &msgq);
                        if (m && (m->msg == CURLMSG_DONE))
                        {
                            CURL *e = m->easy_handle;

                            // find matching entry
                            Request* request = nullptr;
                            curl_easy_getinfo(e, CURLINFO_PRIVATE, &request);
                            if (request)
                            {
                                if (e == request->handle())
                                {
                                    long httpCode = 0;
                                    curl_easy_getinfo(e, CURLINFO_RESPONSE_CODE, &httpCode);

                                    if (m->data.result == 0)
                                        request->signalFinished(httpCode);
                                    else
                                        request->signalTimeout();

                                    // delete
                                    {
                                        auto lock = CreateLock(m_pendingRequestsLock);
                                        m_pendingRequests.remove(request);
                                    }

                                    // cleanup
                                    curl_multi_remove_handle(m_multi, e);
                                    MemDelete(request);
                                }
                            }
                            else
                            {
                                // remove from multi stack
                                curl_multi_remove_handle(m_multi, e);
                                curl_easy_cleanup(e);
                            }
                        }
                    }
                    while (m);
                }

                // calculate the timeout to wait for, we won't wait for longer than a second any way
                // this time is the time of the closest timeout for any of the pending connections
                auto timeoutMs = calcMinTimeout();

                // use the extra dummy socket as well
                curl_waitfd extraFD;
                memset(&extraFD, 0, sizeof(extraFD));
                extraFD.fd = m_wakeupSocket.systemSocket();
                extraFD.events = CURL_WAIT_POLLIN;

                // ask CURL to wait, include our dummy socket as well
                int numFD = 0;
                auto waitRet = curl_multi_wait(m_multi, &extraFD, 1, timeoutMs, &numFD);

                // try to read stuff from the socket
                {
                    uint32_t data = 0;
                    socket::Address sourceAddress;
                    while (m_wakeupSocket.receive(&data, sizeof(data), &sourceAddress) > 0)
                    {
                    }
                }
            }

            TRACE_INFO("Finished CURL servicing thread, {} request(s) serviced", numTotalRequestsServiced);
        }

        //---

    } // curl
} // base