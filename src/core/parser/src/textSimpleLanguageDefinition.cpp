/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: parser #]
***/

#include "build.h"
#include "textToken.h"
#include "textLanguageDefinition.h"
#include "textSimpleLanguageDefinition.h"
#include "textParsingTreeBuilder.h"
#include "core/containers/include/stringBuilder.h"

BEGIN_BOOMER_NAMESPACE()

//--

auto DefaultIdentFirstChars  = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
auto DefaultIdentNextChars  = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

//--

SimpleLanguageDefinitionBuilder::SimpleLanguageDefinitionBuilder()
    : m_enableInts(true)
    , m_enableFloats(true)
    , m_enableStrings(true)
    , m_enableNames(true)
    , m_enableStringsAsNames(false)
{
    identChars(DefaultIdentFirstChars, DefaultIdentNextChars);
}

void SimpleLanguageDefinitionBuilder::clear()
{
    m_enableInts = false;
    m_enableFloats = false;
    m_enableStrings = false;
    m_enableNames = false;
    m_enableStringsAsNames = false;
    m_chars.reset();
    m_keywords.reset();
    identChars(DefaultIdentFirstChars, DefaultIdentNextChars);
}

void SimpleLanguageDefinitionBuilder::identChars(const char* firstChars, const char* nextChars)
{
    m_firstIdentChars.reset();
    m_nextIdentChars.reset();

    while (*firstChars)
        m_firstIdentChars.pushBack(*firstChars++);
    m_firstIdentChars.pushBack(0);

    while (*nextChars)
        m_nextIdentChars.pushBack(*nextChars++);
    m_nextIdentChars.pushBack(0);
}

void SimpleLanguageDefinitionBuilder::enableIntegerNumbers(bool enabled)
{
    m_enableInts = enabled;
}

void SimpleLanguageDefinitionBuilder::enableFloatNumbers(bool enabled)
{
    m_enableFloats = enabled;
}

void SimpleLanguageDefinitionBuilder::enableStrings(bool enabled)
{
    m_enableStrings = enabled;
}

void SimpleLanguageDefinitionBuilder::enableStringsAsNames(bool enabled)
{
    m_enableStringsAsNames = enabled;
}

void SimpleLanguageDefinitionBuilder::enableNames(bool enabled)
{
    m_enableNames = enabled;
}

void SimpleLanguageDefinitionBuilder::addChar(char ch)
{
    m_chars.pushBack(ch);
}

void SimpleLanguageDefinitionBuilder::addKeyword(const char* txt, int id)
{
    ASSERT(*txt && txt);

    auto& keyword = m_keywords.emplaceBack();
    keyword.m_text = StringBuf(txt);
    keyword.m_id = id;
    keyword.m_preprocesor = false;
}

void SimpleLanguageDefinitionBuilder::addPreprocessor(const char* txt)
{
    ASSERT(*txt && txt);

    auto& keyword = m_keywords.emplaceBack();
    keyword.m_text = StringBuf(txt);
    keyword.m_id = -1;
    keyword.m_preprocesor = true;
}

UniquePtr<ITextLanguageDefinition> SimpleLanguageDefinitionBuilder::buildLanguageDefinition() const
{
    // create a parsing tree builder
    prv::ParsingTreeBuilder tree;

    // insert identifier parser to prime the continuation tables
    if (m_firstIdentChars.size() > 1)
        tree.insertIdentifiersMatch(m_firstIdentChars.typedData(), m_nextIdentChars.typedData());

    // insert specialized parsers
    if (m_enableStrings) tree.insertStringMatch(m_enableStringsAsNames);
    if (m_enableNames) tree.insertNameMatch();
    if (m_enableInts) tree.insertIntegerMatch();
    if (m_enableFloats) tree.insertFloatingPointMatch();

    // insert char matches
    for (auto ch : m_chars)
        tree.insertCharMatch(ch);

    // insert keyword matches
    for (auto& k : m_keywords)
        tree.insertKeywordMatch(k.m_text.c_str(), k.m_id);

    // dump the structure
    //logging::ILogStream::GetStream() << tree;

    // pack into tables and create fast runtime parser
    return tree.buildLanguageDefinition();
}

//--

END_BOOMER_NAMESPACE()
