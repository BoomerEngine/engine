/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: string #]
***/

#include "build.h"
#include "stringVector.h"
#include "utf8StringFunctions.h"

BEGIN_BOOMER_NAMESPACE(base)

namespace prv
{

    template<typename T>
    struct ZeroGuard : public base::NoCopy
    {
    public:
        INLINE ZeroGuard(Array<T>& table)
            : m_table(table)
        {
            if (!table.empty() && table.back() == 0)
                table.popBack();
        }

        INLINE ~ZeroGuard()
        {
            if (!m_table.empty() && m_table.back() != 0)
                m_table.pushBack(0);
        }

    private:
        Array<T>& m_table;
    };

    //---

    void BaseHelper::Append(Array<char>& table, StringView view)
    {
        ZeroGuard<char> zg(table);

        for (auto ch : view)
            table.pushBack(ch);
    }

    void BaseHelper::Append(Array<wchar_t>& table, StringView view)
    {
        ZeroGuard<wchar_t> zg(table);

        auto sizeReq = view.length();
        table.reserve(range_cast<uint32_t>(table.size() + sizeReq + 1));

        auto ptr  = view.data();
        auto endPtr  = view.data() + view.length();
        while (ptr < endPtr)
        {
            auto ch = utf8::NextChar(ptr, endPtr);
            table.pushBack((wchar_t) ch);
        }
    }

    void BaseHelper::Append(Array<char>& table, const BaseStringView<wchar_t>& view)
    {
        ZeroGuard<char> zg(table);

        auto sizeReq = utf8::CalcSizeRequired(view.data(), view.length());
		table.reserve(range_cast<uint32_t>(table.size() + sizeReq + 1));

        for (auto ch : view)
        {
            char buf[6];
            auto size = utf8::ConvertChar(buf, ch);
            for (uint32_t i=0; i<size; ++i)
                table.pushBack(buf[i]);
        }
    }

    void BaseHelper::Append(Array<wchar_t>& table, const BaseStringView<wchar_t>& view)
    {
        ZeroGuard<wchar_t> zg(table);

        auto sizeReq = view.length();
		table.reserve(range_cast<uint32_t>(table.size() + sizeReq + 1));

        for (auto ch : view)
            table.pushBack(ch);
    }

} // prv

END_BOOMER_NAMESPACE(base)

