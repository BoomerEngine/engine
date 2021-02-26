/***
* Boomer Engine v4
* Written by Tomasz "RexDex" Jonarski
*
* [#filter: http #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(http)

//----

/// param list for HTTP post/get requests
class CORE_NET_API RequestArgsGetterProxy : public NoCopy
{
public:
    RequestArgsGetterProxy(const RequestArgs& args, StringID name);
    RequestArgsGetterProxy(const RequestArgsGetterProxy& other) = delete;
    RequestArgsGetterProxy(RequestArgsGetterProxy&& other) = delete;
    RequestArgsGetterProxy& operator=(const RequestArgsGetterProxy& other) = delete;
    RequestArgsGetterProxy& operator=(RequestArgsGetterProxy&& other) = delete;

    template< typename T >
    INLINE T get(const T& defaultValue = T()) const
    {
        T ret = defaultValue;
        ParseFromString(view(), ret);
        return ret;
    }

    void print(IFormatStream& f) const;

    StringView view() const;

protected:
    const RequestArgs& m_readArgs;
    StringID m_name;
};

//----

/// param list for HTTP post/get requests
class CORE_NET_API RequestArgsSetterProxy : public RequestArgsGetterProxy
{
public:
    RequestArgsSetterProxy(RequestArgs& args, StringID name);
    RequestArgsSetterProxy(const RequestArgsSetterProxy& other) = delete;
    RequestArgsSetterProxy(RequestArgsSetterProxy&& other) = delete;
    RequestArgsSetterProxy& operator=(const RequestArgsSetterProxy& other) = delete;
    RequestArgsSetterProxy& operator=(RequestArgsSetterProxy&& other) = delete;

    template< typename T >
    INLINE RequestArgsSetterProxy& operator=(const T& val)
    {
        TempString str;
        PrintToString(str, val);
        set(str.c_str());
        return *this;
    }

    template<>
    INLINE RequestArgsSetterProxy& operator=(const StringView& val)
    {
        set(val);
        return *this;
    }

    template<>
    INLINE RequestArgsSetterProxy& operator=(const StringBuf& val)
    {
        set(val.view());
        return *this;
    }

private:
    void set(StringView val);

    RequestArgs& m_writeArgs;
};

//----

/// param list for HTTP post/get requests
class CORE_NET_API RequestArgs
{
public:
    RequestArgs(uint32_t preallocateStorageSize=0);
    RequestArgs(const RequestArgs& other);
    RequestArgs(RequestArgs&& other);
    RequestArgs& operator=(const RequestArgs& other);
    RequestArgs& operator=(RequestArgs&& other);

    //--

    struct CORE_NET_API Param
    {
        StringID name;
        StringView value;
    };

    // request parameters
    Array<Param> params;

    //--

    // print to stream, outputs classic ?param=val&param=val string
    void print(IFormatStream& f) const;

    //--

    // remove all parameters
    void clear();

    // remove all values for specific param
    void remove(StringID key);

    // remove last value matching given key
    void removeLast(StringID key);

    // add param to the request, supports adding multiple parameters
    template< typename T >
    INLINE RequestArgs& add(StringID key, const T& val)
    {
        TempString str;
        PrintToString(str, val);
        addRaw(key, str);
        return *this;
    }

    // add param to the request, supports adding multiple parameters
    template<>
    INLINE RequestArgs& add(StringID key, const StringView& val)
    {
        addRaw(key, val);
        return *this;
    }

    // add param to the request, supports adding multiple parameters
    template<>
    INLINE RequestArgs& add(StringID key, const StringBuf& val)
    {
        addRaw(key, val);
        return *this;
    }

    // add param to the request, supports adding multiple parameters
    template<>
    INLINE RequestArgs& add(StringID key, const StringID& val)
    {
        addRaw(key, val.view());
        return *this;
    }

    // add param to the request, supports adding multiple parameters
    void addRaw(StringID key, StringView val);

    //--

    // do we have a parameter defined
    bool has(StringID key) const;

    // get value, since empty strings are NOT sent this also returns empty string when value is missing
    StringView raw(StringID key) const;

    template< typename T >
    INLINE T get(StringID key, const T& defaultValue = T()) const
    {
        T ret = defaultValue;
        ParseFromString(raw(key), ret);
        return ret;
    }

    //--

    // read param
    RequestArgsGetterProxy operator[](StringID name) const;

    // write param
    RequestArgsSetterProxy operator[](StringID name);

    //--

    // parse from URL string, decodes URL encoding
    // NOTE: we can have the initial & or not
    static bool Parse(StringView txt, RequestArgs& outArgs);

private:
    Array<char> storage;

    void checkStorage(uint32_t minimumAdditionalSize);
    void rebuildParamList(const Array<char>& oldStorage, Array<char>& newStorage);
};

//----

END_BOOMER_NAMESPACE_EX(http)
