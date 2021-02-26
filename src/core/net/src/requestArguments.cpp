/***
* Boomer Engine v4
* Written by Tomasz "RexDex" Jonarski
*
* [#filter: http #]
***/

#include "build.h"
#include "requestArguments.h"
#include <locale>

BEGIN_BOOMER_NAMESPACE_EX(http)

//---

RequestArgsGetterProxy::RequestArgsGetterProxy(const RequestArgs& args, StringID name)
    : m_readArgs(args)
    , m_name(name)
{}

void RequestArgsGetterProxy::print(IFormatStream& f) const
{
    f << view();
}

StringView RequestArgsGetterProxy::view() const
{
    return m_readArgs.raw(m_name);
}

//---

RequestArgsSetterProxy::RequestArgsSetterProxy(RequestArgs& args, StringID name)
    : RequestArgsGetterProxy(args, name)
    , m_writeArgs(args)
{}

void RequestArgsSetterProxy::set(StringView val)
{
    m_writeArgs.addRaw(m_name, val);
}

//---

RequestArgs::RequestArgs(uint32_t preallocateStorageSize/*0*/)
{
    if (preallocateStorageSize > 0)
    {
        storage.reserve(preallocateStorageSize);
        params.reserve(16);
    }
}

RequestArgs::RequestArgs(const RequestArgs& other)
    : storage(other.storage)
    , params(other.params)
{
    rebuildParamList(other.storage, storage);
}
            
RequestArgs::RequestArgs(RequestArgs&& other)
    : storage(std::move(other.storage))
    , params(std::move(other.params))
{}

RequestArgs& RequestArgs::operator=(const RequestArgs& other)
{
    if (this != &other)
    {
        storage = other.storage;
        params = other.params;
        rebuildParamList(other.storage, storage);
    }
    return *this;
}

RequestArgs& RequestArgs::operator=(RequestArgs&& other)
{
    if (this != &other)
    {
        storage = std::move(other.storage);
        params = std::move(other.params);
    }
    return *this;
}

void RequestArgs::checkStorage(uint32_t minimumAdditionalSize)
{
    if (storage.size() + minimumAdditionalSize < storage.capacity())
    { 
        Array<char> newStorage;

        auto autoGrowSize = std::max<uint32_t>(64, storage.size() * 2);
        auto bestSize = Align<uint32_t>(autoGrowSize + minimumAdditionalSize, 64); // allocate full cache lines (hopefully)
        newStorage.reserve(bestSize);
        newStorage.prepare(storage.size());
        memcpy(newStorage.data(), storage.data(), storage.dataSize());

        rebuildParamList(storage, newStorage);

        storage = std::move(newStorage);
    }
}

void RequestArgs::rebuildParamList(const Array<char>& oldStorage, Array<char>& newStorage)
{
    ASSERT(newStorage.size() >= oldStorage.size());

    for (uint32_t i = 0; i < params.size(); ++i)
    {
        auto length = params[i].value.length();
        std::ptrdiff_t startOffset = params[i].value.data() - oldStorage.typedData();
        ASSERT((startOffset >= 0) && (startOffset + length <= oldStorage.size()));
        params[i].value = StringView(newStorage.typedData() + startOffset, length);
    }
}

void RequestArgs::addRaw(StringID key, StringView val)
{
    if (key && val)
    {
        checkStorage(val.length());

        auto ptr  = storage.allocateUninitialized(val.length() + 1);
        memcpy(ptr, val.data(), val.length());
        ptr[val.length()] = 0;

        auto& info = params.emplaceBack();
        info.name = key;
        info.value = StringView(ptr, val.length());
    }
}

//--

void RequestArgs::print(IFormatStream& f) const
{
    bool first = true;
    for (auto& param : params)
    {
        if (param.name && param.value)
        {
            f.append(first ? "?" : "&");
            first = false;
            f << param.name;
            f << "=";
            f.appendUrlEscaped(param.value.data(), param.value.length());
        }
    }
}

bool RequestArgs::has(StringID key) const
{
    for (auto& param : params)
        if (param.name == key)
            return true;

    return false;
}

StringView RequestArgs::raw(StringID key) const
{
    for (auto& param : params)
        if (param.name == key)
            return param.value;
    return StringView();
}

//--

void RequestArgs::clear()
{
    params.reset();
    storage.clear();
}

void RequestArgs::removeLast(StringID key)
{
    for (auto i = params.lastValidIndex(); i >= 0; --i)
    {
        if (params[i].name == key)
        {
            params.erase(i);
            break;
        }
    }
}

void RequestArgs::remove(StringID key)
{
    for (auto i = params.lastValidIndex(); i >= 0; --i)
        if (params[i].name == key)
            params.erase(i);
}

//--

RequestArgsGetterProxy RequestArgs::operator[](StringID name) const
{
    return RequestArgsGetterProxy(*this, name);
}

RequestArgsSetterProxy RequestArgs::operator[](StringID name)
{
    return RequestArgsSetterProxy(*this, name);
}

//---

static INLINE bool IsAlphaNumeric(char ch)
{
    return
        ((ch >= '0') && (ch <= '9')) ||
        ((ch >= 'a') && (ch <= 'z')) ||
        ((ch >= 'A') && (ch <= 'Z'));
}

static INLINE uint8_t GetHexValue(char ch)
{
    switch (ch)
    {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'A': return 10;
    case 'B': return 11;
    case 'C': return 12;
    case 'D': return 13;
    case 'E': return 14;
    case 'F': return 15;
    case 'a': return 10;
    case 'b': return 11;
    case 'c': return 12;
    case 'd': return 13;
    case 'e': return 14;
    case 'f': return 15;
    }
    return 0;
}

static INLINE uint8_t DecodeURLChar(const char* ch)
{
    return (GetHexValue(ch[0]) << 4) | GetHexValue(ch[1]);
}

bool RequestArgs::Parse(StringView txt, RequestArgs& outArgs)
{
    StringBuilder value;
    RequestArgs ret(txt.length() + 60);

    auto str  = txt.data();
    auto end  = str + txt.length();

    // skip the ? if provided
    if (str < end && *str == '?')
        str++;

    while (str < end)
    {
        // read the ident name
        auto identStart  = str;
        while (str < end)
        {
            if (*str == '=')
                break;
            if (!IsAlphaNumeric(*str))
                return false;
            str++;
        }

        if (str == identStart || str == end)
            return false;

        // we have the ident
        auto ident = StringView(identStart, str);
        str += 1;

        // now parse the value
        value.clear();
        while (str < end)
        {
            if (*str == '&')
                break;
            if (*str == '%')
            {
                str += 1;

                if (str + 2 > end)
                    return false;

                auto code = DecodeURLChar(str);
                str += 2;

                value.appendch((char)code);
            }
            else if (IsAlphaNumeric(*str) || *str == '.')
            {
                value.appendch(*str);
                str += 1;
            }
            else
            {
                return false;
            }
        }

        ret.addRaw(StringID(ident), value.view());

        // skip the separator
        if (str < end)
        {
            if (*str != '&')
                return false;
            str += 1;
        }
    }

    outArgs = std::move(ret);
    return true;
}

//---

END_BOOMER_NAMESPACE_EX(http)
