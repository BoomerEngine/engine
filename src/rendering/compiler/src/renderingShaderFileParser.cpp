/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\parser #]
***/

#include "build.h"
#include "renderingShaderDataValue.h"
#include "renderingShaderCodeNode.h"
#include "renderingShaderProgram.h"
#include "renderingShaderProgramInstance.h"
#include "renderingShaderTypeLibrary.h"
#include "renderingShaderTypeUtils.h"
#include "renderingShaderFunction.h"
#include "renderingShaderFileParser.h"
#include "renderingShaderCOdeLibrary.h"

#include "base/parser/include/textLanguageDefinition.h"
#include "base/parser/include/textSimpleLanguageDefinition.h"
#include "base/parser/include/textErrorReporter.h"
#include "base/parser/include/textToken.h"

#include "renderingShaderFileParserHelper.h"
#include "renderingShaderFileStructureParser_Symbols.h"
//#include "renderingShaderFileCodeParser_Symbols.h"

extern int cslf_parse(rendering::shadercompiler::parser::ParsingFileContext& context, rendering::shadercompiler::parser::ParserTokenStream& tokens);
extern int cslc_parse(rendering::shadercompiler::parser::ParsingCodeContext& context, rendering::shadercompiler::parser::ParserCodeTokenStream& tokens);

BEGIN_BOOMER_NAMESPACE(rendering::shadercompiler)

//--

AttributeList& AttributeList::clear()
{
    attributes.reset();
    return *this;
}

AttributeList& AttributeList::add(base::StringID key, base::StringView txt)
{
    attributes[key] = base::StringBuf(txt);
    return *this;
}

AttributeList& AttributeList::merge(const AttributeList& attr)
{
    for (const auto& pair : attr.attributes.pairs())
        add(pair.key, pair.value);
    return *this;
}

bool AttributeList::has(base::StringID key) const
{
    return attributes.contains(key);
}

base::StringView AttributeList::value(base::StringID key) const
{
    if (const auto* val = attributes.find(key))
        return val->view();
    return "";
}

base::StringView AttributeList::valueOrDefault(base::StringID key, base::StringView defaultValue/*=""*/) const
{
    if (const auto* val = attributes.find(key))
        return val->view();
    return defaultValue;
}

int AttributeList::valueAsIntOrDefault(base::StringID key, int defaultValue) const
{
    int ret = defaultValue;
    if (const auto* val = attributes.find(key))
        val->view().match(ret);
    return ret;
}

void AttributeList::calcTypeHash(base::CRC64& crc) const
{
    for (const auto& key : attributes.keys())
        crc << key;
    for (const auto& value : attributes.values())
        crc << value;
}

void AttributeList::print(base::IFormatStream& f) const
{
    if (!attributes.empty())
    {
        f << "attributes(";
        for (uint32_t i = 0; i < attributes.keys().size(); ++i)
        {
            if (i > 0)
                f << ",";
            f << attributes.keys()[i];

            if (attributes.values()[i])
                f << "=" << attributes.values()[i];
        }
        f << ")";
    }
}

//--


namespace parser
{

    //----------

    RTTI_BEGIN_TYPE_ENUM(ElementFlag);
        //RTTI_ENUM_OPTION(Override);
        RTTI_ENUM_OPTION(In);
        RTTI_ENUM_OPTION(Out);
        RTTI_ENUM_OPTION(Const);
        RTTI_ENUM_OPTION(Vertex);
        RTTI_ENUM_OPTION(Export);
    RTTI_END_TYPE();

    //----------

    class LanguageDefinition : public base::ISingleton
    {
        DECLARE_SINGLETON(LanguageDefinition);

    public:
        LanguageDefinition()
        {
            base::parser::SimpleLanguageDefinitionBuilder defs;

            //auto IdentFirstChars  = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01"; // NOTE: added 01 for swizzles
            //auto IdentNextChars  = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
            //defs.identChars(IdentFirstChars, IdentNextChars);

            defs.enableIntegerNumbers(true);
            defs.enableFloatNumbers(true);
            defs.enableStrings(true);
            defs.enableNames(true);
            defs.enableStringsAsNames(true);

            defs.addKeyword("in", TOKEN_IN);
            defs.addKeyword("out", TOKEN_OUT);
            defs.addKeyword("break", TOKEN_BREAK);
            defs.addKeyword("continue", TOKEN_CONTINUE);
            defs.addKeyword("return", TOKEN_RETURN);
            defs.addKeyword("discard", TOKEN_DISCARD);
            defs.addKeyword("do", TOKEN_DO);
            defs.addKeyword("while", TOKEN_WHILE);
            defs.addKeyword("if", TOKEN_IF);
            defs.addKeyword("else", TOKEN_ELSE);
            defs.addKeyword("for", TOKEN_FOR);
            defs.addKeyword("attribute", TOKEN_ATTRIBUTE);
            defs.addKeyword("shared", TOKEN_SHARED);
            defs.addKeyword("export", TOKEN_EXPORT);
            defs.addKeyword("vertex", TOKEN_VERTEX);
            defs.addKeyword("shader", TOKEN_SHADER);                    
            defs.addKeyword("struct", TOKEN_STRUCT);
            defs.addKeyword("const", TOKEN_CONST);
            defs.addKeyword("descriptor", TOKEN_DESCRIPTOR);
			defs.addKeyword("sampler", TOKEN_STATIC_SAMPLER);
			defs.addKeyword("state", TOKEN_RENDER_STATES);

            defs.addKeyword("this", TOKEN_THIS);
            defs.addKeyword("true", TOKEN_BOOL_TRUE);
            defs.addKeyword("false", TOKEN_BOOL_FALSE);

            defs.addKeyword("ConstantBuffer", TOKEN_CONSTANTS);

            defs.addKeyword("+=", TOKEN_ADD_ASSIGN);
            defs.addKeyword("-=", TOKEN_SUB_ASSIGN);
            defs.addKeyword("*=", TOKEN_MUL_ASSIGN);
            defs.addKeyword("/=", TOKEN_DIV_ASSIGN);
            defs.addKeyword("&=", TOKEN_AND_ASSIGN);
            defs.addKeyword("|=", TOKEN_OR_ASSIGN);
            defs.addKeyword("||", TOKEN_OR_OP);
            defs.addKeyword("&&", TOKEN_AND_OP);
            defs.addKeyword("!=", TOKEN_NE_OP);
            defs.addKeyword("==", TOKEN_EQ_OP);
            defs.addKeyword(">=", TOKEN_GE_OP);
            defs.addKeyword("<=", TOKEN_LE_OP);
            defs.addKeyword("<<", TOKEN_LEFT_OP);
            defs.addKeyword(">>", TOKEN_RIGHT_OP);
            defs.addKeyword("<<=", TOKEN_LEFT_ASSIGN);
            defs.addKeyword(">>=", TOKEN_RIGHT_ASSIGN);
            defs.addKeyword("++", TOKEN_INC_OP);
            defs.addKeyword("--", TOKEN_DEC_OP);
			defs.addKeyword("[[", TOKEN_ATTR_START);
			defs.addKeyword("]]", TOKEN_ATTR_END);

            defs.addChar('(');
            defs.addChar(')');
            defs.addChar('{');
            defs.addChar('}');
            defs.addChar('[');
            defs.addChar(']');
            defs.addChar('<');
            defs.addChar('>');
            defs.addChar(',');
            defs.addChar('=');
            defs.addChar(';');
            defs.addChar('&');
            defs.addChar('|');
            defs.addChar('^');
            defs.addChar('+');
            defs.addChar('-');
            defs.addChar('*');
            defs.addChar('/');
            defs.addChar('%');
            defs.addChar('.');
            defs.addChar('!');
            defs.addChar('?');
            defs.addChar(':');
            defs.addChar('~');

            m_language = defs.buildLanguageDefinition();
        }

        INLINE const base::parser::ILanguageDefinition& definitions() const
        {
            return *m_language;
        }

    private:
        base::UniquePtr<base::parser::ILanguageDefinition> m_language;

        virtual void deinit() override
        {
            m_language.reset();
        }
    };

    //----------

    TypeReference::TypeReference()
    {}

    TypeReference::TypeReference(const base::parser::Location& loc, base::StringView name)
        : location(loc)
        , name(name)
    {}

    //----------

    Element::Element(const base::parser::Location& loc, ElementType type, base::StringView name, ElementFlags flags)
        : type(type)
        , location(loc)
        , name(name)
        , flags(flags)
    {}

    AttributeList Element::gatherAttributes() const
    {
        AttributeList ret;

        for (auto attr  : attributes)
        {
            if (attr->name)
                ret.add(base::StringID(attr->name), attr->stringData);
        }

        return ret;
    }

    const Element* Element::findAttribute(base::StringView name) const
    {
        for (auto attr  : attributes)
            if (attr->name == name)
                return attr;

        return nullptr;
    }

    //----------

    static bool ResolveType(base::parser::Token**& stream, base::parser::Token** streamEnd, parser::CodeParsingNode& outNode, const CodeLibrary& lib, base::parser::IErrorReporter& err)
    {
            if (stream[0]->isIdentifier() || stream[0]->isKeyword())
        {
            int size = 1;

            parser::TypeReference typeRef;
            typeRef.location = stream[0]->location();
            typeRef.name = stream[0]->view();

            // some types must be resolved when referenced
            bool mustResolve = false;

            // eat the layout name
            if (stream + 4 < streamEnd)
            {
                // eat program types
                if (stream[0]->view() == "shader")
                {
                    mustResolve = true;
                    if (stream[1]->view() == "<" && stream[2]->isIdentifier() && stream[3]->view() == ">")
                    {
                        typeRef.innerType = stream[2]->view();
                        size += 3;
                    }
                }

                // eat array sizes
                while (stream + size + 3 <= streamEnd && stream[size]->view() == "[")
                {
                    if (stream[size + 1]->isInteger() && stream[size + 2]->view() == "]")
                    {
                        typeRef.arraySizes.pushBack((int)stream[size + 1]->floatNumber());
                        size += 3;
                    }
                    else if (stream[size + 1]->view() == "]")
                    {
                        typeRef.arraySizes.pushBack(-1);
                        size += 2;
                    }
                    else
                    {
                        break;
                    }
                }

                // resolve the type
                DataType type;
                if (lib.resolveType(&typeRef, type, err, mustResolve))
                {
                    // setup node
                    outNode.m_location = stream[0]->location();
                    outNode.m_string = stream[0]->view();
                    outNode.m_type = type;

                    // determine token type based on the resolved type
                    if (type.isProgram())
                        outNode.m_tokenID = TOKEN_SHADER_TYPE;
                    else if (type.isArray())
                        outNode.m_tokenID = TOKEN_ARRAY_TYPE;
                    else if (type.isVector())
                        outNode.m_tokenID = TOKEN_VECTOR_TYPE;
					else if (type.isMatrix())
						outNode.m_tokenID = TOKEN_MATRIX_TYPE;
                    else if ((type.isNumericalScalar() || type.isBoolean()) && !type.isArray())
                        outNode.m_tokenID = TOKEN_CASTABLE_TYPE;
                    else if (type.isComposite())
                        outNode.m_tokenID = TOKEN_STRUCT_TYPE;
                    else
                        return false;

                    // advance token stream
                    stream += size;
                    return true;
                }
            }
        }

        // not a type token
        return false;
    }

    //----------

    const base::parser::ILanguageDefinition& GetShaderLanguageDefinition()
    {
        return parser::LanguageDefinition::GetInstance().definitions();
    }


	static void CheckChildUniqness(const CodeNode* node, base::HashSet<const CodeNode*>& allNodes)
	{
		ASSERT_EX(!allNodes.contains(node), "Node is in two places in hierarchy");
		allNodes.insert(node);

		for (const auto* child : node->children())
			CheckChildUniqness(child, allNodes);
	}

	static void CheckNoLeftOvers(const CodeNode* node)
	{
		ASSERT_EX(node->opCode() != OpCode::ListElement, "This opcode should be long gone");
		ASSERT_EX(node->extraData().m_nextStatement == nullptr, "Statements should be reconciled");

		for (const auto* child : node->children())
			CheckNoLeftOvers(child);
	}

	//----------

	static void ConvertTokens(parser::ParsingCodeContext& ctx, const CodeLibrary& lib, const base::Array<base::parser::Token*>& tokens, base::Array<parser::CodeParsingNode>& outTokens, base::parser::IErrorReporter& err)
	{
		auto stream = (base::parser::Token**) tokens.typedData();
		auto streamEnd = stream + tokens.size();
		while (stream < streamEnd)
		{
			// type name ?
			parser::CodeParsingNode token;
			if (ResolveType(stream, streamEnd, token, lib, err))
			{
				// add type token to the stream
				outTokens.pushBack(token);
			}
			else
			{
				// simple token
				auto& token = *stream[0];
				auto& newToken = outTokens.emplaceBack();
				newToken.m_tokenID = token.keywordID();
				newToken.m_location = token.location();
				newToken.m_string = token.view();

				// fill info
				if (token.isString())
				{
					newToken.m_tokenID = TOKEN_STRING;
				}
				else if (token.isName())
				{
					newToken.m_tokenID = TOKEN_NAME;
					newToken.m_code = ctx.alloc<CodeNode>(token.location(), DataType::NameScalarType(), DataValue(base::StringID(token.string())));
				}
				else if (token.isFloat())
				{
					newToken.m_tokenID = TOKEN_FLOAT_NUMBER;
					newToken.m_code = ctx.alloc<CodeNode>(token.location(), DataType::FloatScalarType(), DataValue((float)token.floatNumber()));
				}
				else if (token.isInteger())
				{
					newToken.m_tokenID = TOKEN_INT_NUMBER;
					newToken.m_code = ctx.alloc<CodeNode>(token.location(), DataType::IntScalarType(), DataValue((int)token.floatNumber()));
				}
				else if (token.isKeyword() && token.keywordID() == TOKEN_BOOL_TRUE)
				{
					newToken.m_tokenID = TOKEN_BOOL_TRUE;
					newToken.m_code = ctx.alloc<CodeNode>(token.location(), DataType::BoolScalarType(), DataValue(true));
				}
				else if (token.isKeyword() && token.keywordID() == TOKEN_BOOL_FALSE)
				{
					newToken.m_tokenID = TOKEN_BOOL_FALSE;
					newToken.m_code = ctx.alloc<CodeNode>(token.location(), DataType::BoolScalarType(), DataValue(false));
				}
				else if (token.isIdentifier())
				{
					newToken.m_tokenID = TOKEN_IDENT;
				}
				else if (token.isChar())
				{
					newToken.m_tokenID = token.ch();
				}
				else
				{
					newToken.m_tokenID = token.keywordID();
				}

				stream += 1;
			}
		}

		// HACK
		{
			auto& newToken = outTokens.emplaceBack();
			newToken.m_tokenID = ';';
			newToken.m_location = outTokens.back().m_location;
			newToken.m_string = ";";
		}
	}

    //----------

    CodeNode* AnalyzeShaderCode(
        base::mem::LinearAllocator& mem,
        base::parser::IErrorReporter& errHandler,
        const base::Array<base::parser::Token*>& tokens,
        const CodeLibrary& lib, const Function* contextFunction, const Program* contextProgram)
    {
        // nothing to compile
        if (tokens.empty())
            return mem.create<CodeNode>(base::parser::Location(), OpCode::Nop);

        // setup the context
        parser::ParsingCodeContext ctx(mem, errHandler, lib, contextFunction, contextProgram);

		// create tokens that can be parsed by the function parser
		base::Array<parser::CodeParsingNode> newTokens;
		ConvertTokens(ctx, lib, tokens, newTokens, errHandler);

        // parse the shit
		parser::ParserCodeTokenStream stream(newTokens);
		auto ret = cslc_parse(ctx, stream);
        if (ret != 0)
            return nullptr;

        // no code compiled
		auto* rootNode = ctx.root();
        if (rootNode == nullptr)
        {
            if (!tokens.empty())
                errHandler.reportError(tokens[0]->location(), "No code generated");
            return nullptr;
        }

        // root node must be a scope node
        if (rootNode->opCode() != OpCode::Scope && contextFunction)
        {
            auto scopeNode = lib.allocator().create<CodeNode>(rootNode->location(), OpCode::Scope);
            scopeNode->addChild(rootNode);
            rootNode = scopeNode;
        }

		// make sure there are no leftovers in the code
		CheckNoLeftOvers(rootNode);

		// make sure nodes are not double-linked
		{
			base::HashSet<const CodeNode*> uniqueNodes;
			CheckChildUniqness(rootNode, uniqueNodes);
		}

        // link the scopes
        CodeNode::LinkScopes(rootNode, nullptr);

        // return code
        return rootNode;
    }

    //----------

    Element* AnalyzeShaderFile(base::mem::LinearAllocator& mem, base::parser::IErrorReporter& errHandler, base::parser::TokenList& tokens)
    {
        // create context
        ParsingNode result;
        ParserTokenStream stream(tokens);
        ParsingFileContext ctx(mem, errHandler, result);

        // run though the bison parser to get the structure
        auto ret = cslf_parse(ctx, stream);
        if (ret != 0)
        {
            if (!tokens.empty())
                errHandler.reportError(tokens.head()->location(), "Failed to parse shader file");
            return nullptr;
        }

        // no structure compiled
        return result.element;
    }

    //----------

} // parser

END_BOOMER_NAMESPACE(rendering::shadercompiler)