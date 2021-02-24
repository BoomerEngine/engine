/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

//--

/// GUID
struct BASE_SYSTEM_API GUID
{
public:
    static const uint32_t NUM_WORDS = 4;

    INLINE GUID(); // empty, zero guid
    INLINE GUID(const GUID& other);
    INLINE GUID(GUID&& other); // clears the source
    INLINE GUID& operator=(const GUID& other);
    INLINE GUID& operator=(GUID&& other); // clears the source

    INLINE const uint32_t* data() const;

    INLINE bool empty() const;
    INLINE operator bool() const;

    INLINE bool operator==(const GUID& other) const;
    INLINE bool operator!=(const GUID& other) const;
    INLINE bool operator<(const GUID& other) const;

    //--

    void print(IFormatStream& f) const;

    //--

    static uint32_t CalcHash(const GUID& guid);

    //--

    static GUID Create(); // creates global guid

    //--

private:
    uint32_t m_words[NUM_WORDS];
};

//--
    
END_BOOMER_NAMESPACE(base)

#include "guid.inl"