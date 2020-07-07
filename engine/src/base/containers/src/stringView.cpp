/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: string\utf16 #]
***/

#include "build.h"
#include "stringView.h"
#include "inplaceArray.h"

namespace base
{
    namespace prv
    {
        namespace search
        {

            template<typename C>
            struct Identity
            {
                INLINE static C Convert(C c)
                {
                    return c;
                }
            };

            template<typename C>
            struct RemoveCase
            {
                INLINE static C Convert(C c)
                {
                    if (c >= 'A' && c <= 'Z')
                        return 'a' + (c - 'A');
                    return c;
                }
            };

            template<typename C, typename F = Identity<C>>
            static int64_t KMP(const C* TData, uint32_t TLength, const C* PData, uint32_t PLength)
            {
                if (PLength == 0)
                    return 0;

                InplaceArray<int, 32> pi;
                pi.allocateWith(PLength, 0);
                for (int i=1, k=0; i<(int)PLength; ++i)
                {
                    while (k && F::Convert(PData[k]) != F::Convert(PData[i]))
                        k = pi[k - 1];

                    if (F::Convert(PData[k]) == F::Convert(PData[i]))
                        ++k;

                    pi[i] = k;
                }

                for (int i=0, k=0; i < (int)TLength; ++i)
                {
                    while (k && F::Convert(PData[k]) != F::Convert(TData[i]))
                        k = pi[k - 1];

                    if (F::Convert(PData[k]) == F::Convert(TData[i]))
                        ++k;

                    if (k == (int)PLength)
                        return i - k + 1;
                }

                return -1;
            }

            template<typename C, typename F = Identity<C>>
            static bool Test(const C* TData, const C* PData, int64_t PLength)
            {
                for (int64_t i=0; i<PLength; ++i)
                {
                    if (F::Convert(TData[i]) != F::Convert(PData[i]))
                        return false;
                }
                return true;
            }

            template<typename C, typename F = Identity<C>>
            static int64_t Linear(const C* TData, int64_t TLength, const C* PData, int64_t PLength)
            {
                if (PLength == 0)
                    return 0;

                int64_t maxTest = TLength - PLength;
                for (int64_t i=0; i<=maxTest; ++i)
                    if (Test<C, F>(TData + i, PData, PLength))
                        return i;

                return -1;
            }

            template<typename C, typename F = Identity<C>>
            static int64_t LinearRev(const C* TData, int64_t TLength, const C* PData, int64_t PLength)
            {
                if (PLength == 0)
                    return 0;

                int64_t maxTest = TLength - PLength;
                for (int64_t i=maxTest; i>=0; --i)
                    if (Test<C, F>(TData + i, PData, PLength))
                        return i;

                return -1;
            }

        } // search

        //--

        template< typename T>
        static bool IsTokenEmtpy(const T* start, const T* pos)
        {
            while (start < pos)
            {
                if (*start > ' ')
                {
                    return false;
                }
                ++start;
            }

            return true;
        }

        template< typename T>
        static bool Contains(const char* ptr, T test)
        {
            while (*ptr)
            {
                if (*ptr++ == test)
                    return true;
            }

            return false;
        }


        template< typename T>
        static void DoSlice(const T* data, uint64_t length, const char* splitChars, bool keepEmpty, Array< StringView<T> >& outTokens)
        {
            const T* str = data;
            const T* end = data + length;
            const T* start = str;
            while (str < end)
            {
                T ch = *str;

                if (Contains<T>(splitChars, ch))
                {
                    if (keepEmpty || !IsTokenEmtpy<T>(start, str))
                        outTokens.emplaceBack(start, str);
                    start = str + 1;
                }

                str += 1;
            }

            if (start < str)
            {
                if (!IsTokenEmtpy(start, str))
                    outTokens.emplaceBack(start, range_cast<uint32_t>(str - start));
            }
        }

        void BaseHelper::Slice(const char* data, uint64_t length, const char* splitChars, bool keepEmpty, Array< StringView<char> >& outTokens)
        {
            DoSlice<char>(data, length, splitChars, keepEmpty, outTokens);
        }

        void BaseHelper::Slice(const wchar_t* data, uint64_t length, const char* splitChars, bool keepEmpty, Array< StringView<wchar_t> >& outTokens)
        {
            DoSlice<wchar_t>(data, length, splitChars, keepEmpty, outTokens);
        }

        int64_t BaseHelper::Find(const char* haystack, uint64_t length, const char* key, uint64_t keyLength)
        {
            if (keyLength <= 3)
                return search::Linear<char>(haystack, length, key, keyLength);
            else
                return search::KMP<char>(haystack, (uint32_t)length, key, (uint32_t)keyLength);
        }

        int64_t BaseHelper::Find(const wchar_t* haystack, uint64_t length, const wchar_t* key, uint64_t keyLength)
        {
            if (keyLength <= 3)
                return search::Linear<wchar_t>(haystack, length, key, keyLength);
            else
                return search::KMP<wchar_t>(haystack, (uint32_t)length, key, (uint32_t)keyLength);
        }

        int64_t BaseHelper::FindNoCase(const char* haystack, uint64_t length, const char* key, uint64_t keyLength)
        {
            if (keyLength <= 3)
                return search::Linear<char, search::RemoveCase<char>>(haystack, length, key, keyLength);
            else
                return search::KMP<char, search::RemoveCase<char>>(haystack, (uint32_t)length, key, (uint32_t)keyLength);
        }

        int64_t BaseHelper::FindNoCase(const wchar_t* haystack, uint64_t length, const wchar_t* key, uint64_t keyLength)
        {
            if (keyLength <= 3)
                return search::Linear<wchar_t, search::RemoveCase<wchar_t>>(haystack, length, key, keyLength);
            else
                return search::KMP<wchar_t, search::RemoveCase<wchar_t>>(haystack, (uint32_t)length, key, (uint32_t)keyLength);
        }

        int64_t BaseHelper::FindRev(const char* haystack, uint64_t length, const char* key, uint64_t keyLength)
        {
            return search::LinearRev<char>(haystack, length, key, keyLength);
        }

        int64_t BaseHelper::FindRev(const wchar_t* haystack, uint64_t length, const wchar_t* key, uint64_t keyLength)
        {
            return search::LinearRev<wchar_t>(haystack, length, key, keyLength);
        }

        int64_t BaseHelper::FindRevNoCase(const char* haystack, uint64_t length, const char* key, uint64_t keyLength)
        {
            return search::LinearRev<char, search::RemoveCase<char>>(haystack, length, key, keyLength);
        }

        int64_t BaseHelper::FindRevNoCase(const wchar_t* haystack, uint64_t length, const wchar_t* key, uint64_t keyLength)
        {
            return search::LinearRev<wchar_t, search::RemoveCase<wchar_t>>(haystack, length, key, keyLength);
        }

        uint64_t BaseHelper::StringHash(const char* str, const char* end)
        {
            uint64_t hval = UINT64_C(0xcbf29ce484222325);
            while (str < end)
            {
                hval ^= (uint64_t)*str++;
                hval *= UINT64_C(0x100000001b3);
            }
            return hval;
        }

        uint64_t BaseHelper::StringHashNoCase(const char* str, const char* end)
        {
            uint64_t hval = UINT64_C(0xcbf29ce484222325);
            while (str < end)
            {
                auto ch = *str++;

                if (ch >= 'A' && ch <= 'Z')
                    ch = 'a' + (ch - 'A');

                hval ^= (uint64_t)ch;
                hval *= UINT64_C(0x100000001b3);
            }
            return hval;
        }

        uint32_t BaseHelper::StringCRC32(const char* str, const char* end, uint32_t crc)
        {
            CRC32 calc(crc);
            calc.append(str, end-str);
            return calc.crc();
        }

        uint64_t BaseHelper::StringCRC64(const char* str, const char* end, uint64_t crc)
        {
            CRC64 calc(crc);
            calc.append(str, end-str);
            return calc.crc();
        }

        uint64_t BaseHelper::StringHash(const wchar_t* str, const wchar_t* end)
        {
            uint64_t hval = UINT64_C(0xcbf29ce484222325);
            while (str < end)
            {
                hval ^= (uint64_t)*str++;
                hval *= UINT64_C(0x100000001b3);
            }
            return hval;
        }

        uint32_t BaseHelper::StringCRC32(const wchar_t* str, const wchar_t* end, uint32_t crc)
        {
            CRC32 calc(crc);
            calc.append(str, (end-str) * sizeof(wchar_t));
            return calc.crc();
        }

        uint64_t BaseHelper::StringCRC64(const wchar_t* str, const wchar_t* end, uint64_t crc)
        {
            CRC64 calc(crc);
            calc.append(str, (end-str) * sizeof(wchar_t));
            return calc.crc();
        }

        //--

        // http://www.geeksforgeeks.org/wildcard-character-matching/
        // TODO: non recursive version
        // TODO: UTF8 FFS....

        template<typename Ch>
        struct MatchCase
        {
            static ALWAYS_INLINE bool Match(Ch a, Ch b)
            {
                return a == b;
            }
        };

        template<typename Ch>
        struct MatchNoCase
        {
            static ALWAYS_INLINE bool Match(Ch a, Ch b)
            {
                if (a >= 'A' && a <= 'Z') a = (a - 'A') + 'a';
                if (b >= 'A' && b <= 'Z') b = (b - 'A') + 'a';
                return a == b;
            }
        };

        template<typename Ch, typename Matcher = MatchCase<Ch>>
        bool MatchWildcardPattern(const Ch* str, const Ch* strEnd, const Ch* pattern, const Ch* patternEnd)
        {
            const Ch* w = NULL; // last `*`
            const Ch* s = NULL; // last checked char

            // loop 1 char at a time
            while (1)
            {
                // end of string ?
                if (str >= strEnd)
                {
                    // pattern also ended, YAY
                    if (pattern == patternEnd)
                        break;

                    if (s < strEnd)
                        return false;

                    str = s++;
                    pattern = w;
                    continue;
                }
                else if (Matcher::Match(*pattern, *str))
                {
                    if ('*' == *pattern)
                    {
                        w = ++pattern;
                        s = str;
                        // "*" -> "foobar"
                        if (pattern < patternEnd)
                            continue;
                        break;
                    }
                    else if (w)
                    {
                        ++str;
                        // "*ooba*" -> "foobar"
                        continue;
                    }

                    return false;
                }

                ++str;
                ++pattern;
            }

            // matched
            return true;
        }

        template< typename Ch >
        bool MatchWildcardPatternNoCase(const Ch* str, const Ch* strEnd, const Ch* pattern, const Ch* patternEnd)
        {
            return MatchWildcardPattern<Ch, MatchNoCase<Ch>>(str, strEnd, pattern, patternEnd);
        }

        ///--

        bool BaseHelper::MatchPattern(StringView<char> str, StringView<char> pattern)
        {
            return MatchWildcardPattern(str.data(), str.data() + str.length(), pattern.data(), pattern.data() + pattern.length());
        }

        bool BaseHelper::MatchPattern(const StringView<wchar_t>& str, const StringView<wchar_t>& pattern)
        {
            return MatchWildcardPattern(str.data(), str.data() + str.length(), pattern.data(), pattern.data() + pattern.length());
        }

        bool BaseHelper::MatchPatternNoCase(StringView<char> str, StringView<char> pattern)
        {
            return MatchWildcardPatternNoCase(str.data(), str.data() + str.length(), pattern.data(), pattern.data() + pattern.length());
        }

        bool BaseHelper::MatchPatternNoCase(const StringView<wchar_t>& str, const StringView<wchar_t>& pattern)
        {
            return MatchWildcardPatternNoCase(str.data(), str.data() + str.length(), pattern.data(), pattern.data() + pattern.length());
        }

        //--

        bool BaseHelper::MatchString(StringView<char> str, StringView<char> pattern)
        {
            return str.findStr(pattern) != INDEX_NONE;
        }

        bool BaseHelper::MatchString(const StringView<wchar_t>& str, const StringView<wchar_t>& pattern)
        {
            return str.findStr(pattern) != INDEX_NONE;
        }

        bool BaseHelper::MatchStringNoCase(StringView<char> str, StringView<char> pattern)
        {
            return str.findStrNoCase(pattern) != INDEX_NONE;
        }

        bool BaseHelper::MatchStringNoCase(const StringView<wchar_t>& str, const StringView<wchar_t>& pattern)
        {
            return str.findStrNoCase(pattern) != INDEX_NONE;
        }

        //--

        static const char Digits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E','F' };

        static bool GetNumberValueForDigit(char ch, uint32_t base, uint8_t& outDigit)
        {
            for (uint8_t i=0; i<base; ++i)
            {
                if (Digits[i] == ch)
                {
                    outDigit = i;
                    return true;
                }
            }

            return false;
        }

        template<typename T>
        INLINE bool CheckNumericalOverflow(T val, T valueToAdd)
        {
            if (valueToAdd > 0)
            {
                auto left = std::numeric_limits<T>::max() - val;
                return valueToAdd > left;
            }
            else if (valueToAdd < 0)
            {
                auto left = std::numeric_limits<T>::lowest() - val;
                return valueToAdd < left;
            }

            return false;
        }

        template<typename Ch, typename T>
        INLINE static MatchResult AMatchInteger(const Ch *str, T &outValue, size_t strLength, uint32_t base)
        {
            static_assert(std::is_signed<T>::value || std::is_unsigned<T>::value, "Only integer types are allowed here");

            // empty strings are not valid input to this function
            if (!str || !*str)
                return MatchResult::EmptyString;

            // determine start and end of parsing range as well as the sign
            auto negative = (*str == '-');
            auto strStart = (*str == '+' || *str == '-') ? str + 1 : str;
            auto strEnd = str + strLength;

            // unsigned values cannot be negative :)
            if (std::is_unsigned<T>::value && negative)
                return MatchResult::InvalidCharacter;

            T minValue = std::numeric_limits<T>::min();
            T maxValue = std::numeric_limits<T>::max();

            T value = 0;
            T mult = negative ? -1 : 1;

            // assemble number
            auto pos = strEnd;
            bool overflowed = false;
            while (pos > strStart)
            {
                auto ch = *(--pos);

                // if a non-zero digit is encountered we must make sure that he mult is not overflowed already
                uint8_t digitValue;
                if (!GetNumberValueForDigit((char)ch, base, digitValue))
                    return MatchResult::InvalidCharacter;

                // apply
                if (digitValue != 0 && overflowed)
                    return MatchResult::Overflow;

                // validate that we will not overflow the type
                auto valueToAdd = range_cast<T>(digitValue * mult);
                if ((valueToAdd / mult) != digitValue)
                    return MatchResult::Overflow;
                if (prv::CheckNumericalOverflow<T>(value, valueToAdd))
                    return MatchResult::Overflow;

                // accumulate
                value += valueToAdd;

                // advance to next multiplier
                T newMult = mult * 10;
                if (newMult / 10 != mult)
                    overflowed = true;
                mult = newMult;
            }

            outValue = value;
            return MatchResult::OK;
        }

        template<typename Ch>
        INLINE MatchResult AMatchFloat(const Ch* str, double& outValue, size_t strLength)
        {
            // empty strings are not valid input to this function
            if (!str || !*str)
                return MatchResult::EmptyString;

            // determine start and end of parsing range as well as the sign
            auto negative = (*str == '-');
            auto strEnd  = str + strLength;
            auto strStart  = (*str == '+' || *str == '-') ? str + 1 : str;

            // validate that we have a proper characters, discover the decimal point position
            auto strDecimal  = strEnd; // if decimal point was not found assume it's at the end
            {
                auto pos  = strStart;
                while (pos < strEnd)
                {
                    auto ch = *pos++;

                    if (pos == strEnd && ch == 'f')
                        break;

                    if (ch == '.')
                    {
                        strDecimal = pos - 1;
                    }
                    else
                    {
                        uint8_t value = 0;
                        if (!prv::GetNumberValueForDigit((char)ch, 10, value))
                            return MatchResult::InvalidCharacter;
                    }
                }
            }

            // accumulate values
            double value = 0.0f;

            // TODO: this is tragic where it comes to the precision loss....
            // TODO: overflow/underflow
            {
                double mult = 1.0f;

                auto pos  = strDecimal;
                while (pos > strStart)
                {
                    auto ch = *(--pos);

                    uint8_t digitValue = 0;
                    if (!prv::GetNumberValueForDigit((char)ch, 10, digitValue))return MatchResult::InvalidCharacter;

                    // accumulate
                    value += (double)digitValue * mult;
                    mult *= 10.0;
                }
            }

            // Fractional part
            if (strDecimal < strEnd)
            {
                double mult = 0.1f;

                auto pos  = strDecimal + 1;
                while (pos < strEnd)
                {
                    auto ch = *(pos++);

                    if (pos == strEnd && ch == 'f')
                        break;

                    uint8_t digitValue = 0;
                    if (!prv::GetNumberValueForDigit((char)ch, 10, digitValue))
                        return MatchResult::InvalidCharacter;

                    // accumulate
                    value += (double)digitValue * mult;
                    mult /= 10.0;
                }
            }

            outValue = negative ? -value : value;
            return MatchResult::OK;
        }

        MatchResult BaseHelper::MatchInteger(const char* str, char& outValue, size_t strLength, uint32_t base)
        {
            return AMatchInteger(str, outValue, strLength, base);
        }

        MatchResult BaseHelper::MatchInteger(const char* str, short& outValue, size_t strLength, uint32_t base)
        {
            return prv::AMatchInteger(str, outValue, strLength, base);
        }

        MatchResult BaseHelper::MatchInteger(const char* str, int& outValue, size_t strLength, uint32_t base)
        {
            return prv::AMatchInteger(str, outValue, strLength, base);
        }

        MatchResult BaseHelper::MatchInteger(const char* str, int64_t& outValue, size_t strLength, uint32_t base)
        {
            return prv::AMatchInteger(str, outValue, strLength, base);
        }

        MatchResult BaseHelper::MatchInteger(const char* str, uint8_t& outValue, size_t strLength, uint32_t base)
        {
            return prv::AMatchInteger(str, outValue, strLength, base);
        }

        MatchResult BaseHelper::MatchInteger(const char* str, uint16_t& outValue, size_t strLength, uint32_t base)
        {
            return prv::AMatchInteger(str, outValue, strLength, base);
        }

        MatchResult BaseHelper::MatchInteger(const char* str, uint32_t& outValue, size_t strLength, uint32_t base)
        {
            return prv::AMatchInteger(str, outValue, strLength, base);
        }

        MatchResult BaseHelper::MatchInteger(const char* str, uint64_t& outValue, size_t strLength, uint32_t base)
        {
            return prv::AMatchInteger(str, outValue, strLength, base);
        }

        //----

        MatchResult BaseHelper::MatchInteger(const wchar_t* str, char& outValue, size_t strLength, uint32_t base)
        {
            return prv::AMatchInteger(str, outValue, strLength, base);
        }

        MatchResult BaseHelper::MatchInteger(const wchar_t* str, short& outValue, size_t strLength, uint32_t base)
        {
            return prv::AMatchInteger(str, outValue, strLength, base);
        }

        MatchResult BaseHelper::MatchInteger(const wchar_t* str, int& outValue, size_t strLength, uint32_t base)
        {
            return prv::AMatchInteger(str, outValue, strLength, base);
        }

        MatchResult BaseHelper::MatchInteger(const wchar_t* str, int64_t& outValue, size_t strLength, uint32_t base)
        {
            return prv::AMatchInteger(str, outValue, strLength, base);
        }

        MatchResult BaseHelper::MatchInteger(const wchar_t* str, uint8_t& outValue, size_t strLength, uint32_t base)
        {
            return prv::AMatchInteger(str, outValue, strLength, base);
        }

        MatchResult BaseHelper::MatchInteger(const wchar_t* str, uint16_t& outValue, size_t strLength, uint32_t base)
        {
            return prv::AMatchInteger(str, outValue, strLength, base);
        }

        MatchResult BaseHelper::MatchInteger(const wchar_t* str, uint32_t& outValue, size_t strLength, uint32_t base)
        {
            return prv::AMatchInteger(str, outValue, strLength, base);
        }

        MatchResult BaseHelper::MatchInteger(const wchar_t* str, uint64_t& outValue, size_t strLength, uint32_t base)
        {
            return prv::AMatchInteger(str, outValue, strLength, base);
        }

        MatchResult BaseHelper::MatchFloat(const char* str, double& outValue, size_t strLength /*= FULL_STRING_LENGTH*/)
        {
            return prv::AMatchFloat(str, outValue, strLength);
        }

        MatchResult BaseHelper::MatchFloat(const wchar_t* str, double& outValue, size_t strLength /*= FULL_STRING_LENGTH*/)
        {
            return prv::AMatchFloat(str, outValue, strLength);
        }

    } // prv
} // base