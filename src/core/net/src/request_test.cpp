/***
* Boomer Engine v4
* Written by Tomasz "RexDex" Jonarski
*
* [#filter: http #]
***/

#include "build.h"
#include "requestArguments.h"

#include "core/test/include/gtest/gtest.h"
#include "core/system/include/thread.h"

DECLARE_TEST_FILE(Requests);

BEGIN_BOOMER_NAMESPACE_EX(http)

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

    auto data = StringView((const char*)res.data.data(), res.data.size());
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

        auto data = StringView((const char*)res.data.data(), res.data.size());
        TRACE_INFO("Returned IP: '{}'", data);
    }
}

TEST_F(RequestTest, TestMultipleFromFibers)
{
    auto connection = service().connect("http://ifconfig.me");
    ASSERT_TRUE(connection);

    auto wait = CreateFence("CURLTest", 200);

    for (uint32_t i = 0; i < 200; ++i)
    {
        RunFiber("SendRequest") << [wait, connection](FIBER_FUNC)
        {
            auto res = connection->wait("", RequestArgs(), Method::GET);
            EXPECT_EQ(200, res.code);
            ASSERT_TRUE(res.data != nullptr);
            ASSERT_GT(20, res.data.size());
            ASSERT_LT(10, res.data.size());

            auto data = StringView((const char*)res.data.data(), res.data.size());
            TRACE_INFO("Returned IP: '{}'", data);

            SignalFence(wait);
        };

        Sleep(10);
    }

    WaitForFence(wait);
}
#endif
//---

END_BOOMER_NAMESPACE_EX(http)