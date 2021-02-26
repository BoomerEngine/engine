/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: parser #]
***/

#include "build.h"
#include "textParsingTreeBuilder.h"
#include "core/containers/include/inplaceArray.h"
#include "core/containers/include/hashMap.h"
#include "core/containers/include/stringBuilder.h"

BEGIN_BOOMER_NAMESPACE_EX(parser)

namespace prv
{


    ParsingTreeBuilder::ParsingTreeBuilder()
        : m_mem(POOL_TEMP)
    {
        // create root node, it will parse nothing
        m_root = m_mem.create<ParsingNode>();
        m_root->m_index = 0;
        m_nodes.pushBack(m_root);
    }

    ParsingTreeBuilder::~ParsingTreeBuilder()
    {

    }

    ParsingNode* ParsingTreeBuilder::duplicateNode(ParsingNode* node)
    {
        auto newNode  = (node == nullptr) ? m_mem.create<ParsingNode>() : m_mem.create<ParsingNode>(*node);
        newNode->m_index = m_nodes.size();
        m_nodes.pushBack(newNode);
        return newNode;
    }

    ParsingNode* ParsingTreeBuilder::uniqueMatchNode(ParsingNode* atNode, const char* chars)
    {
        // nothing to parse
        if (!*chars)
            return nullptr;

        // look at the char list to determine if we already have a unique node
        ParsingNode *followerNode = nullptr;
        uint32_t numMatchingChars = 0;
        for (auto ch  = chars; *ch; ++ch)
        {
            // we don't have a follower node at this char
            if (atNode->m_continuation[*ch].m_node == nullptr)
                return nullptr;

            // make sure it's the same node for all chars
            if (!followerNode)
                followerNode = atNode->m_continuation[*ch].m_node;
            else if (atNode->m_continuation[*ch].m_node != followerNode)
                return nullptr;

            numMatchingChars += 1;
        }

        // make sure this node is not used by other chars
        uint32_t numMatchingContinuations = 0;
        for (uint32_t i=0; i<ARRAY_COUNT(atNode->m_continuation); ++i)
            if (atNode->m_continuation[i].m_node == followerNode)
                numMatchingContinuations += 1;

        // we can't have any other references
        if (numMatchingChars != numMatchingContinuations)
            return nullptr;
        return followerNode;
    }

    void ParsingTreeBuilder::insertUniqueMatchNode(ParsingNode* atNode, const char* chars, Array<ParsingNode*>& outContinuationNodes)
    {
        // try to reuse existing unique node
        auto uniqueMatchNode  = this->uniqueMatchNode(atNode, chars);
        if (uniqueMatchNode)
        {
            outContinuationNodes.pushBack(uniqueMatchNode);
            return;
        }

        // if we don't have a unique continuation node we need to create one
        // problem is that different chars may have different nodes already created, if that's the case we will have fragmented output (multiple continuation nodes)
        InplaceArray<ParsingNode*, 256> existingContinuationNodes;
        for (auto ch  = chars; *ch; ++ch)
        {
            // we don't have a follower node at this char
            auto continuationNode  = atNode->m_continuation[*ch].m_node;
            existingContinuationNodes.pushBackUnique(continuationNode);
        }

        // create a replacement for each node
        for (auto existingContination  : existingContinuationNodes)
        {
            auto copy  = duplicateNode(existingContination);
            outContinuationNodes.pushBack(copy);
        }

        // replace references
        for (auto ch  = chars; *ch; ++ch)
        {
            // we don't have a follower node at this char
            auto continuationNode  = atNode->m_continuation[*ch].m_node;
            auto nodeIndex  = existingContinuationNodes.find(continuationNode);
            ASSERT(nodeIndex != -1);
            atNode->m_continuation[*ch].m_node = outContinuationNodes[nodeIndex];
        }
    }

    ParsingNode* ParsingTreeBuilder::insertUniqueMatchNode(ParsingNode* atNode, char ch)
    {
        // get current match
        auto currentMatchNode  = atNode->m_continuation[ch].m_node;

        // make sure it's not used anywhere else
        if (currentMatchNode)
        {
            bool usedAtOtherNodes = false;
            for (uint32_t i = 0; i < ARRAY_COUNT(atNode->m_continuation); ++i)
            {
                if ((int)i != ch)
                {
                    if (atNode->m_continuation[i].m_node == currentMatchNode)
                    {
                        usedAtOtherNodes = true;
                        break;
                    }
                }
            }

            if (!usedAtOtherNodes)
                return currentMatchNode;
        }

        // duplicate existing match node
        auto newMatchNode  = duplicateNode(currentMatchNode);
        atNode->m_continuation[ch].m_node = newMatchNode;
        return newMatchNode;
    }

    void ParsingTreeBuilder::insertCharMatch(char ch)
    {
        auto singleCharMatch  = insertUniqueMatchNode(m_root, ch);
        ASSERT_EX(singleCharMatch->m_possibleEmit == TextTokenType::Invalid, "Final node already constructed")
        singleCharMatch->m_possibleEmit = TextTokenType::Char;
        singleCharMatch->m_keywordID = (int)ch; // hack
    }

    void ParsingTreeBuilder::insertKeywordMatch(const char* txt, int keywordID)
    {
        // start matching at root
        auto curMatchNode  = m_root;

        // follow the string creating unique match nodes (since we only map one char)
        {
            auto ch  = txt;
            while (*ch)
                curMatchNode = insertUniqueMatchNode(curMatchNode, *ch++);
        }

        // all the final states can be matched to keyword, set them as such
        ASSERT_EX(curMatchNode->m_possibleEmit == TextTokenType::Invalid || curMatchNode->m_possibleEmit == TextTokenType::Identifier, "Final node already constructed")
        if (keywordID > 0)
        {
            curMatchNode->m_possibleEmit = TextTokenType::Keyword;
            curMatchNode->m_keywordID = keywordID;
        }
        else
        {
            curMatchNode->m_possibleEmit = TextTokenType::Preprocessor;
            curMatchNode->m_keywordID = -1;
        }

        // add to map for debug lookup
        m_debugKeywordMap[keywordID] = StringBuf(txt);
    }

    void ParsingTreeBuilder::makeFinalNode(ParsingNode* node)
    {
        for (uint32_t i=0; i<ARRAY_COUNT(node->m_continuation); ++i)
            node->m_continuation[i].m_node = nullptr;
    }

    void ParsingTreeBuilder::spinOnSelf(ParsingNode* node)
    {
        for (uint32_t i=0; i<ARRAY_COUNT(node->m_continuation); ++i)
            node->m_continuation[i].m_node = node;
    }

    void ParsingTreeBuilder::spinOnSelf(ParsingNode* node, const char* spinChars)
    {
        while (*spinChars)
            node->m_continuation[*spinChars++].m_node = node;
    }

    void ParsingTreeBuilder::spinOnSelf(ParsingNode* node, char spinChar)
    {
        node->m_continuation[spinChar].m_node = node;
    }

    void ParsingTreeBuilder::insertStringMatch(bool asNames)
    {
        // match the double quotes
        auto firstCharMatch  = insertUniqueMatchNode(m_root, '\"');

        // set the matchnode as dummy "spin" node that will match multiple chars
        spinOnSelf(firstCharMatch);

        // we are looking for the final quote
        auto finalCharMatch  = insertUniqueMatchNode(firstCharMatch, '\"');

        // emit the string on the final match
        makeFinalNode(finalCharMatch);
        ASSERT_EX(finalCharMatch->m_possibleEmit == TextTokenType::Invalid, "Final node already constructed")
        finalCharMatch->m_possibleEmit = asNames ? TextTokenType::Name : TextTokenType::String;
    }

    void ParsingTreeBuilder::insertNameMatch(bool asStrings)
    {
        // match the first single quotes
        auto firstCharMatch  = insertUniqueMatchNode(m_root, '\'');

        // set the matchnode as dummy "spin" node that will match multiple chars
        spinOnSelf(firstCharMatch);

        // we are looking for the final quote
        auto finalCharMatch  = insertUniqueMatchNode(firstCharMatch, '\'');

        // emit the string on the final match
        makeFinalNode(finalCharMatch);
        ASSERT_EX(finalCharMatch->m_possibleEmit == TextTokenType::Invalid, "Final node already constructed")
        finalCharMatch->m_possibleEmit = asStrings ? TextTokenType::String : TextTokenType::Name;
    }

    void ParsingTreeBuilder::insertIdentifiersMatch(const char* firstChars, const char* nextChars)
    {
        // create nodes matching the characters from the first set
        InplaceArray<ParsingNode*, 64> firstNodes;
        insertUniqueMatchNode(m_root, firstChars, firstNodes);

        // make the spin nodes
        for (auto firstNode  : firstNodes)
        {
            spinOnSelf(firstNode, nextChars);
            firstNode->m_possibleEmit = TextTokenType::Identifier;
        }

        // 0:    f->1 u->1 n->1 c->1
        // 1:    f->1 u->1 n->1 c->1

        // 0:    f->2 u->1 n->1 c->1
        // 2:    f->1 u->3 n->1 c->1
        // 3:    f->1 u->1 n->4 c->1
        // 4:    f->1 u->1 n->1 c->5
        // 5:    f->1 u->1 n->1 c->1 // emit: FUNC

        // 0:    f->2 u->1 n->1 c->1
        // 2:    f->1 u->3 n->1 c->1
        // 3:    f->1 u->1 n->4 c->1
        // 4:    f->1 u->1 n->1 c->5
        // 5:    f->1 u->1 n->1 c->1 t->6
        // 6:    f->1 u->1 n->1 c->1 t->1 i->7
    }

    void ParsingTreeBuilder::insertIntegerMatch()
    {
        const char* numChars = "0123456789";

        // match the digits at root
        InplaceArray<ParsingNode*, 64> firstNodes;
        insertUniqueMatchNode(m_root, numChars, firstNodes);

        // make the number spin on itself as long as we have digits
        for (auto firstNode  : firstNodes)
        {
            spinOnSelf(firstNode, numChars);
            firstNode->m_possibleEmit = TextTokenType::IntNumber;

            // create the number finalization nodes
            {
                InplaceArray<ParsingNode*, 2> finalNodes;
                insertUniqueMatchNode(firstNode, "uU", finalNodes);

                for (auto finalNode  : finalNodes)
                {
                    makeFinalNode(finalNode);
                    ASSERT_EX(finalNode->m_possibleEmit == TextTokenType::Invalid, "Final node already constructed")
                    finalNode->m_possibleEmit = TextTokenType::UnsignedNumber;
                }
            }

            // create the number finalization nodes
            {
                InplaceArray<ParsingNode*, 2> finalNodes;
                insertUniqueMatchNode(firstNode, "iI", finalNodes);

                for (auto finalNode  : finalNodes)
                {
                    makeFinalNode(finalNode);
                    ASSERT_EX(finalNode->m_possibleEmit == TextTokenType::Invalid, "Final node already constructed")
                    finalNode->m_possibleEmit = TextTokenType::IntNumber;
                }
            }

            // create the number finalization nodes
            {
                InplaceArray<ParsingNode*, 2> finalNodes;
                insertUniqueMatchNode(firstNode, "f", finalNodes);

                for (auto finalNode  : finalNodes)
                {
                    makeFinalNode(finalNode);
                    ASSERT_EX(finalNode->m_possibleEmit == TextTokenType::Invalid, "Final node already constructed")
                    finalNode->m_possibleEmit = TextTokenType::FloatNumber;
                }
            }
        }
    }

    void ParsingTreeBuilder::insertFloatingPointMatch()
    {
        const char* numChars = "0123456789";

        // match the digits at root
        InplaceArray<ParsingNode*, 64> firstNodes;
        insertUniqueMatchNode(m_root, numChars, firstNodes);

        // make the number spin on itself as long as we have digits
        for (auto firstNode  : firstNodes)
        {
            // make outselves spin on the numbers
            // NOTE: we don't emit anything yet since in the floating point number we require a '.'
            spinOnSelf(firstNode, numChars);

            // create a node that follows the '.'
            auto fracNode  = insertUniqueMatchNode(firstNode, '.');

            // spin with fractional part
            spinOnSelf(fracNode, numChars);
            fracNode->m_possibleEmit = TextTokenType::FloatNumber;

            // we may end with 'f'
            {
                auto finalNode  = insertUniqueMatchNode(fracNode, 'f');
                makeFinalNode(finalNode);
                ASSERT_EX(finalNode->m_possibleEmit == TextTokenType::Invalid, "Final node already constructed")
                finalNode->m_possibleEmit = TextTokenType::FloatNumber;
            }
        }
    }

    static void AppendCharRanges(IFormatStream& str, const Array<char>& chars)
    {
        struct Range { char start, end; };
        InplaceArray<Range, 10> ranges;

        str.append("[");
        bool reqSep = false;

        uint32_t cur = 0;
        while (cur < chars.size())
        {
            uint32_t start = cur;
            char firstChar = chars[cur];
            char lastValidChar = firstChar;
            char expectedNextChar = firstChar + 1;
            cur += 1;

            while (cur < chars.size())
            {
                if (chars[cur] != expectedNextChar)
                    break;
                lastValidChar = chars[cur];
                expectedNextChar += 1;
                cur += 1;
            }

            if (reqSep)
                str.append(",");
            reqSep = true;

            char firstStr[] = {firstChar, 0};
            char lastStr[] = {lastValidChar, 0};
            if (firstChar == lastValidChar)
                str.append(firstStr);
            else
                str.appendf("{}-{}", firstStr, lastStr);
        }

        str.append("]");
    }

    void ParsingTreeBuilder::print(IFormatStream& str) const
    {
        // print the state machine
        for (uint32_t i=0; i<m_nodes.size(); ++i)
        {
            auto node  = m_nodes[i];

            str.appendf("[{}]: ", i);

            // collect stuff we follow, by node
            HashMap<ParsingNode*, Array<char>> continuations;
            for (uint32_t j=32; j<ARRAY_COUNT(node->m_continuation); ++j)
            {
                auto nextNode  = node->m_continuation[j].m_node;
                if (nextNode != nullptr)
                {
                    continuations[nextNode].pushBack((char)j);
                }
            }

            // print the groups
            if (continuations.empty())
            {
                str.append("FINAL ");
            }
            else
            {
                for (auto con : continuations.pairs())
                {
                    std::sort(con.value.begin(), con.value.end());
                    AppendCharRanges(str, con.value);
                    str.appendf("->{} ", con.key->m_index);
                }
            }

            // do we emit something ?
            switch (node->m_possibleEmit)
            {
                case TextTokenType::String:
                    str.append("emit: STRING");
                    break;

                case TextTokenType::Name:
                    str.append("emit: NAME");
                    break;

                case TextTokenType::IntNumber:
                    str.append("emit: INT");
                    break;

                case TextTokenType::UnsignedNumber:
                    str.append("emit: UINT");
                    break;

                case TextTokenType::FloatNumber:
                    str.append("emit: FLOAT");
                    break;

                case TextTokenType::Char:
                    str.appendf("emit: CHAR {}", node->m_keywordID);
                    break;

                case TextTokenType::Keyword:
                    str.appendf("emit: KEYWORD {} ({})", node->m_keywordID, m_debugKeywordMap[node->m_keywordID]);
                    break;

                case TextTokenType::Identifier:
                    str.append("emit: IDENT");
                    break;
            }

            str.append("\n");
        }
    }

    //--

    class CompiledParserTreeTextLanguageDefinition : public ILanguageDefinition
    {
    public:

#pragma pack(push)
#pragma pack(1)
        struct CompiledNode
        {
            TextTokenType m_emitType;
            uint8_t m_firstContinuationChar;
            short m_emitKeywordID;
            uint16_t m_firstContinuationIndex;
            uint16_t m_numContinuationIndices;

            INLINE CompiledNode()
                : m_emitType(TextTokenType::Invalid)
                , m_firstContinuationChar(0)
                , m_emitKeywordID(-1)
                , m_firstContinuationIndex(0)
                , m_numContinuationIndices(0)
            {}

            INLINE bool transitionIndex(char ch, uint16_t& outNextStateIndex) const
            {
                if (ch <= 0)
                    return false;

                if ((uint32_t)ch < m_firstContinuationChar)
                    return false;

                auto index  = (uint32_t)(ch - m_firstContinuationChar);
                if (index >= m_numContinuationIndices)
                    return false;

				outNextStateIndex = range_cast<uint16_t>(index + m_firstContinuationIndex);
                return true;
            }
        };
#pragma pack(pop)

        Array<CompiledNode> m_nodes;
        Array<uint16_t> m_continuationIndices;

        virtual bool eatToken(const char*& str, const char* endStr, Token& outToken) const override final
        {
            uint32_t curStateIndex = 0;

            // walk the state graph
            auto cur  = str;
            while (cur < endStr)
            {
                auto& curState = m_nodes[curStateIndex];

                // get the index of the transition
                uint16_t transitionIndex = 0;
                if (!curState.transitionIndex(*cur, transitionIndex))
                    break; // no transition to take, this is our final state

                // get the next state of the transition
                auto newStateIndex  = m_continuationIndices[transitionIndex];
                if (newStateIndex == 0)
                    break;

                // take the transition
                curStateIndex = newStateIndex;
                cur += 1;
            }

            // we've reached end of input, if the final state has anything to emit we will emit it, if not, we are done
            auto& finalState = m_nodes[curStateIndex];
            if (finalState.m_emitType == TextTokenType::Invalid)
                return false; // nothing to emit

            // emit what we have parsed
            if (finalState.m_emitType == TextTokenType::String || finalState.m_emitType == TextTokenType::Name)
                outToken = Token(finalState.m_emitType, str + 1, cur - 1, finalState.m_emitKeywordID);
            else
                outToken = Token(finalState.m_emitType, str, cur, finalState.m_emitKeywordID);
            str = cur; // advance the stream state
            return true;
        }
    };

    static uint32_t CountContinuations(const ParsingNode* node, uint8_t* firstIndex = nullptr)
    {
        uint32_t minIndex = ARRAY_COUNT(node->m_continuation);
        uint32_t maxIndex = 0;

        uint32_t i = 0;
        while (i < ARRAY_COUNT(node->m_continuation))
        {
            if (node->m_continuation[i].m_node != nullptr)
            {
                minIndex = i;
                break;
            }
            ++i;
        }

        while (i < ARRAY_COUNT(node->m_continuation))
        {
            if (node->m_continuation[i].m_node != nullptr)
                maxIndex = i;
            ++i;
        }

        if (maxIndex >= minIndex)
        {
            if (firstIndex)
                *firstIndex = range_cast<uint8_t>(minIndex);
            return (maxIndex - minIndex) + 1;
        }

        return 0;
    }

    static void CopyContinuations(const ParsingNode* node, uint32_t& writeIndex, Array<uint16_t>& outContinuationTable)
    {
        uint32_t i = 0;
        uint32_t minIndex = ARRAY_COUNT(node->m_continuation);
        while (i < ARRAY_COUNT(node->m_continuation))
        {
            if (node->m_continuation[i].m_node != nullptr)
            {
                minIndex = i;
                break;
            }
            ++i;
        }

        uint32_t maxIndex = 0;
        while (i < ARRAY_COUNT(node->m_continuation))
        {
            if (node->m_continuation[i].m_node != nullptr)
                maxIndex = i;
            ++i;
        }

        for (uint32_t j=minIndex; j<=maxIndex; ++j)
        {
            auto targetNode  = node->m_continuation[j].m_node;
            outContinuationTable[writeIndex++] = targetNode ? range_cast<uint16_t>(targetNode->m_index) : 0;
        }
    }

    UniquePtr<ILanguageDefinition> ParsingTreeBuilder::buildLanguageDefinition() const
    {
        auto ret  = CreateUniquePtr<CompiledParserTreeTextLanguageDefinition>();

        // preallocate node table
        ret->m_nodes.resize(m_nodes.size());

        // count the number of required continuation indices
        uint32_t numContinuations = 0;
        for (auto node  : m_nodes)
            numContinuations += CountContinuations(node);
        ret->m_continuationIndices.resize(numContinuations);

        // pack nodes
        uint32_t curContinuationIndex = 0;
        for (auto node  : m_nodes)
        {
            auto& targetNode = ret->m_nodes[node->m_index];
            targetNode.m_emitType = node->m_possibleEmit;
            targetNode.m_emitKeywordID = (short)node->m_keywordID;
            targetNode.m_numContinuationIndices = range_cast<uint16_t>(CountContinuations(node, &targetNode.m_firstContinuationChar));
            if (targetNode.m_numContinuationIndices)
            {
                targetNode.m_firstContinuationIndex = range_cast<uint16_t>(curContinuationIndex);
                CopyContinuations(node, curContinuationIndex, ret->m_continuationIndices);
            }
        }

        // return the compiled tables
        return std::move(ret);
    }

} // prv

END_BOOMER_NAMESPACE_EX(parser)
