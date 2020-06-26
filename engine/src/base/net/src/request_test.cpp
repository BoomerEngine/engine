﻿/***
* Boomer Engine v4
* Written by Tomasz "RexDex" Jonarski
*
* [#filter: http #]
***/

#include "build.h"
#include "requestArguments.h"

#include "base/test/include/gtest/gtest.h"
#include "base/app/include/localServiceContainer.h"
#include "base/app/include/commandline.h"
#include "base/system/include/thread.h"

DECLARE_TEST_FILE(Requests);

namespace base
{
    namespace http
    {
#if 0
        //---

        class RequestTest : public testing::Test
        {
        public:
            RequestTest()
            {}

            virtual void SetUp() override
            {
                m_service.create();
                auto service = static_cast<app::ILocalService*>(m_service.get());

                app::CommandLine cmdLine;
                cmdLine.param("noRequestServer", "");

                auto ret = service->onInitializeService(cmdLine);
                ASSERT_EQ(app::ServiceInitializationResult::Finished, ret);
            }

            virtual void TearDown() override
            {
                auto service = static_cast<app::ILocalService*>(m_service.get());
                service->onShutdownService();
                m_service.reset();
            }

            INLINE RequestService& service()
            {
                return *m_service;
            }

        private:
            UniquePtr<RequestService> m_service;
        };

        //---

        TEST_F(RequestTest, TestSimple)
        {
            auto connection = service().connect("http://ifconfig.me");
            ASSERT_TRUE(connection);

            auto res = connection->wait("", RequestArgs(), Method::GET);
            EXPECT_EQ(200, res.code);
            ASSERT_TRUE(res.data != nullptr);
            ASSERT_GT(20, res.data.size());
            ASSERT_LT(10, res.data.size());

            auto data = StringView<char>((const char*)res.data.data(), res.data.size());
            TRACE_INFO("Returned IP: '{}'", data);
        }

        TEST_F(RequestTest, TestMultiple)
        {
            auto connection = service().connect("http://ifconfig.me");
            ASSERT_TRUE(connection);

            for (uint32_t i = 0; i < 20; ++i)
            {
                auto res = connection->wait("", RequestArgs(), Method::GET);
                EXPECT_EQ(200, res.code);
                ASSERT_TRUE(res.data != nullptr);
                ASSERT_GT(20, res.data.size());
                ASSERT_LT(10, res.data.size());

                auto data = StringView<char>((const char*)res.data.data(), res.data.size());
                TRACE_INFO("Returned IP: '{}'", data);
            }
        }

        TEST_F(RequestTest, TestMultipleFromFibers)
        {
            auto connection = service().connect("http://ifconfig.me");
            ASSERT_TRUE(connection);

            auto wait = Fibers::GetInstance().createCounter("CURLTest", 200);

            for (uint32_t i = 0; i < 200; ++i)
            {
                RunFiber("SendRequest") << [wait, connection](FIBER_FUNC)
                {
                    auto res = connection->wait("", RequestArgs(), Method::GET);
                    EXPECT_EQ(200, res.code);
                    ASSERT_TRUE(res.data != nullptr);
                    ASSERT_GT(20, res.data.size());
                    ASSERT_LT(10, res.data.size());

                    auto data = StringView<char>((const char*)res.data.data(), res.data.size());
                    TRACE_INFO("Returned IP: '{}'", data);

                    Fibers::GetInstance().signalCounter(wait);
                };

                Sleep(10);
            }

            Fibers::GetInstance().waitForCounterAndRelease(wait);
        }
#endif
        //---

    } // http
} // base