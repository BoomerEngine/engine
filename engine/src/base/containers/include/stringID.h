/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: string #]
***/

#pragma once

#include "stringBuf.h"

namespace base
{
	namespace prv
	{
		class StringIDDataStorage;
		class StringIDMap;
	}

    /// String class
    class BASE_CONTAINERS_API StringID
    {
    public:
        INLINE StringID();
        INLINE StringID(const StringID& other);
        INLINE StringID(StringView other);
        INLINE ~StringID() = default;

        INLINE explicit StringID(const char* other);

        template< uint32_t N >
        INLINE explicit StringID(const BaseTempString<N>& other);

        INLINE bool operator==(StringID other) const;
        INLINE bool operator!=(StringID other) const;

        INLINE bool operator==(const char* other) const;
        INLINE bool operator==(const StringBuf& other) const;
        INLINE bool operator!=(const char* other) const;
        INLINE bool operator!=(const StringBuf& other) const;

        INLINE bool operator<(StringID other) const; // compares STRINGS, not numerical IDs as this is more stable

        INLINE StringID& operator=(StringID other);

        //---

        //! true if the ID is empty (== EMPTY()
        INLINE bool empty() const;

        //! get the ordering value for this name, can be used if we want to map stuff directly
        INLINE uint32_t index() const;

        //! get the printable C string
        INLINE const char* c_str() const;

        //! get the view of the string
        INLINE StringView view() const;

        //---

        //! check if not empty
        INLINE operator bool() const;

        //---

        //! get empty name
        static StringID EMPTY();

        //! find name without allocating string
        static StringID Find(StringView txt);

        //---

        INLINE static uint32_t CalcHash(StringID id);
        INLINE static uint32_t CalcHash(StringView txt);
        INLINE static uint32_t CalcHash(const char* txt);

    private:
        StringIDIndex indexValue;

		//--

		static StringIDIndex Alloc(StringView txt);

		//--

		static std::atomic<const char*> st_StringTable; // global string table
		friend class prv::StringIDDataStorage;
		friend class prv::StringIDMap;

		//--

		explicit ALWAYS_INLINE StringID(StringIDIndex index)
			: indexValue(index)
		{}

		//--

    };

} // base

INLINE const base::StringID operator"" _id(const char* str, size_t len)
{
    return base::StringID(str);
}