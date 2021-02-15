/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: string #]
***/

#include "build.h"
#include "stringBuilder.h"
#include "inplaceArray.h"

namespace base
{

    ///---

    namespace helper
    {
        struct Region
        {
            const char* m_start;
            const char* m_end;
            StringView m_name;
            uint64_t m_hash;
        };

        static bool IsInsertionPointMarkerStart(const char* str)
        {
            return (str[0] == '$') && (str[1] == '{');
        }

        static bool IsInsertionPointMarkerEnd(const char* str)
        {
            return (str[0] == '}');
        }

        static bool ExtractInsertionPointName(const char*& str, StringView& outName)
        {
            auto start  = str;
            while (*str)
            {
                if (IsInsertionPointMarkerEnd(str))
                {
                    outName = StringView(start, str);
                    str += 3;
                    return true;
                }

                str += 1;
            }

            return false;
        }

        static void SplitIntoInsertionRegions(StringView templateText, Array<Region>& outRegions)
        {
            auto str  = templateText.data();
            auto endStr  = str + templateText.length();

            while (*str)
            {
                // find the insertion point
                auto start  = str;
                auto end  = str;
                StringView name;
                while (*str)
                {
                    if (IsInsertionPointMarkerStart(str))
                    {
                        end = str;

                        str += 2;
                        ExtractInsertionPointName(str, name);
                        break;
                    }

                    end = str;
                    str += 1;
                }

                // create region
                auto& region = outRegions.emplaceBack();
                region.m_start = start;
                region.m_end = end;
                region.m_name = name;
                region.m_hash = name.calcCRC64();
            }
        }

    } // helper

    void ReplaceText(IFormatStream& f, StringView templateText, const ReplaceTextPattern* patterns, uint32_t numPatterns)
    {
        // split into regions
        InplaceArray<helper::Region, 100> regions;
        helper::SplitIntoInsertionRegions(templateText, regions);

        // assemble code
        for (auto& region : regions)
        {
            // append block text
            auto length  = range_cast<uint32_t>(region.m_end - region.m_start);
            f.append(region.m_start, length);

            // find the text for the insertion point
            if (!region.m_name)
            {
                for (uint32_t i = 0; i < numPatterns; ++i)
                {
                    if (patterns[i].name == region.m_name)
                    {
                        f.append(patterns[i].data.data(), patterns[i].data.length());
                        break;
                    }
                }
            }
        }
    }

    ///---

} // base
