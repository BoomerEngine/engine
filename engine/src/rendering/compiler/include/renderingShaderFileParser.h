/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)\n* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\parser #]
***/

#pragma once

#include "base/memory/include/linearAllocator.h"
#include "base/containers/include/inplaceArray.h"
#include "base/parser/include/textToken.h"

namespace rendering
{
    namespace compiler
    {
        namespace parser
        {
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

            typedef base::BitFlags<ElementFlag> ElementFlags;

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

            struct RENDERING_COMPILER_API TypeReference
            {
                base::parser::Location location;
                base::StringView<char> name;
                base::StringView<char> innerType;
                base::InplaceArray<int, 4> arraySizes;

                TypeReference();
                TypeReference(const base::parser::Location& loc, base::StringView<char> name);
            };

            struct RENDERING_COMPILER_API Element
            {
                ElementType type = ElementType::Invalid;
                base::parser::Location location;
                base::StringView<char> name;
                base::Array<Element*> children;
                base::Array<Element*> attributes;
                base::Array<base::parser::Token*> tokens; // inner content
                CodeNode* code = nullptr; // resolved code
                TypeReference* typeRef = nullptr; // type reference
                ElementFlags flags = ElementFlags();

                base::StringView<char> stringData;
                int64_t intData = 0;
                double floatData = 0.0;

                //---

                Element(const base::parser::Location& loc, ElementType type, base::StringView<char> name = base::StringView<char>(), ElementFlags flags=ElementFlags());

                //--

                const Element* findAttribute(base::StringView<char> name) const;

                AttributeList gatherAttributes() const;
            };

            ///----

            // get the parser language to tokenize text for shader compilation
            extern RENDERING_COMPILER_API const base::parser::ILanguageDefinition& GetShaderLanguageDefinition();

            /// analyze structure of the shader file
            /// NOTE: all memory is allocated from the linear allocator
            extern RENDERING_COMPILER_API Element* AnalyzeShaderFile(base::mem::LinearAllocator& mem, base::parser::IErrorReporter& errHandler, base::parser::TokenList& tokens);

            /// analyze structure of the shader function
            extern RENDERING_COMPILER_API CodeNode* AnalyzeShaderCode(base::mem::LinearAllocator& mem, base::parser::IErrorReporter& errHandler, const base::Array<base::parser::Token*>& tokens, const CodeLibrary& lib, const Function* contextFunction, const Program* contextProgram);

            ///----

        } // parser
    } // shader
} // rendering

