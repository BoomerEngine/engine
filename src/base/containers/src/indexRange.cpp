/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#include "build.h"
#include "indexRange.h"

BEGIN_BOOMER_NAMESPACE(base)

///---

#ifndef BUILD_RELEASE
void IndexRange::checkNotEmpty() const
{
    ASSERT_EX(!empty(), "Index range cannot be empty in here");
}
#endif

void IndexRange::splitExclude(Index at, IndexRange& outLeft, Index& outRight) const
{
    if (contains(at))
    {
        if (at > m_first)
            outLeft = IndexRange(m_first, (at - m_first));
        else
            outLeft = IndexRange();

        const auto last = m_first + m_count - 1;
        if (at < last)
            outRight = IndexRange(at + 1, last - at);
        else
            outRight = IndexRange();
    }
    else
    {
        outLeft = IndexRange();
        outRight = IndexRange();
    }
}

void IndexRange::splitLeft(Index at, IndexRange& outLeft, Index& outRight) const
{
    if (contains(at))
    {
        outLeft = IndexRange(m_first, (at - m_first) + 1);

        const auto last = m_first + m_count - 1;
        if (at < last)
            outRight = IndexRange(at + 1, last - at);
        else
            outRight = IndexRange();
    }
    else
    {
        outLeft = IndexRange();
        outRight = *this;
    }
}

void IndexRange::splitRight(Index at, IndexRange& outLeft, Index& outRight) const
{
    if (contains(at))
    {
        if (at > m_first)
            outLeft = IndexRange(m_first, (at - m_first));
        else
            outLeft = IndexRange();

        const auto last = m_first + m_count;
        outRight = IndexRange(at, last - at);
    }
    else
    {
        outLeft = *this;
        outRight = IndexRange();
    }
}

void IndexRange::print(IFormatStream& f) const
{
    if (empty())
        f.append("empty");
    else
        f.appendf("[{}-{}]", first(), last());
}

///---

#ifndef BUILD_RELEASE
void ReversedIndexRange::checkNotEmpty() const
{
    ASSERT_EX(!empty(), "Index range cannot be empty in here");
}
#endif

void ReversedIndexRange::print(IFormatStream& f) const
{
    if (empty())
        f.append("empty");
    else
        f.appendf("[{}-{}]", last(), first());
}

///---

END_BOOMER_NAMESPACE(base)

