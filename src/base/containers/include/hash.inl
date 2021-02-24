/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

template< typename T >
struct Hasher
{
    template< typename W >
    INLINE static uint32_t CalcHash(W val) {
        return T::CalcHash(val);
    }
};


template<typename T >
struct Hasher<T*>
{
    INLINE static uint32_t CalcHash(const T* val) {
        return std::hash<const void*>{}(val);
    }
};
/*
template<typename T >
struct Hasher<const T*>
{
    INLINE static uint32_t CalcHash(const T* val) {
        return std::hash<const void*>{}(val);
    }
};*/

template<>
struct Hasher<uint8_t>
{
    INLINE static uint32_t CalcHash(uint8_t val) {
        return std::hash<uint64_t>{}(val);
    }
};

template<>
struct Hasher<uint16_t>
{
    INLINE static uint32_t CalcHash(uint16_t val) {
        return std::hash<uint64_t>{}(val);
    }
};

template<>
struct Hasher<uint32_t>
{
    INLINE static uint32_t CalcHash(uint32_t val) {
        return std::hash<uint64_t>{}(val);
    }
};

template<>
struct Hasher<uint64_t>
{
    INLINE static uint32_t CalcHash(uint64_t val) {
        return std::hash<uint64_t>{}(val);
    }
};

template<>
struct Hasher<int8_t>
{
    INLINE static uint32_t CalcHash(int8_t val) {
        return std::hash<uint64_t>{}(val);
    }
};

template<>
struct Hasher<int16_t>
{
    INLINE static uint32_t CalcHash(int16_t val) {
        return std::hash<uint64_t>{}(val);
    }
};

template<>
struct Hasher<int32_t>
{
    INLINE static uint32_t CalcHash(int32_t val) {
        return std::hash<uint64_t>{}(val);
    }
};

template<>
struct Hasher<int64_t>
{
    INLINE static uint32_t CalcHash(int64_t val) {
        return std::hash<int64_t>{}(val);
    }
};

END_BOOMER_NAMESPACE(base)


