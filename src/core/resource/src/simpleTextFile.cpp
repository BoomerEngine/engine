/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: text #]
***/

#include "build.h"
#include "simpleTextFile.h"
#include "core/containers/include/stringBuilder.h"
#include "core/containers/include/inplaceArray.h"

BEGIN_BOOMER_NAMESPACE_EX(res)

namespace text
{

    //--

    void ParameterNode::reportError(const char* txt) const
    {
        if (m_fileName)
        {
            TRACE_ERROR("{}({}): {}", m_fileName, m_line, txt);
        }
        else
        {
            TRACE_ERROR("Error at line {}: {}", m_children, txt);
        }
    }


    //--

    ParameterFile::ParameterFile(LinearAllocator& mem)
        : m_mem(mem)
        , m_nodes(nullptr)
    {}

    ParameterFile::~ParameterFile()
    {}

    ParameterNodeIterator ParameterFile::nodes() const
    {
        return ParameterNodeIterator(m_nodes);
    }

    //--

    IParameterFileParserErrorReporter::~IParameterFileParserErrorReporter()
    {}

    class DefaultParameterFileParserErrorReporter : public IParameterFileParserErrorReporter
    {
    public:
        virtual void reportError(StringView context, uint32_t line, StringView txt) override final
        {
            if (context)
            {
                TRACE_ERROR("{}({}): error: {}", context, line, txt);
            }
            else
            {
                TRACE_ERROR("Error at line {}: {}", line, txt);
            }
        }
    };

    IParameterFileParserErrorReporter& IParameterFileParserErrorReporter::GetDefault()
    {
        static DefaultParameterFileParserErrorReporter theErrorReporter;
        return theErrorReporter;
    }

    //--

    class ParameterFileParser : public NoCopy
    {
    public:
        ParameterFileParser(ParameterFile& outFile, IParameterFileParserErrorReporter* errorReporter, StringView data, StringView contextName)
            : m_file(outFile)
            , m_error(errorReporter)
            , m_cur(data.data())
            , m_end(data.data() + data.length())
            , m_currentLine(0)
        {
            outFile.m_nodes = nullptr;
            outFile.m_mem.clear();

            m_contextName = copyString(contextName);
            m_lastTopNode = nullptr;
        }

        bool eatLine(int& outIdent, StringView& outLine)
        {
            int indent = 0;
            while (m_cur < m_end)
            {
                auto ch = *m_cur;

                // ignore the other line ending
                if (ch == '\r')
                {
                    m_cur += 1;
                    if (m_cur < m_end && m_cur[0] == '\n') m_cur += 1;
                    return false;
                }
                else if (ch == '\n')
                {
                    m_cur += 1;
                    if (m_cur < m_end && m_cur[0] == '\r') m_cur += 1;
                    return false;
                }

                // comment detected, skip till the end of the line
                else if (ch == '#')
                {
                    while (m_cur < m_end)
                        if (*m_cur++ == '\n')
                            break;

                    return false;
                }

                // is this a non-ws char ?
                else if (ch > ' ')
                {
                    outIdent = indent;
                    const char* lineStart = m_cur;
                    const char* lineEnd = nullptr;

                    while (m_cur < m_end)
                    {
                        if (*m_cur == '#')
                        {
                            if (lineEnd == nullptr)
                                lineEnd = m_cur;
                        }
                        else if (*m_cur == '\n' || *m_cur == '\r')
                        {
                            if (lineEnd == nullptr)
                                lineEnd = m_cur;
                            ++m_cur;
                            break;
                        }

                        ++m_cur;
                    }

                    if (lineEnd == nullptr)
                        lineEnd = m_end;

                    outLine = StringView(lineStart, lineEnd);
                    return true;
                }

                // advance
                m_cur += 1;
                indent += 1;
            }

            // end of line reached, no indentation found
            return false;
        }

        ParameterNode* prepareNode(int currentIndent)
        {
            // close all existing nodes up to the given indentation level
            for (int i = m_stack.lastValidIndex(); i >= 0; --i)
            {
                auto& entry = m_stack[i];
                if (entry.m_indentation < currentIndent)
                    break; // node does not have to be closed
                m_stack.popBack();
            }

            // create new node
            if (currentIndent < 0)
                return nullptr;

            // create a new node
            auto newNode  = m_file.m_mem.create<ParameterNode>();
            newNode->m_line = m_currentLine;
            newNode->m_fileName = m_contextName;

            // link with previous node on the stack
            if (m_stack.empty())
            {
                if (m_lastTopNode)
                    m_lastTopNode->m_next = newNode;
                else
                    m_file.m_nodes = newNode;
                m_lastTopNode = newNode;
            }
            else
            {
                auto& top = m_stack.back();
                if (top.m_lastChild)
                    top.m_lastChild->m_next = newNode;
                else
                    top.m_node->m_children = newNode;
                top.m_lastChild = newNode;
                top.m_node->m_numChildren += 1;
            }

            // add entry to stack
            auto& stackEntry = m_stack.emplaceBack();
            stackEntry.m_node = newNode;
            stackEntry.m_lastChild = nullptr;
            stackEntry.m_lastToken = nullptr;
            stackEntry.m_indentation = currentIndent;
            return newNode;
        }

        bool eatWhitespaces(const char*& cur, const char* end) const
        {
            auto pos  = cur;
            while (pos < end && *pos)
            {
                ASSERT(*pos != '\n');
                if (*pos > ' ')
                {
                    cur = pos;
                    return true;
                }

                ++pos;
            }

            return false;
        }

        bool eatEquals(const char*& cur, const char* end) const
        {
            auto pos  = cur;

            if (!eatWhitespaces(pos, end))
                return false;

            if (pos < end && *pos != '=')
                return false;

            cur = pos + 1;
            return true;
        }

        /*char* makeString(char* from, char* to) const
        {
            if (to <= from)
                return (char*)"";

            if (to == m_end || *to > ' ') // we can't write to this memory
            {
                auto length = to - from;
                auto mem  = (char*)m_file.m_mem.alloc(length + 1, 1);
                memcpy(mem, from, length);
                mem[length] = 0;
                return mem;
            }
            else
            {
                *to = 0;
                return from;
            }
        }*/

        StringView eatToken(const char*& cur, const char* end) const
        {
            // eat initial white spaces
            if (!eatWhitespaces(cur, end))
                return nullptr; // we've reached end of line

            // a string ?
            if (*cur == '\"')
            {
                auto start  = cur;
                cur += 1;

                // parse till the end of string
                while (*cur != 0)
                {
                    if (*cur == '\"')
                    {
                        cur += 1;
                        return StringView(start + 1, cur - 1);
                    }
                    ++cur;
                }

                // no string
                reportError("Unexpected end of string");
                return nullptr;
            }
            else
            {
                auto start  = cur;

                while (*cur > ' ' && *cur != 0 && *cur != '=')
                    ++cur;

                if (*cur != 0 && *cur <= ' ')
                {
                    cur += 1;
                    return StringView(start, cur - 1);
                }
                else
                {
                    return StringView(start, cur);
                }
            }
        }

        void appendToken(StringView str)
        {
            auto& currentNode = m_stack.back();

            auto token  = m_file.m_mem.create<ParameterToken>();
            token->next = nullptr;
            token->value = str;

            if (currentNode.m_lastToken)
                currentNode.m_lastToken->next = token;
            else
                currentNode.m_node->m_tokens = token;
            currentNode.m_lastToken = token;
            currentNode.m_node->m_numTokens += 1;
        }

        void appendKeyValuePair(StringView key, StringView value)
        {
            auto& currentNode = m_stack.back();

            auto valueToken  = m_file.mem().create<ParameterToken>();
            valueToken->next = nullptr;
            valueToken->value = value;

            auto keyToken  = m_file.mem().create<ParameterToken>();
            keyToken->next = valueToken;
            keyToken->value = key;

            auto childNode  = m_file.mem().create<ParameterNode>();
            childNode->m_line = currentNode.m_node->m_line;
            childNode->m_fileName = currentNode.m_node->m_fileName;
            childNode->m_tokens = keyToken;
            childNode->m_numTokens = 2;

            childNode->m_next = nullptr;
            if (currentNode.m_lastChild)
                currentNode.m_lastChild->m_next = childNode;
            else
                currentNode.m_node->m_children = childNode;
            currentNode.m_lastChild = childNode;
            currentNode.m_node->m_numChildren += 1;
        }

        bool parse()
        {
            // process the whole file
            while (m_cur < m_end)
            {
                // advance line
                m_currentLine += 1;

                // determine current indentation
                int indent = -1;
                StringView fullLine;
                if (!eatLine(indent, fullLine))
                    continue;

                // prepare stack for the current indentation, this will open/close nodes
                // NOTE: this may fail if something is wrong with the indentations
                auto node  = prepareNode(indent);
                if (!node)
                    return false;

                // parse the tokens
                const char* cur = fullLine.data();
                const char* end = cur + fullLine.length();
                while (cur < end)
                {
                    // eat token string
                    auto tokenStr = eatToken(cur, end);
                    if (!tokenStr)
                        break;

                    // is this a key=value sugar
                    if (eatEquals(cur, end))
                    {
                        auto valueStr = eatToken(cur, end);
                        if (!valueStr)
                        {
                            reportError(TempString("Missing value for '{}'", tokenStr));
                            break;
                        }

                        // create a child node
                        appendKeyValuePair(tokenStr, valueStr);
                    }
                    else
                    {
                        // append as a token
                        appendToken(tokenStr);
                    }

                }
            }

            // close all nodes
            prepareNode(-1);

            // we are done
            return true;
        }

        StringView copyString(StringView txt)
        {
            if (txt == nullptr)
                return nullptr;

            auto length = txt.length();
            auto mem  = (char*)m_file.m_mem.alloc(length + 1, 1);
            memcpy(mem, txt.data(), length);
            mem[length] = 0; // since we are making a copy make it zero terminated for good measure and easier debugging
            return StringView(mem, txt.length());
        }

        void reportError(StringView txt) const
        {
            if (m_error)
                m_error->reportError(m_contextName, m_currentLine, txt);
        }

    private:
        ParameterFile& m_file;
        StringView m_contextName;
        IParameterFileParserErrorReporter* m_error;
        const char* m_cur;
        const char* m_end;
        uint32_t m_currentLine;

        struct Entry
        {
            ParameterNode* m_node;
            ParameterNode* m_lastChild;
            ParameterToken* m_lastToken;
            int m_indentation;
        };

        InplaceArray<Entry, 16> m_stack;
        ParameterNode* m_lastTopNode;
    };

    bool ParseParameters(StringView data, ParameterFile& outFile, StringView context, IParameterFileParserErrorReporter* errorReporter)
    {
        // use the default error reporter if not specified
        if (!errorReporter)
            errorReporter = &IParameterFileParserErrorReporter::GetDefault();

        // prepare parser
        ParameterFileParser parser(outFile, errorReporter, data, context);
        return parser.parse();
    }

} // text

END_BOOMER_NAMESPACE_EX(res)
