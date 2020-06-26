/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
***/

#pragma once

#include "renderingShaderDataType.h"
#include "renderingShaderDataValue.h"
#include "renderingShaderProgramInstance.h"

#include "base/containers/include/flags.h"
#include "base/containers/include/array.h"
#include "base/memory/include/linearAllocator.h"
#include "base/parser/include/textToken.h"
#include "base/parser/include/textErrorReporter.h"

namespace rendering
{
    namespace compiler
    {
        class ResourceTable;
        struct ResourceTableEntry;
        class FunctionFolder;
        class CompositeType;

        //----

        enum class OpCode : uint16_t
        {
            Nop, // do-nothing plug

            Scope, // list of statements
            Store, // store value at given reference
            Load, // loads value from dereferenced reference, requires a reference
            Call, // call to a shader defined function
            NativeCall, // call to a native function
            Const, // literal value, stored as float/int union
            FuncRef, // reference to a function
            ParamRef, // reference to a data parameter
            AccessArray, // access sub-element of an array
            AccessMember, // access member of a structure
            ReadSwizzle, // read swizzle (extracted from member access for simplicity)
            Cast, // explicit cast
            This, // current program instance
            ProgramInstance, // program instance constructor
            ProgramInstanceParam, // program instance initialization variable constructor
            ResourceTable, // a resource table type
            //ConstantTable, // a constant table

            Ident, // unresolved identifier, muted to FuncRef or ParamRef
            VariableDecl, // variable declaration
            
            First, // evaluates all nodes but returns the first value
            Loop, // repeats code while condition occurs
            Break, // breaks the innermost while loop
            Continue, // continues the innermost while loop
            Exit, // "exit" the shader without writing anything (discard)
            Return, // return from function
            IfElse, // if-else condition 
        };

        //---

        enum class TypeMatchTypeConv : char
        {
            NotMatches = -1,
            Matches = 0,
            ConvToInt,
            ConvToBool,
            ConvToUint,
            ConvToFloat,
        };

        enum class TypeMatchTypeExpand : uint8_t
        {
            DontExpand,
            ExpandXX,
            ExpandXXX,
            ExpandXXXX,
        };

        struct TypeMatchResult
        {
            TypeMatchTypeConv m_conv;
            TypeMatchTypeExpand m_expand;

            INLINE TypeMatchResult(TypeMatchTypeConv conv = TypeMatchTypeConv::NotMatches, TypeMatchTypeExpand expand = TypeMatchTypeExpand::DontExpand)
                : m_conv(conv)
                , m_expand(expand)
            {}
        };

        //----

        /// swizzle entry
        enum class ComponentMaskBit : char
        {
            NotDefined = -1,
            X = 0,
            Y = 1,
            Z = 2,
            W = 3,
            Zero = 4,
            One = 5,
        };

        //---

        /// component selector (read swizzle or write mask)
        class RENDERING_COMPILER_API ComponentMask
        {
        public:
            ComponentMask(); // not defined
            ComponentMask(ComponentMaskBit x, ComponentMaskBit y, ComponentMaskBit z, ComponentMaskBit w);

            // is this a valid mask swizzle ?
            // mask must have each component only once
            bool isMask() const;

            // is this a valid swizzle ?
            INLINE bool valid() const
            {
                return m_bits[0] != ComponentMaskBit::NotDefined;
            }           

            // get number of components defined in the swizzle
            // determines the output type
            INLINE uint32_t numComponents() const
            {
                for (uint32_t i = 0; i < MAX_COMPONENTS; ++i)
                    if (m_bits[i] == ComponentMaskBit::NotDefined) return i;

                return MAX_COMPONENTS;
            }

            // get swizzle for component
            INLINE ComponentMaskBit componentSwizzle(uint32_t index) const
            {
                ASSERT(index < MAX_COMPONENTS);
                return m_bits[index];
            }

            // parse the read (select) swizzle string 
            // allows for the use of 0 and 1
            // allows for repeating of the arguments (xxxx) etc
            static bool Parse(const char* txt, ComponentMask& outComponentMask);

            //--

            // get the unique numerical value representing the swizzle
            uint16_t numericalValue() const;

            // get the write mask, returns 0 if the write mask is invalid
            uint8_t writeMask() const;

            // get number of components needed to apply the swizzle (highest referenced component)
            uint8_t numberOfComponentsNeeded() const;

            // get number of output components the swizzle produces (only read swizzle)
            uint8_t numberOfComponentsProduced() const;

            // get list of components to write (in a mask)
            typedef base:: InplaceArray<uint8_t, 4> TWriteList;
            void componentsToWrite(TWriteList& outComponents) const;

            //--

            // get a string representation (for debug and error printing)
            void print(base::IFormatStream& f) const;

        private:
            static const uint32_t MAX_COMPONENTS = 4;
            ComponentMaskBit m_bits[MAX_COMPONENTS]; // general case, supports all combinations
        };

        //----

        /// a building block of the shader internal code
        class RENDERING_COMPILER_API CodeNode
        {
        public:
            CodeNode();
            CodeNode(const base::parser::Location& loc, OpCode op);
            CodeNode(const base::parser::Location& loc, OpCode op, const CodeNode* a);
            CodeNode(const base::parser::Location& loc, OpCode op, const CodeNode* a, const CodeNode* b);
            CodeNode(const base::parser::Location& loc, OpCode op, const CodeNode* a, const CodeNode* b, const CodeNode* c);
            CodeNode(const base::parser::Location& loc, OpCode op, const CodeNode* a, const CodeNode* b, const CodeNode* c, const CodeNode* d);
            CodeNode(const base::parser::Location& loc, OpCode op, const CodeNode* a, const CodeNode* b, const CodeNode* c, const CodeNode* d, const CodeNode* e);
            CodeNode(const base::parser::Location& loc, OpCode op, const DataType& dataType);
            CodeNode(const base::parser::Location& loc, const DataType& dataType, const DataValue& dataValue); // cosnt node
            ~CodeNode(); // never directly deleted

            //--

            struct ExtraData
            {
                const DataParameter* m_paramRef = nullptr;; // variable access
                const INativeFunction* m_nativeFunctionRef = nullptr;; // reference to a native function (implemented in the c++)
                base::StringID m_name; // member/function access
                base::StringBuf m_path; // member/function access
                CodeNode* m_nodeRef = nullptr;;  // break/continue
                CodeNode* m_parentScopeRef = nullptr;; // paren OpScope we are under
                const ResourceTable* m_resourceTable = nullptr; // descriptor reference
                const ResourceTableEntry* m_resourceTableMember = nullptr;
                const CompositeType* m_compositeTable = nullptr; // constant buffer reference
                ComponentMask m_mask; // member mask for AccessMember and Store operations
                DataType m_castType; // type for casting
                uint32_t m_attributes = 0; // extra attributes
                const Function* m_finalFunctionRef = nullptr;
                //const Program* m_finalProgramRef = nullptr;
                //const ProgramConstants* m_finalProgramConsts = nullptr;
            };

            //--

            // get the location in file
            INLINE const base::parser::Location& location() const { return m_loc; }

            // get the opcode
            INLINE OpCode opCode() const { return m_op; }

            // get the data type computed for the node
            // statement nodes do not have a type computed
            INLINE const DataType& dataType() const { return m_dataType; }
            INLINE bool dataTypeResolved() const { return m_typesResolved; }

            // get the value of this node (if assigned)
            INLINE const DataValue& dataValue() const { return m_dataValue; }

            // get child nodes
            typedef base::Array<const CodeNode*> TChildren;
            INLINE const TChildren& children() const { return m_children; }

            // get extra data for code node
            INLINE const ExtraData& extraData() const { return m_extraData; }

            // get assigned value
            INLINE ExtraData& extraData() { return m_extraData; }

            //--

            // add an extra child node
            void addChild(const CodeNode* child);

            // transfer children from another node
            void moveChildren(CodeNode* otherNode);

            //--

            // print  to debug stream
            void print(base::IFormatStream& builder, uint32_t level, uint32_t childIndex) const;

            // clone node
            CodeNode* clone(base::mem::LinearAllocator& mem) const;

            //--

            // link scopes
            static void LinkScopes(CodeNode* node, CodeNode* parentScopeNode);

            // mutate the code tree
            static bool MutateNode(CodeLibrary& lib, const Program* program, const Function* func, CodeNode*& node, base::Array<CodeNode*>& parentStack, base::parser::IErrorReporter& err);

            // resolve type at node
            static bool ResolveTypes(CodeLibrary& lib, const Program* program, const Function* func, CodeNode* node, base::parser::IErrorReporter& err);

            /*// calculate CRC of the code
            static bool CalcCRC(base::CRC64& crc, const CodeNode* node);*/

        private:
            OpCode m_op; // operation

            base::parser::Location m_loc; // location in source code
            DataType m_dataType; // data type of this node
            DataValue m_dataValue; // data assigned to this node (NOTE: not necessary a const-expr data)
            ExtraData m_extraData; // value of the node
            TChildren m_children; // child nodes
            base::Array<const DataParameter*> m_declarations; // declarations in this scope
            bool m_typesResolved = false;

            //--

            static bool MatchNodeType(CodeLibrary& lib, CodeNode*& nodePtr, DataType requiredType, bool explicitCast, base::parser::IErrorReporter& err);
            static CodeNode* Dereferece(CodeLibrary& lib, CodeNode* nodePtr, base::parser::IErrorReporter& err);

            //--

            static CodeNode* InsertTypeCastNode(CodeLibrary& lib, CodeNode* nodePtr, TypeMatchTypeConv convType);
            static CodeNode* InsertScalarExtensionNode(CodeLibrary& lib, CodeNode* nodePtr, TypeMatchTypeExpand expandType);

            static bool HandleResourceArrayAccess(CodeLibrary& lib, CodeNode* node, base::parser::IErrorReporter& err);

            struct ResolveResult
            {
                const DataParameter* m_param;
                const Function* m_function;
                const INativeFunction* m_nativeFunction;
                const Program* m_program;

                INLINE operator bool() const
                {
                    return (m_param != nullptr) || (m_function != nullptr) || (m_nativeFunction != nullptr);
                }

                ResolveResult();
                ResolveResult(const DataParameter* param, const Program* program = nullptr);
                ResolveResult(const Function* func);
                ResolveResult(const INativeFunction* func);
            };

            static ResolveResult ResolveIdent(CodeLibrary& lib, const Program* program, const Function* func, CodeNode* contextNode, const base::StringID name, base::parser::IErrorReporter& err);

            friend class FunctionFolder;
        };

        //----

    } // shader
} // rendering