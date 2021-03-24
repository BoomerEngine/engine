/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#pragma once

#include "scriptLibrary.h"
#include "core/memory/include/linearAllocator.h"
#include "core/parser/include/textToken.h"

BEGIN_BOOMER_NAMESPACE_EX(script)

//---

/// parser node
struct FileParsingNode
{
    int tokenID = -1;
    TextTokenLocation location;
    StringView stringValue;
    Array<Token*> tokens;
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

    INLINE FileParsingNode(const TextTokenLocation& loc, StringView txt)
    {
        stringValue = txt;
        location = loc;
    }

    INLINE FileParsingNode(const TextTokenLocation& loc, double val)
    {
        floatValue = val;
        location = loc;
    }

    INLINE FileParsingNode(const TextTokenLocation& loc, int64_t val)
    {
        intValue = val;
        location = loc;
    }

    INLINE FileParsingNode(const TextTokenLocation& loc, uint64_t val)
    {
        uintValue = val;
        location = loc;
    }
};

//---

/// token reader
class FileParsingTokenStream : public NoCopy
{
public:
    FileParsingTokenStream(TokenList&& tokens);

    /// read a token from the stream (used as yylex)
    int readToken(FileParsingNode& outNode);

    /// extract inner tokens
    void extractInnerTokenStream(char delimiter, Array<Token*>& outList);

    //--

    INLINE const TextTokenLocation& location() const { return m_lastTokenLocation; }
    INLINE StringView text() const { return m_lastTokenText; }

private:
    TokenList m_tokens;

    StringView m_lastTokenText;
    TextTokenLocation m_lastTokenLocation;
};

//---

class FileParser;

/// helper class for parsing file content
class FileParsingContext : public NoCopy
{
public:
    FileParsingContext(FileParser& fileParser, const StubFile* file, const StubModule* module);

    StubFlags globalFlags;
    StubFunction* currentFunction = nullptr;
    Array<Stub*> currentObjects;
    Array<StubConstantValue*> currentConstantValue;

    //--

    StubLocation mapLocation(const TextTokenLocation& location);

    StubClass* beginCompound(const TextTokenLocation& location, StringID name, const StubFlags& flags);
    StubEnum* beginEnum(const TextTokenLocation& location, StringID name, const StubFlags& flags);
    void endObject();

    StubFunction* addFunction(const TextTokenLocation& location, StringID name, const StubFlags& flags);
    StubProperty* addVar(const TextTokenLocation& location, StringID name, const StubTypeDecl* typeDecl, const StubFlags& flags);
    StubEnumOption* addEnumOption(const TextTokenLocation& location, StringID name, bool hasValue = false, int64_t value = 0);
    StubConstant* addConstant(const TextTokenLocation& location, StringID name);
    StubFunctionArg* addFunctionArg(const TextTokenLocation& location, StringID name, const StubTypeDecl* typeDecl, const StubFlags& flags);

    StubModuleImport* createModuleImport(const TextTokenLocation& location, StringID name);
    StubTypeName* createTypeName(const TextTokenLocation& location, StringID name, const StubTypeDecl* decl);
    StubTypeRef* createTypeRef(const TextTokenLocation& location, StringID name);

    StubTypeDecl* createEngineType(const TextTokenLocation& location, StringID engineTypeAlias);
    StubTypeDecl* createSimpleType(const TextTokenLocation& location, const StubTypeRef* classTypeRef);
    StubTypeDecl* createPointerType(const TextTokenLocation& location, const StubTypeRef* classTypeRef);
    StubTypeDecl* createWeakPointerType(const TextTokenLocation& location, const StubTypeRef* classTypeRef);
    StubTypeDecl* createClassType(const TextTokenLocation& location, const StubTypeRef* classTypeRef);
    StubTypeDecl* createStaticArrayType(const TextTokenLocation& location, const StubTypeDecl* innerType, uint32_t arraySize);
    StubTypeDecl* createDynamicArrayType(const TextTokenLocation& location, const StubTypeDecl* innerType);

    StubConstantValue* createConstValueInt(const TextTokenLocation& location, int64_t value);
    StubConstantValue* createConstValueUint(const TextTokenLocation& location, uint64_t value);
    StubConstantValue* createConstValueFloat(const TextTokenLocation& location, double value);
    StubConstantValue* createConstValueBool(const TextTokenLocation& location, bool value);
    StubConstantValue* createConstValueString(const TextTokenLocation& location, StringView view);
    StubConstantValue* createConstValueName(const TextTokenLocation& location, StringID name);
    StubConstantValue* createConstValueCompound(const TextTokenLocation& location, const StubTypeDecl* typeRef);

    Stub* contextObject();

    void reportError(const TextTokenLocation& location, StringView message);
    void reportWarning(const TextTokenLocation& location, StringView message);

private:
    FileParser& m_parser;
    const StubFile* m_file;
    const StubModule* m_module;

    HashMap<const Stub*, HashMap<StringID, StubTypeRef*>> m_typeReferences;
    HashMap<StringID, StubModuleImport*> m_localImports;
};

//---

END_BOOMER_NAMESPACE_EX(script)
