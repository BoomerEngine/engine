/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#pragma once

#include "base/memory/include/linearAllocator.h"
#include "scriptFunctionCode.h"

namespace base
{
    namespace script
    {

        //---

        class FunctionParser;

        //---

        /// parser node
        struct FunctionParsingNode
        {
            int tokenID = -1;
            base::parser::Location location;
            base::StringView stringValue;
            StringID name;
            double floatValue = 0.0f;
            int64_t intValue = 0;
            uint64_t uintValue = 0U;
            Array<StringID> names;
            FunctionNode* node = nullptr;
            Array<FunctionNode*> nodes;
            FunctionTypeInfo type;
        };

        class FunctionParsingContext;

        /// token reader
        class FunctionParsingTokenStream : public base::NoCopy
        {
        public:
            FunctionParsingTokenStream(const base::Array<base::parser::Token*>& tokens, FunctionParsingContext& ctx);

            int readToken(FunctionParsingNode& outNode);

            INLINE const parser::Location& location() const { return m_lastTokenLocation; }
            INLINE StringView text() const { return m_lastTokenText; }

        private:
            parser::TokenList m_tokens;
            FunctionParsingContext& m_ctx;

            StringView m_lastTokenText;
            parser::Location m_lastTokenLocation;

            uint32_t matchTypeName(StringID& outTypeName, FunctionTypeInfo& outTypeInfo) const;
        };

        /// helper class for parsing function
        class FunctionParsingContext : public base::NoCopy
        {
        public:
            FunctionParsingContext(mem::LinearAllocator& mem, FunctionParser& fileParser, const StubFunction* function, FunctionCode& outCode);

            //--

            StubLocation mapLocation(const parser::Location& location);

            FunctionNode* createNode(const parser::Location& location, FunctionNodeOp op);
            FunctionNode* createNode(const parser::Location& location, FunctionNodeOp op, FunctionNode* a);
            FunctionNode* createNode(const parser::Location& location, FunctionNodeOp op, FunctionNode* a, FunctionNode* b);
            FunctionNode* createNode(const parser::Location& location, FunctionNodeOp op, FunctionNode* a, FunctionNode* b, FunctionNode* c);

            FunctionNode* makeBreakpoint(FunctionNode* node);

            FunctionTypeInfo createStaticArrayType(FunctionTypeInfo innerType, uint32_t arraySize);
            FunctionTypeInfo createDynamicArrayType(FunctionTypeInfo innerType);
            FunctionTypeInfo createClassType(const parser::Location& location, StringID className);
            FunctionTypeInfo createPtrType(const parser::Location& location, StringID className);
            FunctionTypeInfo createWeakPtrType(const parser::Location& location, StringID className);

            FunctionTypeInfo resolveTypeFromName(StringID name);

            FunctionNode* createIntConst(const parser::Location& location, int64_t val);
            FunctionNode* createUintConst(const parser::Location& location, uint64_t val);
            FunctionNode* createFloatConst(const parser::Location& location, double val);
            FunctionNode* createBoolConst(const parser::Location& location, bool val);
            FunctionNode* createStringConst(const parser::Location& location, StringView val);
            FunctionNode* createNameConst(const parser::Location& location, StringID val);
            FunctionNode* createNullConst(const parser::Location& location);
            FunctionNode* createClassTypeConst(const parser::Location& location, StringID className);

            void rootStatement(FunctionNode* node);

            void pushContextNode(FunctionNode* node);
            void popContextNode();

            FunctionNode* findContextNode(FunctionNodeOp op);
            FunctionNode* findBreakContextNode();
            FunctionNode* findContinueContextNode();

            void reportError(const parser::Location& location, StringView message);

            FunctionTypeInfo currentType;

        private:
            FunctionParser& m_parser;
            const StubFunction* m_function;
            FunctionCode& m_code;
            mem::LinearAllocator& m_mem;

            Array<FunctionNode*> m_contextStack;
        };

        //---

    } // script
} // base