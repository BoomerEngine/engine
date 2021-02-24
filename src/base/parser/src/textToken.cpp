/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: parser #]
***/

#include "build.h"
#include "textToken.h"
#include "base/containers/include/stringParser.h"

BEGIN_BOOMER_NAMESPACE(base::parser)

//--

void Location::print(base::IFormatStream& f) const
{
    if (m_contextName.empty())
        f << "unknown";
    else if (m_pos != 0)
#ifdef PLATFORM_WINDOWS
        f.appendf("{}({},{})", m_contextName, m_line, m_pos);
#else
        f.appendf("{}({}:{})", m_contextName, line, offset);
#endif
    else if (m_line != 0)
        f.appendf("{}({})", m_contextName, m_line);
    else if (m_line != 0)
        f <<m_contextName;
}

//--

StringBuf Token::string() const
{
    if (m_end == m_str)
        return StringBuf::EMPTY();

    return StringBuf(m_str, m_end - m_str);
}

const char* Token::typeName() const
{
    switch (m_type)
    {
        case TextTokenType::Invalid: return "Invalid";
        case TextTokenType::String: return "String";
        case TextTokenType::Name: return "Name";
        case TextTokenType::IntNumber: return "IntNumber";
        case TextTokenType::UnsignedNumber: return "UnsignedNumber";
        case TextTokenType::FloatNumber: return "FloatNumber";
        case TextTokenType::Char: return "Char";
        case TextTokenType::Keyword: return "Keyword";
        case TextTokenType::Identifier: return "Identifier";
        case TextTokenType::Preprocessor: return "Preprocessor";
    }

    return "Unknown";
}

StringView Token::view() const
{
    if (m_end == m_str)
        return StringView();

    return StringView(m_str, m_end);
}

double Token::floatNumber() const
{
    if (m_end == m_str || !isNumber())
        return 0.0;

    double ret = 0.0;
    view().match(ret);
    return ret;
}
        
//--

void Token::print(IFormatStream& f) const
{
    f.appendf("{} ({})", view(), typeName());
}

//--

void TokenList::clear()
{
    m_head = nullptr;
    m_tail = nullptr;
}

void TokenList::pushBack(Token* token)
{
    if (token)
    {
        if (m_tail)
            m_tail->m_next = token;
        token->m_prev = m_tail;
        m_tail = token;

        if (!m_head)
            m_head = token;
    }
}

void TokenList::pushFront(Token* token)
{
    if (token)
    {
        if (m_head)
            m_head->m_prev = token;
        token->m_next = m_head;
        m_head = token;

        if (!m_tail)
            m_tail = token;
    }
}

void TokenList::pushBack(TokenList&& list)
{
    if (!list.empty())
    {
        if (!empty())
        {
            m_tail->m_next = list.m_head;
            list.m_head->m_prev = m_tail;
            m_tail = list.m_tail;
        }
        else
        {
            m_head = list.m_head;
            m_tail = list.m_tail;
        }

        list.m_head = nullptr;
        list.m_tail = nullptr;
    }
}

void TokenList::pushFront(TokenList&& list)
{
    if (!list.empty())
    {
        if (!empty())
        {
            m_head->m_prev = list.m_tail;
            list.m_tail->m_next = m_head;
            m_head = list.m_head;
        }
        else
        {
            m_head = list.m_head;
            m_tail = list.m_tail;
        }

        list.m_head = nullptr;
        list.m_tail = nullptr;
    }
}

Token* TokenList::unlink(Token* token)
{
    if (token)
    {
        auto next  = token->next();

        if (token->m_next)
        {
            DEBUG_CHECK(token != m_tail);
            token->m_next->m_prev = token->m_prev;
        }
        else
        {
            DEBUG_CHECK(token == m_tail);
            m_tail = token->m_prev;
        }

        if (token->m_prev)
        {
            DEBUG_CHECK(token != m_head);
            token->m_prev->m_next = token->m_next;
        }
        else
        {
            DEBUG_CHECK(token == m_head);
            m_head = token->m_next;
        }

        token->m_prev = nullptr;
        token->m_next = nullptr;

        return next;
    }
    else
    {
        return nullptr;
    }
}

void TokenList::linkAfter(Token* place, Token* other)
{
    if (other)
    {
        DEBUG_CHECK(place != other);
        if (place)
        {
            DEBUG_CHECK(!empty());
        }
        else
        {
            DEBUG_CHECK(empty());
            pushBack(other);
        }
    }
}

void TokenList::linkBefore(Token* place, Token* other)
{
    if (other)
    {
        DEBUG_CHECK(place != other);
        if (place)
        {
            DEBUG_CHECK(!empty());
        }
        else
        {
            DEBUG_CHECK(empty());
            pushBack(other);
        }
    }
}

Token* TokenList::popFront()
{
    Token* ret = nullptr;

    if (m_head)
    {
        ret = m_head;

        if (m_head->m_next)
        {
            m_head->m_next->m_prev = nullptr;
            m_head = m_head->m_next;
        }
        else
        {
            m_head = nullptr;
            m_tail = nullptr;
        }

        ret->m_next = nullptr;
        ret->m_prev = nullptr;
    }

    return ret;
}

Token* TokenList::popBack()
{
    Token* ret = nullptr;

    if (m_tail)
    {
        ret = m_tail;

        if (m_tail->m_prev)
        {
            m_tail->m_prev->m_next = nullptr;
            m_tail = m_tail->m_prev;
        }
        else
        {
            m_head = nullptr;
            m_tail = nullptr;
        }

        ret->m_next = nullptr;
        ret->m_prev = nullptr;
    }

    return ret;
}

Token* TokenList::unlinkLine(Token* start, TokenList& outList)
{
    auto cur  = start;
    uint32_t startLine = cur->location().line();
    while (cur != nullptr)
    {
        if (cur->location().line() != startLine)
            break;

        auto next  = cur->next();

        unlink(cur);
        outList.pushBack(cur);

        cur = next;
    }

    return cur;
}

void TokenList::print(IFormatStream& f) const
{
    if (auto cur  = head())
    {
        auto line = cur->location().line();
        auto pos = cur->location().charPos();
        auto file = cur->location().contextName();
        while (cur)
        {
            if (cur->location().contextName() != file)
            {
                f << "\n";
                file = cur->location().contextName();
                line = cur->location().line();
                pos = cur->location().charPos();
            }
            else if (cur->location().line() != line)
            {
                f << "\n";
                line = cur->location().line();
                pos = cur->location().charPos();
            }

            auto intendedPos = cur->location().charPos();
            if (intendedPos > pos)
                f.appendPadding(' ', intendedPos - pos);

            if (cur->isString())
            {
                f << "\"";
                f << cur->view();
                f << "\"";
            }
            else if (cur->isString())
            {
                f << "\'";
                f << cur->view();
                f << "\'";
            }
            else
            {
                f << cur->view();
            }
            pos = cur->location().charPos() + cur->view().length();
            cur = cur->next();
        }
    }
}
        
//--

END_BOOMER_NAMESPACE(base::parser)