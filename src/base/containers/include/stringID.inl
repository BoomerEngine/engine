/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: string #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

///----

INLINE StringID::StringID()
    : indexValue(0)
{}

INLINE StringID::StringID(const StringID& other)
    : indexValue(other.indexValue)
{}

INLINE StringID::StringID(const char* other)
{
	indexValue = Alloc(other);
}

INLINE StringID::StringID(StringView other)
{
	indexValue = Alloc(other);
}

template< uint32_t N >
INLINE StringID::StringID(const BaseTempString<N>& other)
{
	indexValue = Alloc(other.c_str());
}

INLINE bool StringID::operator==(StringID other) const
{
    return indexValue == other.indexValue;
}

INLINE bool StringID::operator!=(StringID other) const
{
    return indexValue != other.indexValue;
}

INLINE bool StringID::operator==(const char* other) const
{
    return view() == other;
}

INLINE bool StringID::operator==(const StringBuf& other) const
{
    return view() == other;
}

INLINE bool StringID::operator!=(const char* other) const
{
    return view() != other;
}

INLINE bool StringID::operator!=(const StringBuf& other) const
{
    return view() != other;
}

INLINE bool StringID::operator<(StringID other) const
{
    return view() < other.view();
}

INLINE StringID& StringID::operator=(StringID other)
{
    indexValue = other.indexValue;
    return *this;
}

INLINE bool StringID::empty() const
{
    return 0 == indexValue;
}

INLINE StringID::operator bool() const
{
    return 0 != indexValue;
}

INLINE uint32_t StringID::CalcHash(StringID id)
{
    return StringView::CalcHash(id.view());
}

INLINE uint32_t StringID::CalcHash(StringView txt)
{
    return StringView::CalcHash(txt);
}

INLINE uint32_t StringID::CalcHash(const char* txt)
{
    return StringView::CalcHash(txt);
}
        
INLINE const char* StringID::c_str() const
{
    return st_StringTable.load() + indexValue;
}

INLINE StringView StringID::view() const
{
	return StringView(st_StringTable.load() + indexValue);
}

INLINE uint32_t StringID::index() const
{
    return indexValue;
}

END_BOOMER_NAMESPACE(base)
