/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"
#include "scriptLibrary.h"
#include "scriptTypeCasting.h"
#include "core/containers/include/hashSet.h"

BEGIN_BOOMER_NAMESPACE_EX(script)

//--

TypeCastMatrix::TypeCastMatrix()
{}

bool TypeCastMatrix::initialize(IErrorHandler& err, const Array<StubFunction*>& allGlobalFunctions)
{
    bool valid = true;

    // look at all registered casting function
    uint32_t numCastFunctions = 0;
    uint32_t numOperators = 0;
    for (auto castFunc  : allGlobalFunctions)
    {
        // extract operators
        if (castFunc->flags.test(StubFlag::Operator))
        {
            if (castFunc->operatorName.empty())
                continue;

            m_operators[castFunc->operatorName].pushBack(castFunc);
            numOperators += 1;
            continue;
        }

        // skip non-cast functions
        if (!castFunc->flags.test(StubFlag::Cast))
            continue;

        // ignore invalid functions
        if (!castFunc->returnTypeDecl || castFunc->args.size() != 1 || !castFunc->args[0]->typeDecl)
        {
            err.reportError(castFunc->location.file->absolutePath, castFunc->location.line, TempString("Cast function should have one argument and return a value"));
            valid = false;
            continue;
        }

        // get source and destination type for this cast
        auto sourceType  = castFunc->args[0]->typeDecl;
        auto destType  = castFunc->returnTypeDecl;

        // create cast entry
        auto typeInfo  = createTypeInfo(sourceType);
        auto castInfo  = typeInfo->createCastInfo(destType);

        // setup cast
        castInfo->m_castFunction = castFunc;
        castInfo->m_explicit = castFunc->flags.test(StubFlag::Explicit);
        castInfo->m_cost = castFunc->castCost;
        numCastFunctions += 1;
    }

    TRACE_SPAM("Found {} cast function(s)", numCastFunctions);
    TRACE_SPAM("Found {} operator function(s) ({} types)", numOperators, m_operators.size());
    return valid;
}

TypeCastMatrix::CastInfo* TypeCastMatrix::TypeInfo::createCastInfo(const StubTypeDecl* destType)
{
    for (auto& ptr : m_casts)
        if (StubTypeDecl::Match(ptr.m_destType, destType))
            return &ptr;

    auto& entry = m_casts.emplaceBack();
    entry.m_destType = destType;
    return &entry;
}

const TypeCastMatrix::CastInfo* TypeCastMatrix::TypeInfo::findCastInfo(const StubTypeDecl* destType) const
{
    for (auto& ptr : m_casts)
        if (StubTypeDecl::Match(ptr.m_destType, destType))
            return &ptr;

    return nullptr;
}


TypeCastMatrix::TypeInfo* TypeCastMatrix::createTypeInfo(const StubTypeDecl* sourceType)
{
    for (auto& ptr : m_types)
        if (StubTypeDecl::Match(ptr.m_sourceType, sourceType))
            return &ptr;

    auto& entry = m_types.emplaceBack();
    entry.m_sourceType = sourceType;
    return &entry;
}

const TypeCastMatrix::TypeInfo* TypeCastMatrix::findTypeInfo(const StubTypeDecl* sourceType) const
{
    for (auto& ptr : m_types)
        if (StubTypeDecl::Match(ptr.m_sourceType, sourceType))
            return &ptr;

    return nullptr;
}

//--

const StubFunction* TypeCastMatrix::findOperator(StringID op, const StubTypeDecl* left, bool leftAssignable, const StubTypeDecl* right, bool allowCasts) const
{
    // get list of matching operators
    auto matchingOperators  = m_operators.find(op);
    if (!matchingOperators)
        return nullptr;

    /*{
        StringBuilder temp1;
        left->print(temp1);
        TRACE_INFO("Matching operator, left type = {}, assignable = {}", temp1.c_str(), leftAssignable);
    }

    if (right)
    {
        StringBuilder temp1;
        right->print(temp1);
        TRACE_INFO("Matching operator, right type = {}", temp1.c_str());
    }*/

    // look for best function
    int bestCastCost = 0;
    const StubFunction* bestFunction = NULL;
    for (auto funcPtr  : (*matchingOperators))
    {
        // filter out binary operators
        if (funcPtr->args.size() == 2 && !right)
            continue;

        //TRACE_INFO("Maching operator '{}'", funcPtr->name);

        // assignment operators require left side to be assignable
        auto assignOperator = funcPtr->args[0]->flags.test(StubFlag::Out);
        if (assignOperator && !leftAssignable)
        {
            //TRACE_INFO("Left not assignable");
            continue;
        }

        // check left argument
        auto leftCast = findBestCast(left, funcPtr->args[0]->typeDecl);
        if (leftCast.m_cost == -1 || (leftCast.m_explicit && !allowCasts) )
        {
            //TRACE_INFO("Left not castable");
            continue;
        }

        // get the cost of casting the left side
        auto castCost = leftCast.m_cost;

        // check right side
        if (funcPtr->args.size() == 2)
        {
            auto rightCast = findBestCast(right, funcPtr->args[1]->typeDecl);
            if (rightCast.m_cost == -1 || (rightCast.m_explicit && !allowCasts) )
            {
                //TRACE_INFO("Right not castable");
                continue;
            }

            castCost += rightCast.m_cost;
        }

        // keep best operator so far
        //TRACE_INFO("Current cost {}, best {}", castCost, bestCastCost);
        if (!bestFunction || castCost < bestCastCost)
        {
            bestFunction = funcPtr;
            bestCastCost = castCost;
        }
    }

    // return best operator found
    return bestFunction;
}

TypeCast TypeCastMatrix::findBestCast(const StubTypeDecl* source, const StubTypeDecl* dest) const
{
    TypeCast ret;
    ret.m_sourceType = source;
    ret.m_destType = dest;
    if (findBestCast(ret))
        return ret;

    return TypeCast();
}

bool TypeCastMatrix::findBestCast(TypeCast& outCast) const
{
    const StubTypeDecl* sourceType = outCast.m_sourceType;
    const StubTypeDecl* destType = outCast.m_destType;

    // invalid types
    if (!sourceType || !destType)
        return false;

    // no cast in case of same types
    if (StubTypeDecl::Match(sourceType, destType))
    {
        outCast.m_cost = 0;
        outCast.m_castType = TypeCastMethod::Passthrough;
        return true;
    }

    // pointer cast to boolean so we dont have to write ptr != null
    if (sourceType->isSharedPointerType() && destType->isType<bool>())
    {
        outCast.m_castType = TypeCastMethod::OpCode;
        outCast.m_castingOp = FunctionNodeOp::CastStrongPtrToBool;
        outCast.m_cost = 10;
        return true;
    }
    else if (sourceType->isWeakPointerType() && destType->isType<bool>())
    {
        outCast.m_castType = TypeCastMethod::OpCode;
        outCast.m_castingOp = FunctionNodeOp::CastWeakPtrToBool;
        outCast.m_cost = 10;
        return true;
    }

    // variant type casts, geting a variant is implicit but casting back requires explicit type specification
    if (sourceType->isType<Variant>())
    {
        outCast.m_castType = TypeCastMethod::OpCode;
        outCast.m_castingOp = FunctionNodeOp::CastVariantToType;
        outCast.m_cost = 20;
        outCast.m_explicit = true; // requires explicit cast
        return true;
    }
    else if (destType->isType<Variant>())
    {
        outCast.m_castType = TypeCastMethod::OpCode;
        outCast.m_castingOp = FunctionNodeOp::CastTypeToVariant;
        outCast.m_cost = 5;
        return true;
    }

    // enum to integer
    if (sourceType->isEnumType() && destType->isType<int64_t>())
    {
        outCast.m_castType = TypeCastMethod::OpCode;
        outCast.m_castingOp = FunctionNodeOp::CastEnumToInt64;
        outCast.m_cost = 3;
        outCast.m_explicit = true;
        return true;
    }
    else if (sourceType->isType<int64_t>() && destType->isEnumType())
    {
        outCast.m_castType = TypeCastMethod::OpCode;
        outCast.m_castingOp = FunctionNodeOp::CastInt64ToEnum;
        outCast.m_cost = 3;
        outCast.m_explicit = true;
        return true;
    }
    else if (sourceType->isEnumType() && destType->isType<int>())
    {
        outCast.m_castType = TypeCastMethod::OpCode;
        outCast.m_castingOp = FunctionNodeOp::CastEnumToInt32;
        outCast.m_cost = 3;
        outCast.m_explicit = true;
        return true;
    }
    else if (sourceType->isType<int>() && destType->isEnumType())
    {
        outCast.m_castType = TypeCastMethod::OpCode;
        outCast.m_castingOp = FunctionNodeOp::CastInt32ToEnum;
        outCast.m_cost = 3;
        outCast.m_explicit = true;
        return true;
    }
    else if (sourceType->isType<StringID>() && destType->isEnumType())
    {
        outCast.m_castType = TypeCastMethod::OpCode;
        outCast.m_castingOp = FunctionNodeOp::CastNameToEnum;
        outCast.m_cost = 5;
        outCast.m_explicit = true;
        return true;
    }
    else if (sourceType->isEnumType() && destType->isType<StringID>())
    {
        outCast.m_castType = TypeCastMethod::OpCode;
        outCast.m_castingOp = FunctionNodeOp::CastEnumToName;
        outCast.m_cost = 5;
        outCast.m_explicit = false;
        return true;
    }
    else if (sourceType->isEnumType() && destType->isType<StringBuf>())
    {
        outCast.m_castType = TypeCastMethod::OpCode;
        outCast.m_castingOp = FunctionNodeOp::CastEnumToString;
        outCast.m_cost = 5;
        outCast.m_explicit = false;
        return true;
    }

    // pointer casts
    if (sourceType->isPointerType() && destType->isPointerType())
    {
        auto sourceClass  = sourceType->classType();
        auto destClass  = destType->classType();

        // upcast is always legal
        if (sourceClass == destClass || sourceClass->is(destClass))
        {
            if (sourceType->isSharedPointerType() == destType->isSharedPointerType())
            {
                outCast.m_castType = TypeCastMethod::PassthroughNoRef;
            }
            else if (!destType->isSharedPointerType())
            {
                outCast.m_castType = TypeCastMethod::OpCode;
                outCast.m_castingOp = FunctionNodeOp::CastStrongToWeak;
            }
            else
            {
                outCast.m_castType = TypeCastMethod::OpCode;
                outCast.m_castingOp = FunctionNodeOp::CastWeakToStrong;
            }

            outCast.m_cost = 1;
            return true;
        }

        // down cast requires matching type
        else if (destClass->is(sourceClass))
        {
            if (sourceType->isSharedPointerType() && destType->isSharedPointerType())
            {
                outCast.m_castType = TypeCastMethod::OpCode;
                outCast.m_castingOp = FunctionNodeOp::CastDownStrong;
                outCast.m_cost = 2;
                outCast.m_explicit = true;
                return true;
            }
            else if (!sourceType->isSharedPointerType() && !destType->isSharedPointerType())
            {
                outCast.m_castType = TypeCastMethod::OpCode;
                outCast.m_castingOp = FunctionNodeOp::CastDownWeak;
                outCast.m_cost = 2;
                outCast.m_explicit = true;
                return true;
            }
        }
    }

    // class cast
    if (sourceType->isClassType() && destType->isClassType())
    {
        auto sourceClass  = sourceType->classType();
        auto destClass  = destType->classType();

        if (sourceClass == destClass || sourceClass->is(destClass))
        {
            outCast.m_castType = TypeCastMethod::PassthroughNoRef;
            outCast.m_cost = 0;
            return true;
        }
        else if (destClass->is(sourceClass))
        {
            outCast.m_castType = TypeCastMethod::OpCode;
            outCast.m_castingOp = FunctionNodeOp::CastClassMetaDownCast;
            outCast.m_cost = 3;
            outCast.m_explicit = true;
            return true;
        }
    }
    else if (sourceType->isClassType() && destType->isType<bool>())
    {
        outCast.m_castType = TypeCastMethod::OpCode;
        outCast.m_castingOp = FunctionNodeOp::CastClassToBool;
        outCast.m_cost = 7;
        return true;
    }
    else if (sourceType->isClassType() && destType->isType<StringID>())
    {
        outCast.m_castType = TypeCastMethod::OpCode;
        outCast.m_castingOp = FunctionNodeOp::CastClassToName;
        outCast.m_cost = 10;
        outCast.m_explicit = true;
        return true;
    }
    else if (sourceType->isClassType() && destType->isType<StringBuf>())
    {
        outCast.m_castType = TypeCastMethod::OpCode;
        outCast.m_castingOp = FunctionNodeOp::CastClassToString;
        outCast.m_cost = 10;
        return true;
    }

    // use user defined cast functions (NOTE: some of them may be native or even opcode implemented but still, they are defined in the files)
    if (auto typeInfo  = findTypeInfo(sourceType))
    {
        if (auto castInfo  = typeInfo->findCastInfo(destType))
        {
            outCast.m_castType = TypeCastMethod::CastFunc;
            outCast.m_castFunction = castInfo->m_castFunction;
            outCast.m_cost = castInfo->m_cost;
            outCast.m_explicit = castInfo->m_explicit;
            return true;
        }
    }

    // no known cast
    return false;
}

//--

END_BOOMER_NAMESPACE_EX(script)
