/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: rtti\view #]
***/

#include "build.h"
#include "rttiDataView.h"
#include "rttiType.h"
#include "rttiNativeClassType.h"

namespace base
{
    namespace rtti
    {

        //--

        ALWAYS_INLINE bool IsValidIdentChar(char ch)
        {
            return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || (ch == '_');
        }

        ALWAYS_INLINE bool IsValidNumberChar(char ch)
        {
            return (ch >= '0' && ch <= '9');
        }

        bool ParsePropertyName(StringView<char>& view, StringView<char>& outPropertyName)
        {
            auto cur  = view.data();
            auto end  = cur + view.length();

            if (cur < end && *cur == '.')
                cur += 1; // just skip the first '.'

            if (cur < end && *cur == '[')
                return false; // it will be an array

            auto identStart  = cur;
            while (cur < end)
            {
                if (*cur == '[' || *cur == '.')
                    break;

                if (!IsValidIdentChar(*cur))
                    return false; // whoops...

                ++cur;
            }

            outPropertyName = StringView<char>(identStart, cur);
            view = StringView<char>(cur, end);
            return !outPropertyName.empty();
        }

        bool ParseArrayIndex(StringView<char>& view, uint32_t& outArrayCount)
        {
            auto cur  = view.data();
            auto end  = cur + view.length();

            if (cur >= end || *cur != '[')
                return false; // does not look like array bracket

            cur += 1;

            auto numberStart  = cur;
            while (cur < end)
            {
                if (*cur == ']')
                {
                    auto numberView = StringView<char>(numberStart, cur);
                    if (MatchResult::OK == numberView.match(outArrayCount))
                    {
                        view = StringView<char>(cur+1, end);
                        return true;
                    }

                    return false;
                }

                if (!IsValidNumberChar(*cur))
                    return false; // whoops...

                ++cur;
            }

            return false;
        }

        bool ExtractParentPath(StringView<char>& view, StringView<char>& outChild)
        {
            if (view.empty())
                return false;

            if (view.endsWith("]"))
            {
                auto index = view.findLastChar('[');
                if (index != INDEX_NONE)
                {
                    outChild = view.subString(index);
                    view = view.leftPart(index);
                    return true;
                }
            }
            else
            {
                auto index = view.findLastChar('.');
                if (index != INDEX_NONE)
                {
                    outChild = view.subString(index+1);
                    view = view.leftPart(index);
                    return true;
                }
                else
                {
                    outChild = view;
                    view = StringView<char>();
                    return true;
                }
            }

            return false;
        }

        //--

    } // rtti
} // base

