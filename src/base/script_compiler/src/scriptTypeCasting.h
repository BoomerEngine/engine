/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#pragma once

#include "scriptFunctionCode.h"
#include "base/memory/include/linearAllocator.h"

BEGIN_BOOMER_NAMESPACE(base::script)

class StubLibrary;
struct StubTypeDecl;
struct StubFunction;

// type cast meta info
enum class TypeCastMethod : uint8_t
{
    Invalid,
    Passthrough,
    PassthroughNoRef,
    CastFunc,
    OpCode,
};

// type cast
struct TypeCast
{
    const StubTypeDecl* m_sourceType = nullptr;
    const StubTypeDecl* m_destType = nullptr;

    TypeCastMethod m_castType = TypeCastMethod::Invalid;

    const StubFunction* m_castFunction = nullptr;
    FunctionNodeOp m_castingOp = FunctionNodeOp::Nop;

    int m_cost = -1;
    bool m_explicit = false;
};

// helper class to handle type casting
class TypeCastMatrix : public base::NoCopy
{
public:
    TypeCastMatrix();

    // initialize from stub library, looks for type references and operators
    bool initialize(IErrorHandler& err, const Array<StubFunction*>& allGlobalFunctions);

    // find best cast for converting from one type to other
    bool findBestCast(TypeCast& outCast) const;

    // find best cast for converting from one type to other
    TypeCast findBestCast(const StubTypeDecl* source, const StubTypeDecl* dest) const;

    // find best operator
    const StubFunction* findOperator(StringID op, const StubTypeDecl* left, bool leftAssignable, const StubTypeDecl* right, bool allowCasts) const;

private:
    struct CastInfo
    {
        const StubTypeDecl* m_destType = nullptr;
        const StubFunction* m_castFunction = nullptr;

        int m_cost = 0;
        bool m_explicit = false;
    };

    struct TypeInfo
    {
        const StubTypeDecl* m_sourceType = nullptr;
        Array<CastInfo> m_casts;

        CastInfo* createCastInfo(const StubTypeDecl* destType);

        const CastInfo* findCastInfo(const StubTypeDecl* destType) const;
    };

    //--

    TypeInfo* createTypeInfo(const StubTypeDecl* sourceType);

    const TypeInfo* findTypeInfo(const StubTypeDecl* sourceType) const;

    Array<TypeInfo> m_types;

    HashMap<StringID, Array<const StubFunction*>> m_operators;
};

END_BOOMER_NAMESPACE(base::script)
