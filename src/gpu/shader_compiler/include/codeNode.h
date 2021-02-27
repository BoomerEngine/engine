/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\nodes #]
***/

#pragma once

#include "dataType.h"
#include "dataValue.h"
#include "programInstance.h"

#include "core/containers/include/flags.h"
#include "core/containers/include/array.h"
#include "core/memory/include/linearAllocator.h"
#include "core/parser/include/textToken.h"
#include "core/parser/include/textErrorReporter.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::compiler)

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
	VariableDecl, // variable declaration
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
	CreateVector, // vector constructor
	CreateMatrix, // matrix constructor
	CreateArray, // array constructor

	Ident, // unresolved identifier, muted to FuncRef or ParamRef

	Loop, // repeats code while condition occurs
	Break, // breaks the innermost while loop
	Continue, // continues the innermost while loop
	Exit, // "exit" the shader without writing anything (discard)
	Return, // return from function
	IfElse, // if-else condition 

	ListElement, // element of linked list, used only during initial parsing
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
class GPU_SHADER_COMPILER_API ComponentMask
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
    typedef  InplaceArray<uint8_t, 4> TWriteList;
    void componentsToWrite(TWriteList& outComponents) const;

    //--

    // get a string representation (for debug and error printing)
    void print(IFormatStream& f) const;

private:
    static const uint32_t MAX_COMPONENTS = 4;
    ComponentMaskBit m_bits[MAX_COMPONENTS]; // general case, supports all combinations
};

//----

/// a building block of the shader internal code
class GPU_SHADER_COMPILER_API CodeNode
{
public:
    CodeNode();
    CodeNode(const parser::Location& loc, OpCode op);
    CodeNode(const parser::Location& loc, OpCode op, const CodeNode* a);
    CodeNode(const parser::Location& loc, OpCode op, const CodeNode* a, const CodeNode* b);
    CodeNode(const parser::Location& loc, OpCode op, const CodeNode* a, const CodeNode* b, const CodeNode* c);
    CodeNode(const parser::Location& loc, OpCode op, const CodeNode* a, const CodeNode* b, const CodeNode* c, const CodeNode* d);
    CodeNode(const parser::Location& loc, OpCode op, const CodeNode* a, const CodeNode* b, const CodeNode* c, const CodeNode* d, const CodeNode* e);
    CodeNode(const parser::Location& loc, OpCode op, const DataType& dataType);
    CodeNode(const parser::Location& loc, const DataType& dataType, const DataValue& dataValue); // constant node
    ~CodeNode(); // never directly deleted

    //--

	// The Mr. Creosote structure a.k.a. "The Bucket"
    struct ExtraData
    {
		StringID m_name; // member/function access
		StringBuf m_path; // member/function access

		const DataParameter* m_paramRef = nullptr;; // variable access
        const INativeFunction* m_nativeFunctionRef = nullptr;; // reference to a native function (implemented in the c++)
		const Function* m_finalFunctionRef = nullptr;
		CodeNode* m_nodeRef = nullptr;;  // break/continue
		CodeNode* m_parentScopeRef = nullptr;; // paren OpScope we are under
		CodeNode* m_nextStatement = nullptr;

        const ResourceTable* m_resourceTable = nullptr; // descriptor reference
        const ResourceTableEntry* m_resourceTableMember = nullptr;

        const CompositeType* m_compositeTable = nullptr; // constant buffer reference
        ComponentMask m_mask; // member mask for AccessMember and Store operations

        DataType m_castType; // type for casting
		TypeMatchTypeConv m_castMode = TypeMatchTypeConv::NotMatches; // how to cast data
		uint8_t m_rawAttribute = 0;

		Array<ExtraAttribute> m_attributesMap;
	};

    //--

    // get the location in file
    INLINE const parser::Location& location() const { return m_loc; }

    // get the opcode
    INLINE OpCode opCode() const { return m_op; }

    // get the data type computed for the node
    // statement nodes do not have a type computed
    INLINE const DataType& dataType() const { return m_dataType; }
    INLINE bool dataTypeResolved() const { return m_typesResolved; }

    // get the value of this node (if assigned)
    INLINE const DataValue& dataValue() const { return m_dataValue; }

    // get child nodes
    typedef Array<const CodeNode*> TChildren;
    INLINE const TChildren& children() const { return m_children; }

    // get extra data for code node
    INLINE const ExtraData& extraData() const { return m_extraData; }

	// declarations in this scope
	INLINE const Array<const DataParameter*>& declarations() const { return m_declarations; }

    // get assigned value
    INLINE ExtraData& extraData() { return m_extraData; }

    //--

    // add an extra child node
    void addChild(const CodeNode* child);

    // transfer children from another node
    void moveChildren(Array<const CodeNode*>&& children);

    //--

    // print  to debug stream
    void print(IFormatStream& builder, uint32_t level, uint32_t childIndex) const;

    // clone node
    CodeNode* clone(mem::LinearAllocator& mem) const;

    //--

    // link scopes
    static void LinkScopes(CodeNode* node, CodeNode* parentScopeNode);

    // mutate the code tree
    static bool MutateNode(CodeLibrary& lib, const Program* program, const Function* func, CodeNode*& node, Array<CodeNode*>& parentStack, parser::IErrorReporter& err);

    // resolve type at node
    static bool ResolveTypes(CodeLibrary& lib, const Program* program, const Function* func, CodeNode* node, parser::IErrorReporter& err);

private:
    OpCode m_op; // operation

    parser::Location m_loc; // location in source code
    DataType m_dataType; // data type of this node
    DataValue m_dataValue; // data assigned to this node (NOTE: not necessary a const-expr data)
    ExtraData m_extraData; // value of the node
    TChildren m_children; // child nodes
    Array<const DataParameter*> m_declarations; // declarations in this scope
    bool m_typesResolved = false;

	//--

#define SHADER_RESOLVE_FUNC CodeLibrary& lib, const Program* program, const Function* func, CodeNode* node, parser::IErrorReporter& err
	static bool ResolveTypes_This(SHADER_RESOLVE_FUNC);
	static bool ResolveTypes_Load(SHADER_RESOLVE_FUNC);
	static bool ResolveTypes_Store(SHADER_RESOLVE_FUNC);
	static bool ResolveTypes_VariableDecalration(SHADER_RESOLVE_FUNC);
	static bool ResolveTypes_Ident(SHADER_RESOLVE_FUNC);
	static bool ResolveTypes_AccessArray(SHADER_RESOLVE_FUNC);
	static bool ResolveTypes_AccessMember(SHADER_RESOLVE_FUNC);
	static bool ResolveTypes_AccessMember_Vector(const DataType&, StringID, SHADER_RESOLVE_FUNC);
	static bool ResolveTypes_AccessMember_Matrix(const DataType&, StringID, SHADER_RESOLVE_FUNC);
	static bool ResolveTypes_AccessMember_Scalar(const DataType&, StringID, SHADER_RESOLVE_FUNC);
	static bool ResolveTypes_AccessMember_Program(const DataType&, StringID, SHADER_RESOLVE_FUNC);
	static bool ResolveTypes_AccessMember_Struct(const DataType&, StringID, SHADER_RESOLVE_FUNC);
	static bool ResolveTypes_Cast(SHADER_RESOLVE_FUNC);
	static bool ResolveTypes_Call(SHADER_RESOLVE_FUNC);
	static bool ResolveTypes_IfElse(SHADER_RESOLVE_FUNC);
	static bool ResolveTypes_Loop(SHADER_RESOLVE_FUNC);
	static bool ResolveTypes_ProgramInstance(SHADER_RESOLVE_FUNC);
	static bool ResolveTypes_Return(SHADER_RESOLVE_FUNC);
	static bool ResolveTypes_CreateVector(SHADER_RESOLVE_FUNC);
	static bool ResolveTypes_CreateMatrix(SHADER_RESOLVE_FUNC);
	static bool ResolveTypes_CreateArray(SHADER_RESOLVE_FUNC);

    //--

    static bool MatchNodeType(CodeLibrary& lib, CodeNode*& nodePtr, DataType requiredType, bool explicitCast, parser::IErrorReporter& err);
    static CodeNode* Dereferece(CodeLibrary& lib, CodeNode* nodePtr, parser::IErrorReporter& err);

    //--

    static CodeNode* InsertTypeCastNode(CodeLibrary& lib, CodeNode* nodePtr, TypeMatchTypeConv convType);
    static CodeNode* InsertScalarExtensionNode(CodeLibrary& lib, CodeNode* nodePtr, TypeMatchTypeExpand expandType);

    static bool HandleResourceArrayAccess(CodeLibrary& lib, CodeNode* node, parser::IErrorReporter& err);

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

    static ResolveResult ResolveIdent(CodeLibrary& lib, const Program* program, const Function* func, CodeNode* contextNode, const StringID name, parser::IErrorReporter& err);

    friend class FunctionFolder;
};

//----

END_BOOMER_NAMESPACE_EX(gpu::compiler)
