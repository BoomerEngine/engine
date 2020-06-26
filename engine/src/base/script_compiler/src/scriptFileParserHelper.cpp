/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"
#include "scriptLibrary.h"
#include "scriptFileParser.h"
#include "scriptFileParserHelper.h"
#include "scriptFileStructureParser_Symbols.h"

namespace base
{
    namespace script
    {
        //---

        FileParsingTokenStream::FileParsingTokenStream(parser::TokenList&& tokens)
            : m_tokens(std::move(tokens))
        {}

        int FileParsingTokenStream::readToken(FileParsingNode& outNode)
        {
            // end of stream
            if (!m_tokens.empty())
                return 0;

            // get the token
            auto token  = m_tokens.popFront();
            m_lastTokenText = token->view();
            m_lastTokenLocation = token->location();

            // fill info
            if (token->isString())
            {
                outNode.tokenID = TOKEN_STRING;
                outNode.stringValue = token->view();
            }
            else if (token->isName())
            {
                outNode.tokenID = TOKEN_NAME;
                outNode.stringValue = token->view();
                outNode.name = StringID(token->view());
            }
            else if (token->isFloat())
            {
                outNode.tokenID = TOKEN_FLOAT_NUMBER;
                outNode.floatValue = token->floatNumber();
                outNode.stringValue = token->view();
            }
            else if (token->isInteger())
            {
                outNode.tokenID = TOKEN_INT_NUMBER;
                outNode.intValue = (int64_t)token->floatNumber();
                outNode.stringValue = token->view();
            }
            else if (token->isIdentifier())
            {
                outNode.tokenID = TOKEN_IDENT;
                // TODO: type name!
                outNode.stringValue = token->view();
                outNode.name = StringID(outNode.stringValue);
            }
            else if (token->isChar())
            {
                outNode.tokenID = token->ch();
            }
            else
            {
                outNode.tokenID = token->keywordID();
            }

            outNode.location = token->location();

            return outNode.tokenID;
        }

        void FileParsingTokenStream::extractInnerTokenStream(char delimiter, base::Array<base::parser::Token*>& outList)
        {
            uint32_t level = 0;

            while (m_tokens.head())
            {
                if (m_tokens.head()->ch() == delimiter && !level)
                    if (level == 0)
                        break;

                if (m_tokens.head()->ch() == '(' || m_tokens.head()->ch() == '{' || m_tokens.head()->ch() == '[')
                    level += 1;
                else if (m_tokens.head()->ch() == ')' || m_tokens.head()->ch() == '}' || m_tokens.head()->ch() == ']')
                    level -= 1;
                
                outList.pushBack(m_tokens.popFront());
            }
        }

        //---

        FileParsingContext::FileParsingContext(FileParser& fileParser, const StubFile* file, const StubModule* module)
            : m_parser(fileParser)
            , m_file(file)
            , m_module(module)
        {
            ASSERT(file != nullptr);
            ASSERT(file->owner != nullptr);
            ASSERT(file->owner->stubType == StubType::Module);
            currentObjects.pushBack(const_cast<Stub*>(file->owner));
            currentObjects.pushBack(const_cast<StubFile*>(file));
        }

        void FileParsingContext::reportError(const parser::Location& location, StringView<char> message)
        {
            m_parser.errorHandler().reportError(mapLocation(location).file->absolutePath, location.line(), message);
        }

        void FileParsingContext::reportWarning(const parser::Location& location, StringView<char> message)
        {
            m_parser.errorHandler().reportWarning(mapLocation(location).file->absolutePath, location.line(), message);
        }

        Stub* FileParsingContext::contextObject()
        {
            return currentObjects.back();
        }

        StubLocation FileParsingContext::mapLocation(const parser::Location& location)
        {
            StubLocation ret;
            ret.file = m_file;
            ret.line = location.line();
            return ret;
        }

        StubClass* FileParsingContext::beginCompound(const parser::Location& location, StringID name, const StubFlags& flags)
        {
            ASSERT(!name.empty());

            auto compound  = m_parser.stubs().createClass(mapLocation(location), name, contextObject());
            compound->flags = flags;

            currentObjects.pushBack(compound);
            return compound;
        }

        StubEnum* FileParsingContext::beginEnum(const parser::Location& location, StringID name, const StubFlags& flags)
        {
            ASSERT(!name.empty());

            auto enumObject  = m_parser.stubs().createEnum(mapLocation(location), name, contextObject());
            if (enumObject)
                enumObject->flags = flags;

            currentObjects.pushBack(enumObject);
            return enumObject;
        }

        void FileParsingContext::endObject()
        {
            if (currentObjects.size() > 1)
                currentObjects.popBack();
        }

        StubModuleImport* FileParsingContext::createModuleImport(const parser::Location& location, StringID name)
        {
            ASSERT(!name.empty());

            if (name == m_module->name)
            {
                reportWarning(location, TempString("Script package '{}' should not depend on itself, declaration ignored", name));
                return nullptr;
            }

            StubModuleImport* ret = nullptr;
            if (m_localImports.find(name, ret))
            {
                reportWarning(location, TempString("Script package '{}' already indicated as dependency, second definition ignored", name));
            }
            else
            {
                ret = m_parser.stubs().createModuleImport(mapLocation(location), const_cast<StubFile*>(m_file), name);
                m_localImports[name] = ret;
            }

            return ret;
        }

        StubTypeName* FileParsingContext::createTypeName(const parser::Location& location, StringID name, const StubTypeDecl* decl)
        {
            ASSERT(!name.empty());
            ASSERT(decl);

            return m_parser.stubs().createTypeAlias(mapLocation(location), name, decl, contextObject());
        }

        StubTypeRef* FileParsingContext::createTypeRef(const parser::Location& location, StringID name)
        {
            ASSERT(name);

            auto& cache = m_typeReferences[contextObject()];

            StubTypeRef* ret = nullptr;
            if (cache.find(name, ret))
                return ret;

            ret = m_parser.stubs().createTypeRef(mapLocation(location), name, contextObject());
            cache[name] = ret;

            return ret;
        }

        StubTypeDecl* FileParsingContext::createWeakPointerType(const parser::Location& location, const StubTypeRef* classTypeRef)
        {
            return m_parser.stubs().createWeakPointerType(mapLocation(location), classTypeRef);
        }

        StubTypeDecl* FileParsingContext::createPointerType(const parser::Location& location, const StubTypeRef* classTypeRef)
        {
            return m_parser.stubs().createSharedPointerType(mapLocation(location), classTypeRef);
        }

        StubTypeDecl* FileParsingContext::createClassType(const parser::Location& location, const StubTypeRef* classTypeRef)
        {
            return m_parser.stubs().createClassType(mapLocation(location), classTypeRef);
        }

        StubTypeDecl* FileParsingContext::createSimpleType(const parser::Location& location, const StubTypeRef* classTypeRef)
        {
            return m_parser.stubs().createSimpleType(mapLocation(location), classTypeRef);
        }

        StubTypeDecl* FileParsingContext::createEngineType(const parser::Location& location, StringID engineTypeAlias)
        {
            return m_parser.stubs().createEngineType(mapLocation(location), engineTypeAlias);
        }

        StubTypeDecl* FileParsingContext::createStaticArrayType(const parser::Location& location, const StubTypeDecl* innerType, uint32_t arraySize)
        {
            return m_parser.stubs().createStaticArrayType(mapLocation(location), innerType, arraySize);
        }

        StubTypeDecl* FileParsingContext::createDynamicArrayType(const parser::Location& location, const StubTypeDecl* innerType)
        {
            return m_parser.stubs().createDynamicArrayType(mapLocation(location), innerType);
        }

        StubFunction* FileParsingContext::addFunction(const parser::Location& location, StringID name, const StubFlags& flags)
        {
            ASSERT(!name.empty());

            auto owner  = contextObject();
            auto func = m_parser.stubs().createFunction(mapLocation(location), name, owner);
            func->flags = flags;

            if (!owner || owner->stubType != StubType::Class)
                func->flags |= StubFlag::Static;

            return func;
        }

        StubProperty* FileParsingContext::addVar(const parser::Location& location, StringID name, const StubTypeDecl* typeDecl, const StubFlags& flags)
        {
            ASSERT(!name.empty());
            ASSERT(typeDecl != nullptr);
            auto owner  = contextObject();

            auto var = m_parser.stubs().createProperty(mapLocation(location), name, owner);
            var->flags = flags;
            var->typeDecl = typeDecl;
            return var;
        }

        StubFunctionArg* FileParsingContext::addFunctionArg(const parser::Location& location, StringID name, const StubTypeDecl* typeDecl, const StubFlags& flags)
        {
            ASSERT(!name.empty());
            ASSERT(typeDecl != nullptr);

            if (!currentFunction)
            {
                reportError(location, "Function argument '{}' can only be added to a function");
                return nullptr;
            }

            auto var = m_parser.stubs().createFunctionArg(mapLocation(location), name, currentFunction);
            var->flags = flags;
            var->typeDecl = typeDecl;
            var->index = (short)currentFunction->args.size();

            currentFunction->args.pushBack(var);
            return var;
        }

        StubEnumOption* FileParsingContext::addEnumOption(const parser::Location& location, StringID name, bool hasValue, int64_t value)
        {
            ASSERT(!name.empty());
            auto owner  = contextObject();
            if (owner && owner->stubType == StubType::Enum)
            {
                auto enumOwner  = static_cast<StubEnum*>(owner);

                auto enumOption  = m_parser.stubs().createEnumOption(mapLocation(location), owner, name);
                enumOwner->options.pushBack(enumOption);

                if (hasValue)
                {
                    enumOption->hasUserAssignedValue = true;
                    enumOption->assignedValue = value;
                }

                return enumOption;
            }

            return nullptr;
        }

        StubConstant* FileParsingContext::addConstant(const parser::Location& location, StringID name)
        {
            ASSERT(!name.empty());
            return m_parser.stubs().createConst(mapLocation(location), name, contextObject());
        }

        StubConstantValue* FileParsingContext::createConstValueInt(const parser::Location& location, int64_t value)
        {
            return  m_parser.stubs().createConstValueInt(mapLocation(location), value);
        }

        StubConstantValue* FileParsingContext::createConstValueUint(const parser::Location& location, uint64_t value)
        {
            return  m_parser.stubs().createConstValueUint(mapLocation(location), value);
        }

        StubConstantValue* FileParsingContext::createConstValueFloat(const parser::Location& location, double value)
        {
            return  m_parser.stubs().createConstValueFloat(mapLocation(location), value);
        }

        StubConstantValue* FileParsingContext::createConstValueBool(const parser::Location& location, bool value)
        {
            return  m_parser.stubs().createConstValueBool(mapLocation(location), value);
        }

        StubConstantValue* FileParsingContext::createConstValueString(const parser::Location& location, StringView<char> view)
        {
            return  m_parser.stubs().createConstValueString(mapLocation(location), view);
        }

        StubConstantValue* FileParsingContext::createConstValueName(const parser::Location& location, StringID name)
        {
            return  m_parser.stubs().createConstValueName(mapLocation(location), name);
        }

        StubConstantValue* FileParsingContext::createConstValueCompound(const parser::Location& location, const StubTypeDecl* typeRef)
        {
            return m_parser.stubs().createConstValueCompound(mapLocation(location), typeRef);
        }

        //--

    } // script
} // base