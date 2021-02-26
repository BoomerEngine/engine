/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: string #]
***/

#include "build.h"
#include "inplaceArray.h"
#include "utf8StringFunctions.h"

BEGIN_BOOMER_NAMESPACE()

namespace prv
{

    //---

    StringDataHolder* StringDataHolder::CreateAnsi(const char* txt, uint32_t length/* = INDEX_MAX*/)
    {
        if (!txt || !*txt)
            return nullptr;

        if (length == INDEX_MAX)
            length = strlen(txt);

        auto data = mem::GlobalPool<POOL_STRINGS, StringDataHolder>::Alloc(sizeof(StringDataHolder) + length, 1);
        data->m_refs = 1;
        data->m_length = length;
        memcpy(data->m_txt, txt, length);
        data->m_txt[length] = 0;
        return data;
    }

    StringDataHolder* StringDataHolder::CreateUnicode(const wchar_t* txt, uint32_t uniLength /*= INDEX_MAX*/)
    {
        if (!txt || !*txt)
            return nullptr;

        if (uniLength == INDEX_MAX)
            uniLength = wcslen(txt);

        auto length  = utf8::CalcSizeRequired(txt, uniLength);

        auto data = mem::GlobalPool<POOL_STRINGS, StringDataHolder>::Alloc(sizeof(StringDataHolder) + length, 1);
        data->m_refs = 1;
        data->m_length = length;
        utf8::FromUniChar(data->m_txt, length + 1, txt, uniLength);
        data->m_txt[length] = 0;
        return data;
    }

    StringDataHolder* StringDataHolder::CreateEmpty(uint32_t length)
    {
        if (!length)
            return nullptr;

        auto data = mem::GlobalPool<POOL_STRINGS, StringDataHolder>::Alloc(sizeof(StringDataHolder) + length, 1);
        data->m_refs = 1;
        data->m_length = length;
        memset(data->m_txt, 0, length + 1);
        return data;
    }

    StringDataHolder* StringDataHolder::copy() const
    {
        auto ret  = StringDataHolder::CreateEmpty(m_length);
        memcpy(ret->m_txt, m_txt, m_length + 1);
        return ret;
    }

    void StringDataHolder::ReleaseToPool(void* mem, uint32_t length)
    {
        mem::GlobalPool<POOL_STRINGS, StringDataHolder>::Free(mem);
    }

    //---

} // prv

//--

StringBuf EmptyString;

const StringBuf& StringBuf::EMPTY()
{
    return EmptyString;
}

//--

StringBuf::StringBuf(const void* rawData, uint32_t rawDataSize)
    : m_data(nullptr)
{
    if (rawData && rawDataSize)
    {
        auto uniData  = (const wchar_t*)rawData;
        if (rawDataSize >= 2 && (*uniData == 0xFFFE || *uniData == 0xFFFE))
        {
            auto stringLength  = (rawDataSize - 2) / 2;
            m_data = prv::StringDataHolder::CreateUnicode(uniData + 1, stringLength);
        }
        else
        {
            m_data = prv::StringDataHolder::CreateAnsi((const char*)rawData, rawDataSize);
        }
    }       
}

StringBuf::StringBuf(const Buffer& buffer)
    : m_data(nullptr)
{
    if (buffer)
    {
        auto rawDataSize  = buffer.size();
        auto uniData  = (const wchar_t*)buffer.data();
        if (rawDataSize >= 2 && (*uniData == 0xFFFE || *uniData == 0xFFFE))
        {
            auto stringLength  = (rawDataSize - 2) / 2;
            m_data = prv::StringDataHolder::CreateUnicode(uniData + 1, stringLength);
        }
        else
        {
            m_data = prv::StringDataHolder::CreateAnsi((const char*)buffer.data(), rawDataSize);
        }
    }
}

StringBuf& StringBuf::operator=(const StringBuf& other)
{
    if (this != &other)
    {
        clear();
        m_data = other.m_data;
        if (m_data)
            m_data->addRef();
    }

    return *this;
}

StringBuf& StringBuf::operator=(StringBuf&& other)
{
    if (this != &other)
    {
        clear();
        m_data = other.m_data;
        other.m_data = nullptr;
    }
    return *this;
}

bool StringBuf::operator==(StringView other) const
{
    return 0 == view().cmp(other);
}

bool StringBuf::operator!=(StringView other) const
{
    return 0 != view().cmp(other);
}

bool StringBuf::operator<(StringView other) const
{
    return 0 > view().cmp(other);
}

int StringBuf::compareWith(StringView other) const
{
    return view().cmp(other);
}

int StringBuf::compareWithNoCase(StringView other) const
{
    return view().caseCmp(other);
}

StringBuf StringBuf::leftPart(uint32_t count) const
{
    return StringBuf(view().leftPart(count));
}

StringBuf StringBuf::rightPart(uint32_t count) const
{
    return StringBuf(view().rightPart(count));
}

StringBuf StringBuf::subString(uint32_t first, uint32_t count /*=(uint32_t)-1*/) const
{
    return StringBuf(view().subString(first, count));
}

void StringBuf::split(uint32_t index, StringBuf& outLeft, StringBuf& outRight) const
{
    outLeft = leftPart(index);
    outRight = subString(index);
}

bool StringBuf::splitAt(StringView str, StringBuf& outLeft, StringBuf& outRight) const
{
    int pos = findStr(str);
    if (pos != INDEX_NONE)
    {
        outLeft = leftPart(pos);
        outRight = subString(pos + str.length());
        return true;
    }

    return false;
}

int StringBuf::findStr(StringView pattern, int firstPosition) const
{
    return (int)view().findStr(pattern, firstPosition);
}

int StringBuf::findStrRev(StringView pattern, int firstPosition) const
{
    return (int)view().findRevStr(pattern, firstPosition);
}

int StringBuf::findStrNoCase(StringView pattern, int firstPosition) const
{
    return (int)view().findStrNoCase(pattern, firstPosition);
}

int StringBuf::findStrRevNoCase(StringView pattern, int firstPosition) const
{
    return (int)view().findRevStrNoCase(pattern, firstPosition);
}

int StringBuf::findFirstChar(char ch) const
{
    return (int)view().findFirstChar(ch);
}

int StringBuf::findLastChar(char ch) const
{
    return (int)view().findLastChar(ch);
}

void StringBuf::replaceChar(char ch, char newChar)
{
    bool copyCreated = false;

    auto len  = length();
    for (uint32_t i = 0; i < len; ++i)
    {
        char strCh = c_str()[i];
        if (strCh == ch)
        {
            // create copy of the buffer
            if (!copyCreated)
            {
                copyCreated = true;
                m_data = m_data->copy();
            }

            // replace
            ((char*)c_str())[i] = newChar;
        }
    }
}

bool StringBuf::beginsWith(StringView pattern) const
{
    return view().beginsWith(pattern);
}

bool StringBuf::beginsWithNoCase(StringView pattern) const
{
    return view().beginsWithNoCase(pattern);
}

bool StringBuf::endsWith(StringView pattern) const
{
    return view().endsWith(pattern);
}

bool StringBuf::endsWithNoCase(StringView pattern) const
{
    return view().endsWithNoCase(pattern);
}

//--

StringBuf StringBuf::stringAfterFirst(StringView pattern, bool returnFullStringIfNotFound/*=false*/) const
{
    if (returnFullStringIfNotFound)
        return StringBuf(view().afterFirstOrFull(pattern));
    else
        return StringBuf(view().afterFirst(pattern));
}

StringBuf StringBuf::stringBeforeFirst(StringView pattern, bool returnFullStringIfNotFound/*=false*/) const
{
    if (returnFullStringIfNotFound)
        return StringBuf(view().beforeFirstOrFull(pattern));
    else
        return StringBuf(view().beforeFirst(pattern));
}


StringBuf StringBuf::stringAfterLast(StringView pattern, bool returnFullStringIfNotFound/*=false*/) const
{
    if (returnFullStringIfNotFound)
        return StringBuf(view().afterLastOrFull(pattern));
    else
        return StringBuf(view().afterLast(pattern));
}

StringBuf StringBuf::stringBeforeLast(StringView pattern, bool returnFullStringIfNotFound/*=false*/) const
{
    if (returnFullStringIfNotFound)
        return StringBuf(view().beforeLastOrFull(pattern));
    else
        return StringBuf(view().beforeLast(pattern));
}
    
//--

StringBuf StringBuf::stringAfterFirstNoCase(StringView pattern, bool returnFullStringIfNotFound/*=false*/) const
{
    if (returnFullStringIfNotFound)
        return StringBuf(view().afterFirstNoCaseOrFull(pattern));
    else
        return StringBuf(view().afterFirstNoCase(pattern));
}

StringBuf StringBuf::stringBeforeFirstNoCase(StringView pattern, bool returnFullStringIfNotFound/*=false*/) const
{
    if (returnFullStringIfNotFound)
        return StringBuf(view().beforeFirstNoCaseOrFull(pattern));
    else
        return StringBuf(view().beforeFirstNoCase(pattern));
}


StringBuf StringBuf::stringAfterLastNoCase(StringView pattern, bool returnFullStringIfNotFound/*=false*/) const
{
    if (returnFullStringIfNotFound)
        return StringBuf(view().afterLastNoCaseOrFull(pattern));
    else
        return StringBuf(view().afterLastNoCase(pattern));
}

StringBuf StringBuf::stringBeforeLastNoCase(StringView pattern, bool returnFullStringIfNotFound/*=false*/) const
{
    if (returnFullStringIfNotFound)
        return StringBuf(view().beforeLastNoCaseOrFull(pattern));
    else
        return StringBuf(view().beforeLastNoCase(pattern));
}

//--

StringBuf StringBuf::toLower() const
{
    StringBuf copy(*this);
    uint32_t length = copy.length();
    char* buf = const_cast<char*>(copy.c_str());
    for (uint32_t i = 0; i < length; ++i)
    {
        if (buf[i] >= 'A' && buf[i] <= 'Z')
            buf[i] += 'a' - 'A';
    }

    return copy;
}

StringBuf StringBuf::toUpper() const
{
    StringBuf copy(*this);
    uint32_t length = copy.length();
    char* buf = const_cast<char*>(copy.c_str());
    for (uint32_t i = 0; i < length; ++i)
    {
        if (buf[i] >= 'a' && buf[i] <= 'z')
            buf[i] += 'A' - 'a';
    }

    return copy;
}

void StringBuf::slice(const char* splitChars, bool keepEmpty, Array< StringBuf >& outTokens) const
{
    InplaceArray<StringView, 16> tokenViews;
    view().slice(splitChars, keepEmpty, tokenViews);

    outTokens.reserve(tokenViews.size());
    for (auto &view : tokenViews)
        outTokens.emplaceBack(view);
}

//---

END_BOOMER_NAMESPACE()
