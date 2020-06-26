/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [#filter: messages\tcp #]
***/

#include "build.h"
#include "tcpMessageClient.h"
#include "tcpMessageServer.h"

#include "base/test/include/gtest/gtest.h"

DECLARE_TEST_FILE(TcpMessageTest);

using namespace base;
using namespace base::net;

namespace test
{

    //--

    struct HelloWorldMessage
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(HelloWorldMessage);

    public:
        StringBuf m_text;
    };

    RTTI_BEGIN_TYPE_STRUCT(HelloWorldMessage);
        RTTI_PROPERTY(m_text).metadata<replication::SetupMetadata>("maxLength:20");
    RTTI_END_TYPE();

    //--

    struct AnswerToTheBigQuestionMessage
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(AnswerToTheBigQuestionMessage);

    public:
        uint32_t m_answer;
    };

    RTTI_BEGIN_TYPE_STRUCT(AnswerToTheBigQuestionMessage);
        RTTI_PROPERTY(m_answer).metadata<replication::SetupMetadata>("u:10");
    RTTI_END_TYPE();

    //--

}

//--

namespace test
{
    //--

    class HelloReply : public IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(HelloReply, IObject);

    public:
        void messageHello(const HelloWorldMessage& msg, const net::MessageConnectionPtr& con)
        {
            TRACE_INFO("Hello received: '{}'", msg.m_text);
            m_text = msg.m_text;
            m_helloReceived = true;

            AnswerToTheBigQuestionMessage response;
            response.m_answer = 42;
            con->send(1, response);
        }

        bool m_helloReceived = false;
        StringBuf m_text;
    };

    RTTI_BEGIN_TYPE_CLASS(HelloReply);
        RTTI_FUNCTION("messageHello", messageHello);
    RTTI_END_TYPE();

    //--

    class AnswerCapture : public IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(AnswerCapture, IObject);

    public:
        void messageAnswer(const AnswerToTheBigQuestionMessage& msg)
        {
            m_answerReceived = true;
            m_answer = msg.m_answer;
        }

        bool m_answerReceived = false;
        uint32_t m_answer = 0;
    };

    RTTI_BEGIN_TYPE_CLASS(AnswerCapture);
        RTTI_FUNCTION("messageAnswer", messageAnswer);
    RTTI_END_TYPE();

    //--

} // test

TEST(TcpMessage,ClientServerPassing)
{
    uint16_t port = 12000;

    auto server  = CreateSharedPtr<TcpMessageServer>();
    auto serverObject  = CreateSharedPtr<test::HelloReply>();
    server->attachObject(1, serverObject);

    auto client  = CreateSharedPtr<TcpMessageClient>();
    auto clientObject  = CreateSharedPtr<test::AnswerCapture>();
    client->attachObject(1, clientObject);

    while (!server->startListening(++port)) {};
    ASSERT_TRUE(server->isListening());

    ASSERT_TRUE(client->connect(socket::Address::Local4(port)));
    ASSERT_TRUE(client->isConnected());

    test::HelloWorldMessage hello;
    hello.m_text = "Hello, network!";
    client->send(1, hello);

    while (!serverObject->m_helloReceived)
    {
        Sleep(500);
        server->executePendingMessages();
    }

    ASSERT_TRUE(serverObject->m_helloReceived);
    ASSERT_STREQ(hello.m_text.c_str(), serverObject->m_text.c_str());

    if (!clientObject->m_answerReceived)
    {
        Sleep(500);
        client->executePendingMessages();
    }

    ASSERT_TRUE(clientObject->m_answerReceived);
    ASSERT_EQ(42, clientObject->m_answer);
}