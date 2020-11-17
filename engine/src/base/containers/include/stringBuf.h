/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: string #]
***/

#pragma once

#include "stringVector.h"
#include "array.h"

namespace base
{
    
    //--

    namespace prv
    {

        // holder for string data (all strings are interned)
        // for "STL" style strings, use StringVector
        class BASE_CONTAINERS_API StringDataHolder : public NoCopy
        {
        public:
            // add internal reference
            INLINE void addRef() { ++m_refs; }

            /// release a reference
            INLINE void release() { if (0 == --m_refs) ReleaseToPool(this, m_length); }

            // get the zero-terminated C style string representation of the data stored in the storage buffer
            INLINE const char* c_str() const { return m_txt; }

            // get length of the current data
            INLINE uint32_t length() const { return m_length; }

            //--

            // create from ansi string
            static StringDataHolder* CreateAnsi(const char* txt, uint32_t length = INDEX_MAX);

            // create from string
            static StringDataHolder* CreateUnicode(const wchar_t* txt, uint32_t length = INDEX_MAX);

            // create empty
            static StringDataHolder* CreateEmpty(uint32_t length); // empty buffer with preallocated length

            //--

            // make a un-shared copy
            StringDataHolder* copy() const;

        private:
            std::atomic<uint32_t> m_refs;
            uint32_t m_length;
            char m_txt[1];

            static void ReleaseToPool(void* mem, uint32_t length);
        };

    } // prv

    //--

    /// general string buffer
    class BASE_CONTAINERS_API StringBuf
    {
    public:
        INLINE StringBuf()
            : m_data(nullptr)
        {};

        INLINE StringBuf(StringBuf&& other)
            : m_data(other.m_data)
        {
            other.m_data = nullptr;
        }

        // TODO: make explicit!
        INLINE StringBuf(const char* str, uint32_t length = INDEX_MAX)
        {
            m_data = prv::StringDataHolder::CreateAnsi(str, length);
        }

        INLINE explicit StringBuf(const wchar_t* str, uint32_t length = INDEX_MAX)
        {
            m_data = prv::StringDataHolder::CreateUnicode(str, length);
        }

        INLINE explicit StringBuf(BaseStringView<char> view)
        {
            m_data = prv::StringDataHolder::CreateAnsi(view.data(), view.length());
        }

        INLINE explicit StringBuf(BaseStringView<wchar_t> view)
        {
            m_data = prv::StringDataHolder::CreateUnicode(view.data(), view.length());
        }

        INLINE explicit StringBuf(uint32_t length)
        {
            m_data = prv::StringDataHolder::CreateEmpty(length);
        }

        INLINE ~StringBuf()
        {
            if (m_data)
                m_data->release();
        }

        template< uint32_t N >
        INLINE StringBuf(const BaseTempString<N>& str)
        {
            m_data = prv::StringDataHolder::CreateAnsi(str.c_str(), str.length());
        }

        template< uint32_t N >
        INLINE StringBuf& operator=(const BaseTempString<N>& str)
        {
            *this = StringBuf(str.c_str());
            return *this;
        }

        INLINE StringBuf(const StringBuf& other)
            : m_data(other.m_data)
        {
            if (m_data)
                m_data->addRef();
        }

        StringBuf(const Buffer& buffer);
        StringBuf(const void* rawData, uint32_t rawDataSize);

        StringBuf& operator=(const StringBuf& other);
        StringBuf& operator=(StringBuf&& other);

        bool operator==(StringView other) const;
        bool operator!=(StringView other) const;
        bool operator<(StringView other) const;

        //---

        int compareWith(StringView other) const;
        int compareWithNoCase(StringView other) const;

        //---

        StringBuf leftPart(uint32_t count) const;
        StringBuf rightPart(uint32_t count) const;
        StringBuf subString(uint32_t first, uint32_t count = INDEX_MAX) const;

        //---

        void split(uint32_t index, StringBuf& outLeft, StringBuf& outRight) const;
        bool splitAt(StringView str, StringBuf& outLeft, StringBuf& outRight) const;

        void slice(const char* splitChars, bool keepEmpty, Array< StringBuf >& outTokens) const;

        //---

        int findStr(StringView pattern, int firstPosition = 0) const;
        int findStrRev(StringView pattern, int firstPosition = std::numeric_limits<int>::max()) const;
        int findStrNoCase(StringView pattern, int firstPosition = 0) const;
        int findStrRevNoCase(StringView pattern, int firstPosition = std::numeric_limits<int>::max()) const;

        int findFirstChar(char ch) const;
        int findLastChar(char ch) const;

        //---

        void replaceChar(char ch, char newChar);

        //---

        bool beginsWith(StringView pattern) const;
        bool beginsWithNoCase(StringView pattern) const;
        bool endsWith(StringView pattern) const;
        bool endsWithNoCase(StringView pattern) const;

        //---

        StringBuf stringAfterFirst(StringView pattern, bool returnFullStringIfNotFound = false) const;
        StringBuf stringBeforeFirst(StringView pattern, bool returnFullStringIfNotFound = false) const;
        StringBuf stringAfterLast(StringView pattern, bool returnFullStringIfNotFound=false) const;
        StringBuf stringBeforeLast(StringView pattern, bool returnFullStringIfNotFound = false) const;

        //---

        StringBuf stringAfterFirstNoCase(StringView pattern, bool returnFullStringIfNotFound = false) const;
        StringBuf stringBeforeFirstNoCase(StringView pattern, bool returnFullStringIfNotFound = false) const;
        StringBuf stringAfterLastNoCase(StringView pattern, bool returnFullStringIfNotFound = false) const;
        StringBuf stringBeforeLastNoCase(StringView pattern, bool returnFullStringIfNotFound = false) const;

        //---

        StringBuf toLower() const;
        StringBuf toUpper() const;

        //---

        INLINE void clear();

        INLINE bool empty() const;

        INLINE uint32_t length() const;

        INLINE const char* c_str() const;

        INLINE UTF16StringVector uni_str() const;

        INLINE StringView view() const;

        INLINE operator StringView() const;

        INLINE uint32_t cRC32() const;

        INLINE uint64_t cRC64() const;

        INLINE explicit operator bool() const;

        //---

        INLINE static uint32_t CalcHash(const StringBuf& txt);
        INLINE static uint32_t CalcHash(StringView txt);
        INLINE static uint32_t CalcHash(const char* txt);

        //---

        // Empty string buffer
        static const StringBuf& EMPTY();
        
        //---

    private:
        prv::StringDataHolder* m_data = nullptr;
    };

    //--

    /// entry for the text template replacement
    struct ReplaceTextPattern
    {
        StringView name;
        StringView data;
    };

    /// print template into the output replacing ${X} markers with proper content
    extern BASE_CONTAINERS_API void ReplaceText(IFormatStream& f, StringView templateText, const ReplaceTextPattern* patterns, uint32_t numPatterns);

    //--

} // base
