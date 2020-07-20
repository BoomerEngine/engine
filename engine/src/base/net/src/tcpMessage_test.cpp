/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [#filter: messages\tcp #]
***/

#include "build.h"
#include "messagePool.h"
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
            con->send(response);
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
    // create server, on any port
    auto server  = CreateSharedPtr<TcpMessageServer>();
    ASSERT_TRUE(server->startListening(0));
    ASSERT_TRUE(server->isListening());

    // create client and connect it
    auto client = CreateSharedPtr<TcpMessageClient>();
    ASSERT_TRUE(client->connect(socket::Address::Local4(server->listeningAddress().port())));
    ASSERT_TRUE(client->isConnected());

    // server should report a connection
    MessageConnectionPtr connectionOnServerSide;
    {
        auto timeout = NativeTimePoint::Now() + 1.0;
        while (!timeout.reached())
        {
            if (connectionOnServerSide = server->pullNextAcceptedConnection())
                break;
            Sleep(100);
        }
    }
    ASSERT_TRUE(connectionOnServerSide);

    // send message to server
    {
        test::HelloWorldMessage hello;
        hello.m_text = "Hello, network!";
        client->send(hello);
    }

    // wait for message on the server side
    MessagePtr receivedMessage;
    {
        auto timeout = NativeTimePoint::Now() + 1.0;
        while (!timeout.reached())
        {
            if (receivedMessage = connectionOnServerSide->pullNextMessage())
                break;
            Sleep(100);
        }
    }
    ASSERT_TRUE(receivedMessage);

    // dispatch the message to the object, it should be parsed correctly
    {
        auto serverObject = CreateSharedPtr<test::HelloReply>();
        ASSERT_TRUE(receivedMessage->dispatch(serverObject, connectionOnServerSide));
        ASSERT_TRUE(serverObject->m_helloReceived);
        ASSERT_STREQ("Hello, network!", serverObject->m_text.c_str());
        receivedMessage = nullptr;
    }

    // wait for the response
    {
        auto timeout = NativeTimePoint::Now() + 1.0;
        while (!timeout.reached())
        {
            if (receivedMessage = client->pullNextMessage())
                break;
            Sleep(100);
        }
    }
    ASSERT_TRUE(receivedMessage);

    // dispatch answer
    {
        auto clientObject = CreateSharedPtr<test::AnswerCapture>();
        ASSERT_TRUE(receivedMessage->dispatch(clientObject));
        ASSERT_TRUE(clientObject->m_answerReceived);
        ASSERT_EQ(42, clientObject->m_answer);
        receivedMessage = nullptr;
    }

    // close the connection
    connectionOnServerSide->close();
    connectionOnServerSide.reset();

    // make sure client registers server closing connection
    {
        auto timeout = NativeTimePoint::Now() + 5.0;
        while (!timeout.reached())
        {
            if (!client->isConnected())
            {
                client.reset();
                break;
            }
            Sleep(100);
        }
    }
    ASSERT_FALSE(client);

    // destroy server object now
    server.reset();
}