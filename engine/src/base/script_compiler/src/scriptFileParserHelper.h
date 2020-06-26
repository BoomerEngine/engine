/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#pragma once

#include "scriptLibrary.h"
#include "base/memory/include/linearAllocator.h"
#include "base/parser/include/textToken.h"

namespace base
{
    namespace script
    {

        //---

        /// parser node
        struct FileParsingNode
        {
            int tokenID = -1;
            base::parser::Location location;
            base::StringView<char> stringValue;
            base::Array<base::parser::Token*> tokens;
            StubTypeDecl* typeDecl = nullptr;
            StubTypeRef* typeRef = nullptr;
            StubConstantValue* constValue = nullptr;
            StubFlags flags;
            StringID name;
            StringID nameEx;
            double floatValue = 0.0f;
            int64_t intValue = 0;
            uint64_t uintValue = 0U;
            Array<StringID> names;

            INLINE FileParsingNode()
            {}

            INLINE FileParsingNode(const base::parser::Location& loc, base::StringView<char> txt)
            {
                stringValue = txt;
                location = loc;
            }

            INLINE FileParsingNode(const base::parser::Location& loc, double val)
            {
                floatValue = val;
                location = loc;
            }

            INLINE FileParsingNode(const base::parser::Location& loc, int64_t val)
            {
                intValue = val;
                location = loc;
            }

            INLINE FileParsingNode(const base::parser::Location& loc, uint64_t val)
            {
                uintValue = val;
                location = loc;
            }
        };

        //---

        /// token reader
        class FileParsingTokenStream : public base::NoCopy
        {
        public:
            FileParsingTokenStream(parser::TokenList&& tokens);

            /// read a token from the stream (used as yylex)
            int readToken(FileParsingNode& outNode);

            /// extract inner tokens
            void extractInnerTokenStream(char delimiter, base::Array<base::parser::Token*>& outList);

            //--

            INLINE const parser::Location& location() const { return m_lastTokenLocation; }
            INLINE StringView<char> text() const { return m_lastTokenText; }

        private:
            parser::TokenList m_tokens;

            StringView<char> m_lastTokenText;
            parser::Location m_lastTokenLocation;
        };

        //---

        class FileParser;

        /// helper class for parsing file content
        class FileParsingContext : public base::NoCopy
        {
        public:
            FileParsingContext(FileParser& fileParser, const StubFile* file, const StubModule* module);

            StubFlags globalFlags;
            StubFunction* currentFunction = nullptr;
            Array<Stub*> currentObjects;
            Array<StubConstantValue*> currentConstantValue;

            //--

            StubLocation mapLocation(const parser::Location& location);

            StubClass* beginCompound(const parser::Location& location, StringID name, const StubFlags& flags);
            StubEnum* beginEnum(const parser::Location& location, StringID name, const StubFlags& flags);
            void endObject();

            StubFunction* addFunction(const parser::Location& location, StringID name, const StubFlags& flags);
            StubProperty* addVar(const parser::Location& location, StringID name, const StubTypeDecl* typeDecl, const StubFlags& flags);
            StubEnumOption* addEnumOption(const parser::Location& location, StringID name, bool hasValue = false, int64_t value = 0);
            StubConstant* addConstant(const parser::Location& location, StringID name);
            StubFunctionArg* addFunctionArg(const parser::Location& location, StringID name, const StubTypeDecl* typeDecl, const StubFlags& flags);

            StubModuleImport* createModuleImport(const parser::Location& location, StringID name);
            StubTypeName* createTypeName(const parser::Location& location, StringID name, const StubTypeDecl* decl);
            StubTypeRef* createTypeRef(const parser::Location& location, StringID name);

            StubTypeDecl* createEngineType(const parser::Location& location, StringID engineTypeAlias);
            StubTypeDecl* createSimpleType(const parser::Location& location, const StubTypeRef* classTypeRef);
            StubTypeDecl* createPointerType(const parser::Location& location, const StubTypeRef* classTypeRef);
            StubTypeDecl* createWeakPointerType(const parser::Location& location, const StubTypeRef* classTypeRef);
            StubTypeDecl* createClassType(const parser::Location& location, const StubTypeRef* classTypeRef);
            StubTypeDecl* createStaticArrayType(const parser::Location& location, const StubTypeDecl* innerType, uint32_t arraySize);
            StubTypeDecl* createDynamicArrayType(const parser::Location& location, const StubTypeDecl* innerType);

            StubConstantValue* createConstValueInt(const parser::Location& location, int64_t value);
            StubConstantValue* createConstValueUint(const parser::Location& location, uint64_t value);
            StubConstantValue* createConstValueFloat(const parser::Location& location, double value);
            StubConstantValue* createConstValueBool(const parser::Location& location, bool value);
            StubConstantValue* createConstValueString(const parser::Location& location, StringView<char> view);
            StubConstantValue* createConstValueName(const parser::Location& location, StringID name);
            StubConstantValue* createConstValueCompound(const parser::Location& location, const StubTypeDecl* typeRef);

            Stub* contextObject();

            void reportError(const parser::Location& location, StringView<char> message);
            void reportWarning(const parser::Location& location, StringView<char> message);

        private:
            FileParser& m_parser;
            const StubFile* m_file;
            const StubModule* m_module;

            HashMap<const Stub*, HashMap<StringID, StubTypeRef*>> m_typeReferences;
            HashMap<StringID, StubModuleImport*> m_localImports;
        };

        //---


    } // script
} // base