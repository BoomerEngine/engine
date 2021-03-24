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
#include "core/memory/include/linearAllocator.h"
#include "core/containers/include/hashMap.h"

BEGIN_BOOMER_NAMESPACE()

namespace prv
{
    //---

    struct ParsingNode;

    // helper temporary node used when building the parser tree
    struct ParsingNodeRef
    {
        ParsingNode* m_node;

        //--

        INLINE ParsingNodeRef()
            : m_node(nullptr)
        {}
    };

    // helper temporary node used when building the parser tree
    struct ParsingNode
    {
        // assigned index of this node
        uint32_t m_index;

        // type of stuff to emit, if TextTokenType::Invalid this is not an emit node
        // NOTE: we can be an emit node even if we have a "continuation table", it just means that we may match to a longer token as well:
        // example "func" may be an good match for IDENTIFIER but a "function" will be a match for KEYWORD, etc
        TextTokenType m_possibleEmit;

        // in case we are emitting a keyword this is the ID of the keyword
        int m_keywordID;

        // continuations
        ParsingNodeRef m_continuation[128];

        //--

        INLINE ParsingNode()
            : m_possibleEmit(TextTokenType::Invalid)
            , m_keywordID(-1)
        {}

    };

    //---

    // helper tree builder
    // NOTE: this may take some memory
    class ParsingTreeBuilder : public NoCopy
    {
    public:
        ParsingTreeBuilder();
        ~ParsingTreeBuilder();

        // insert a match for identifiers
        void insertIdentifiersMatch(const char* firstChars, const char* nextChars);

        // insert a match for keyword
        void insertKeywordMatch(const char* txt, int keywordID);

        // insert a match for single character
        void insertCharMatch(char ch);

        // insert a match for double quoted string
        void insertStringMatch(bool asNames = false);

        // insert a match for single quoted string
        void insertNameMatch(bool asStrings = false);

        // insert a match for integer numbers (no dot)
        void insertIntegerMatch();

        // insert a match for floating point number
        void insertFloatingPointMatch();

        //--

        // dump structure to a string buffer for debugging
        void print(IFormatStream& str) const;

        //--

        // create a language definition from the parser tree
        UniquePtr<ITextLanguageDefinition> buildLanguageDefinition() const;

    private:
        LinearAllocator m_mem;
        ParsingNode* m_root;
        Array<ParsingNode*> m_nodes;
        HashMap<int, StringBuf> m_debugKeywordMap;

        //--

        void insertUniqueMatchNode(ParsingNode* atNode, const char* chars, Array<ParsingNode*>& outContinuationNodes);
        ParsingNode* insertUniqueMatchNode(ParsingNode* atNode, char ch);
        ParsingNode* uniqueMatchNode(ParsingNode* atNode, const char* chars);
        ParsingNode* duplicateNode(ParsingNode* node);
        void makeFinalNode(ParsingNode* node);
        void spinOnSelf(ParsingNode* node);
        void spinOnSelf(ParsingNode* node, const char* spinChars);
        void spinOnSelf(ParsingNode* node, char spinChar);
    };

    //---

} // prv

END_BOOMER_NAMESPACE()
