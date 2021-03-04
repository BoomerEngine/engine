/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)\n* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\parser #]
***/

#pragma once

#include "core/memory/include/linearAllocator.h"
#include "core/containers/include/inplaceArray.h"
#include "core/parser/include/textToken.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::compiler)

///----

enum class ElementFlag : uint64_t
{
    In,
    Out,
    InOut,
    Const,
    Shared,
    Vertex,
    Export,
    MAX,
};

typedef BitFlags<ElementFlag> ElementFlags;

///----

enum class ElementType : uint8_t
{
    Invalid,
    Global,
    Struct,
    StructElement,
    Function,
    FunctionArgument,
    Descriptor,
    DescriptorResourceElement,
    DescriptorConstantTable,
    DescriptorConstantTableElement,
	StaticSampler,
	RenderStates,
	KeyValueParam,
    Program,
    Attribute,
    Variable,
    Using,
};

enum class StatementType : uint8_t
{
    Invalid,
    FunctionScope,
    LocalScope,

};

struct GPU_SHADER_COMPILER_API TypeReference
{
    parser::Location location;
    StringView name;
    StringView innerType;
    InplaceArray<int, 4> arraySizes;

    TypeReference();
    TypeReference(const parser::Location& loc, StringView name);
};

struct GPU_SHADER_COMPILER_API Element
{
    ElementType type = ElementType::Invalid;
    parser::Location location;
    StringView name;
    Array<Element*> children;
    Array<Element*> attributes;
    Array<parser::Token*> tokens; // inner content
    CodeNode* code = nullptr; // resolved code
    TypeReference* typeRef = nullptr; // type reference
    ElementFlags flags = ElementFlags();

    StringView stringData;
    int64_t intData = 0;
    double floatData = 0.0;

    //---

    Element(const parser::Location& loc, ElementType type, StringView name = StringView(), ElementFlags flags=ElementFlags());

    //--

    const Element* findAttribute(StringView name) const;

    AttributeList gatherAttributes() const;
};

///----

// get the parser language to tokenize text for shader compilation
extern GPU_SHADER_COMPILER_API const parser::ILanguageDefinition& GetlanguageDefinition();

/// analyze structure of the shader file
/// NOTE: all memory is allocated from the linear allocator
extern GPU_SHADER_COMPILER_API Element* Analyzefile(LinearAllocator& mem, parser::IErrorReporter& errHandler, parser::TokenList& tokens);

/// analyze structure of the shader function
extern GPU_SHADER_COMPILER_API CodeNode* Analyzecode(LinearAllocator& mem, parser::IErrorReporter& errHandler, const Array<parser::Token*>& tokens, const CodeLibrary& lib, const Function* contextFunction, const Program* contextProgram);

///----

END_BOOMER_NAMESPACE_EX(gpu::compiler)
