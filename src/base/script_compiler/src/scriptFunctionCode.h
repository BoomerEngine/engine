/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#pragma once

#include "scriptLibrary.h"

BEGIN_BOOMER_NAMESPACE(base::script)

struct FunctionNode;
struct FunctionVar;
struct FunctionScope;
class OpcodeGenerator;
class OpcodeList;

enum class FunctionNodeOp : uint8_t
{
    Nop,
    Statement,
    Scope,
    StatementList,
    ExpressionList,
    IfThenElse,
    Switch,
    Case,
    DefaultCase,
    For,
    While,
    DoWhile,
    Assign,
    Operator,
    Call,
    New,
    Ident,
    Type,
    AccessMember,
    AccessIndex,
    Var,
    Const,
    Null,
    Return,
    Break,
    Continue,
    Conditional,
    This,

    VarArg,
    VarClass,
    VarLocal,
    FunctionVirtual,
    FunctionStatic,
    FunctionFinal,
    FunctionAlias,
    EnumConst,
    CallFinal,
    CallVirtual,
    CallStatic,
    Context,
    ContextRef,
    MemberOffset,
    MemberOffsetRef,
    Construct,
    MakeValueFromRef,

    GeneralEqual,
    GeneralNotEqual,
    PointerEqual,
    PointerNotEqual,

    CastWeakToStrong,
    CastStrongToWeak,
    CastDownStrong,
    CastDownWeak,
    CastEnumToInt64,
    CastEnumToInt32,
    CastEnumToName,
    CastEnumToString,
    CastInt64ToEnum,
    CastInt32ToEnum,
    CastNameToEnum,
    CastStrongPtrToBool,
    CastWeakPtrToBool,
    CastTypeToVariant,
    CastVariantToType,
    CastClassMetaDownCast,
    CastClassToBool,
    CastClassToName,
    CastClassToString,
};

struct FunctionTypeInfo
{
    const StubTypeDecl* type = nullptr;
    bool reference = false; // this is a reference to memory location
    bool constant = false; // value cannot be modified

    //-

    INLINE FunctionTypeInfo(const FunctionTypeInfo& other) = default;
    INLINE FunctionTypeInfo& operator=(const FunctionTypeInfo& other) = default;

    INLINE FunctionTypeInfo(const StubTypeDecl* decl = nullptr) : type(decl) {}

    INLINE operator bool() const { return type != nullptr; }

    bool isStruct() const;
    bool isClassType() const;
    bool isEnumType() const;
    bool isSharedPtr() const;
    bool isWeakPtr() const;
    bool isAnyPtr() const;

    const StubClass* typeClass() const;
    const StubEnum* typeEnum() const;

    FunctionTypeInfo removeRef() const;
    FunctionTypeInfo makeRef() const;

    FunctionTypeInfo removeConst() const;
    FunctionTypeInfo makeConst() const;

    template< typename T >
    INLINE bool isType() const
    {
        if (type && type->metaType == StubTypeType::Engine)
            return type->name == base::reflection::GetTypeName<T>();

        return false;
    }

    INLINE void print(IFormatStream& f) const
    {
        if (reference)
            f << "ref ";
        if (constant)
            f << "const ";
        f << type->fullName();
    }
};

struct FunctionVar
{
    StringID name;
    StubLocation location;
    FunctionScope* scope = nullptr;
    FunctionTypeInfo type;
    short index = -1;
};

enum class FunctionNumberType : uint8_t
{
    Unsigned,
    Integer,
    FloatingPoint,
};

struct FunctionNumber
{
    union
    {
        uint64_t u = 0;
        int64_t i;
        double f;
    } value;

    FunctionNumberType type;

    INLINE bool isUnsigned() const { return type == FunctionNumberType::Unsigned; }
    INLINE bool isInteger() const { return type == FunctionNumberType::Integer; }
    INLINE bool isFloat() const { return type == FunctionNumberType::FloatingPoint; }

    FunctionNumber(FunctionNumberType t = FunctionNumberType::Integer);
    explicit FunctionNumber(uint64_t u);
    explicit FunctionNumber(int64_t i);
    explicit FunctionNumber(double d);

    bool isNegative() const;

    uint64_t asUnsigned() const;
    int64_t asInteger() const;
    double asFloat() const;

    bool fitsUint8() const;
    bool fitsUint16() const;
    bool fitsUint32() const;
    bool fitsUint64() const;

    bool fitsInt8() const;
    bool fitsInt16() const;
    bool fitsInt32() const;
    bool fitsInt64() const;

    void print(IFormatStream& f) const;
};

struct FunctionNodeData
{
    StringView text;
    StringID name;
    FunctionTypeInfo type;
    FunctionNumber number;

    //--

    const FunctionVar* localVar = nullptr;
    const StubFunctionArg* argVar = nullptr;
    const StubProperty* classVar = nullptr;
    const StubFunction* function = nullptr;
    const StubEnum* enumStub = nullptr;

    Array<const StubFunction*> aliasFunctions;
};

struct FunctionScope
{
    FunctionNode* ownerNode = nullptr;
    FunctionScope* parent = nullptr;

    HashMap<StringID, const FunctionVar*> m_localVars;

    const FunctionVar* findLocalVar(StringID name) const;
    const FunctionVar* findVar(StringID name) const;
};

struct FunctionNode
{
    FunctionNodeOp op = FunctionNodeOp::Nop;
    StubLocation location;

    FunctionScope* scope = nullptr; // eventually always valid
    Array<FunctionNode*> children;

    FunctionNodeData data;
    FunctionNode* contextNode = nullptr;

    mutable const StubOpcode* loopBreak = nullptr;
    mutable const StubOpcode* loopContinue = nullptr;

    //--

    FunctionTypeInfo type;

    //--

    void add(FunctionNode* node);
    void addFromStatementList(FunctionNode* node);
    void addFromExpressionList(FunctionNode* node);

    void print(uint32_t indent, IFormatStream& f) const;

    const FunctionNode* child(uint32_t index) const;
};

class FunctionCode
{
public:
    FunctionCode(mem::LinearAllocator& mem, StubLibrary& lib, const StubFunction* function);
    ~FunctionCode();

    // set root fuction node
    void rootNode(FunctionNode* rootNode);

    // compile parsed function code
    bool compile(IErrorHandler& err);

    // print the code
    void printTree();

private:
    mem::LinearAllocator& m_mem;
    StubLibrary& m_lib;
    const StubFunction* m_function;
    const StubClass* m_class;

    bool m_static;

    FunctionNode* m_rootNode = nullptr;

    short m_localVarIndex = 0;

    Array<const FunctionVar*> m_allVars;

    //--

    bool connectScopes(IErrorHandler& err, FunctionNode* node, FunctionScope* parentScope);
    bool resolveVars(IErrorHandler& err, FunctionNode* node);

    typedef base::Array<FunctionNode*> TNodeStack;
    bool resolveTypes(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack);

    bool resolveIdent(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack);
    bool resolveThis(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack);
    bool resolveNull(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack);
    bool resolveOperator(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack);
    bool resolveCall(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack);
    bool resolveMember(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack);
    bool resolveType(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack);
    bool resolveAssign(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack);
    bool resolveExplicitCast(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack);
    bool resolveStructConstruct(IErrorHandler& err, FunctionNode*& node, const StubClass* structStub, TNodeStack& parentStack);
    bool resolveIfElse(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack);
    bool resolveLoop(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack);
    bool resolveReturn(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack);
    bool resolveFunctionAlias(IErrorHandler& err, const FunctionNode* node, FunctionNode* funcNode, TNodeStack& parentStack);
    bool resolveNew(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack);

    bool makeIntoConstantNode(IErrorHandler& err, FunctionNode*& node, const StubConstant* value);
    bool makeIntoConstantNode(IErrorHandler& err, FunctionNode*& node, const StubConstantValue* value, const FunctionTypeInfo& valueType);
    bool makeIntoValue(IErrorHandler& err, FunctionNode*& node);

    bool makeIntoMatchingType(IErrorHandler& err, FunctionNode*& node, const FunctionTypeInfo& requiredType, bool explicitCast);
    bool makeIntoMatchingTypeNoRefChange(IErrorHandler& err, FunctionNode*& node, const FunctionTypeInfo& requiredType, bool explicitCast);
    bool makeIntoMatchingTypeFuncCall(IErrorHandler& err, FunctionNode*& node, const FunctionTypeInfo& requiredType, bool explicitCast);
    bool makeIntoMatchingTypeOperatorCall(IErrorHandler& err, FunctionNode*& node, const FunctionTypeInfo& requiredType, bool explicitCast);
    bool makeIntoMatchingTypeOpcodeCall(IErrorHandler& err, FunctionNode*& node, const FunctionTypeInfo& requiredType, bool explicitCast);

    int calcAliasCastingConst(const FunctionNode* callNode, const StubFunction* func, bool allowExplicitCasting);

    enum class ConstantMatchResult
    {
        OK,
        TypeToSmall,
        NotANumber,
    };

    ConstantMatchResult tryMakeIntoMatchingConstantValue(IErrorHandler& err, FunctionNode*& node, const FunctionTypeInfo& requiredType);

    bool generateFunctionCodeOpcodes(IErrorHandler& err, OpcodeGenerator& gen, OpcodeList& opcodes);
    bool generateClassConstructor(IErrorHandler& err, OpcodeGenerator& gen, OpcodeList& opcodes);
    bool generateClassDestructor(IErrorHandler& err, OpcodeGenerator& gen, OpcodeList& opcodes);

    bool reportError(IErrorHandler& err, const StubLocation& location, StringView txt);
    void reportWarning(IErrorHandler& err, const StubLocation& location, StringView txt);

    bool reportError(IErrorHandler& err, const FunctionNode* node, StringView txt);
    void reportWarning(IErrorHandler& err, const FunctionNode* node, StringView txt);
};

END_BOOMER_NAMESPACE(base::script)
