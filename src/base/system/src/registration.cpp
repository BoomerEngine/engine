/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#include "build.h"
#include "registration.h"

BEGIN_BOOMER_NAMESPACE(base)

//--

void RegistrationTokenHolder::releaseOwnerRef()
{
    ASSERT_EX(m_totalRefCount.load() > 0, "Object already destroyed");

    if (--m_ownerRefCount == 0)
    {
        m_lock.acquire();
        m_func = TCallbackUnregisterFunction();
        m_lock.release();
    }
}

void RegistrationTokenHolder::releaseUserRef()
{
    if (--m_totalRefCount == 0)
    {
        delete this;
    }
}

void RegistrationTokenHolder::addUserRef()
{
    ASSERT_EX(m_totalRefCount.load() > 0, "Object already destroyed");
    ++m_totalRefCount;
}

void RegistrationTokenHolder::call()
{
    ASSERT_EX(m_totalRefCount.load() > 0, "Object already destroyed");

    m_lock.acquire();
    auto func = m_func;
    m_lock.release();
    if (func)
        func();
}

RegistrationTokenHolder::~RegistrationTokenHolder()
{
    ASSERT_EX(m_totalRefCount.load() == 0, "Object destroyed with non-zero ref count");
    ASSERT_EX(m_ownerRefCount.load() == 0, "Object destroyed with non-zero ref count");
}

//--

RegistrationToken::RegistrationToken()
    : m_token(nullptr)
{}

RegistrationToken::RegistrationToken(RegistrationTokenHolder* token)
    : m_token(token)
{
    m_token->addUserRef();
}

RegistrationToken::RegistrationToken(const RegistrationToken& other)
    : m_token(other.m_token)
{
    if (m_token)
        m_token->addUserRef();
}

RegistrationToken::RegistrationToken(RegistrationToken&& other)
{
    m_token = other.m_token;
    other.m_token = nullptr;
}

RegistrationToken::~RegistrationToken()
{
    unregister();
}

RegistrationToken& RegistrationToken::operator=(const RegistrationToken& other)
{
    if (this != &other)
    {
        unregister();
        m_token = other.m_token;
        if (m_token)
            m_token->addUserRef();
    }

    return *this;
}

RegistrationToken& RegistrationToken::operator=(RegistrationToken&& other)
{
    if (this != &other)
    {
        unregister();
        m_token = other.m_token;
        other.m_token = nullptr;
    }

    return *this;

}

void RegistrationToken::unregister()
{
    if (m_token)
    {
        m_token->call();
        drop();
    }
}

void RegistrationToken::drop()
{
    if (m_token)
    {
        m_token->releaseUserRef();
        m_token = nullptr;
    }
}

END_BOOMER_NAMESPACE(base)
