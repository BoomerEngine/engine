/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

#include "atomic.h"
#include "spinLock.h"

BEGIN_BOOMER_NAMESPACE()

typedef std::function<void()> TCallbackUnregisterFunction;

/// internals of the registration token/owner
class CORE_SYSTEM_API RegistrationTokenHolder
{
public:
    INLINE RegistrationTokenHolder(const TCallbackUnregisterFunction& func)
        : m_func(func)
        , m_ownerRefCount(1)
        , m_totalRefCount(1)
    {}

    void releaseOwnerRef();
    void releaseUserRef();
    void addUserRef();
    void call();

private:
    ~RegistrationTokenHolder();

    TCallbackUnregisterFunction m_func;
    std::atomic<uint32_t> m_ownerRefCount;
    std::atomic<uint32_t> m_totalRefCount;
    SpinLock m_lock;
};

//! Unregistration token
/// Should be returned always when stuff is getting registered somewhere
/// It's the responsibility of the caller to keep this around as long as we want to be registered
class CORE_SYSTEM_API RegistrationToken
{
public:
    RegistrationToken();
    RegistrationToken(RegistrationTokenHolder* token);
    RegistrationToken(const RegistrationToken& other);
    RegistrationToken(RegistrationToken&& other);
    ~RegistrationToken();

    RegistrationToken& operator=(const RegistrationToken& other);
    RegistrationToken& operator=(RegistrationToken&& other);

    /// did we register anywhere ? (NOTE: still true even if expired)
    INLINE bool isRegistered() const { return m_token != nullptr; }

    /// unregister now, keeps the object around
    void unregister();

    /// release the token, should be called in case stuff got unregistered using other means
    void drop();

private:
    RegistrationTokenHolder* m_token;
};

//! Owner of registration data
template< typename T >
class RegistrationData : public NoCopy
{
public:
    INLINE RegistrationData()
            : m_token(nullptr)
    {}

    INLINE RegistrationData(const T& data)
            : m_token(nullptr)
            , m_data(data)
    {}

    INLINE RegistrationData(RegistrationData<T>&& data)
            : m_token(std::move(data.m_token))
            , m_data(std::move(data.m_data))
    {
        data.m_token = nullptr;
    }

    INLINE RegistrationData& operator=(RegistrationData<T>&& data)
    {
        if (this != &data)
        {
            m_token = std::move(data.m_token);
            m_data = std::move(data.m_data);
            data.m_token = nullptr;
        }
        return *this;
    }

    INLINE ~RegistrationData()
    {
        drop();
    }

    /// get the data we registered with, always valid unless we delete it
    INLINE const T& get() const { return m_data; }

    /// unregister ourselves
    INLINE void unregister()
    {
        if (m_token)
        {
            m_token->call();
            drop();
        }
    }

    /// unlink the token, causes the data to never be unregistered
    INLINE void drop()
    {
        if (m_token)
        {
            m_token->releaseOwnerRef();
            m_token->releaseUserRef();
            m_token = nullptr;
        }
    }

    /// reset to new values, returns registration token to keep
    INLINE RegistrationToken setup(const T& data, const TCallbackUnregisterFunction& unregisterFunc)
    {
        // drop current data
        drop();

        // create new token
        m_token = new RegistrationTokenHolder(unregisterFunc);
        m_data = data;

        // return the token
        // NOTE: if it's not captured we will get unregistered right away
        return RegistrationToken(m_token);
    }

    /// compare by value
    INLINE bool operator==(const T& data) const
    {
        return m_data == data;
    }

    /// compare by value
    INLINE bool operator!=(const T& data) const
    {
        return m_data != data;
    }

    /// compare by value
    INLINE bool operator==(const RegistrationData<T>& data) const
    {
        return m_data == data.get();
    }

    /// compare by value
    INLINE bool operator!=(const RegistrationData<T>& data) const
    {
        return m_data != data.get();
    }

private:
    T m_data; // data that was registered, can be anything
    RegistrationTokenHolder* m_token; // registration token
};

END_BOOMER_NAMESPACE()
