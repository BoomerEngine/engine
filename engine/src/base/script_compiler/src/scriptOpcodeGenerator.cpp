/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"
#include "scriptLibrary.h"
#include "scriptFunctionCode.h"
#include "scriptOpcodeGenerator.h"
#include "base/script/include/scriptFunctionRuntimeCode.h"

namespace base
{
    namespace script
    {

        //--

        uint32_t OpcodeList::size() const
        {
            uint32_t count = 0;

            for (auto ptr = m_head; ptr != nullptr; ptr = ptr->next)
                count += 1;

            return count;
        }

        void OpcodeList::print(IFormatStream &f) const
        {
            uint32_t count = 0;

            for (auto ptr = m_head; ptr != nullptr; ptr = ptr->next)
            {
                f.appendf("[{}]: ", count);
                ptr->print(f);
                f.append("\n");
                count += 1;
            }
        }

        void OpcodeList::Glue(OpcodeList &a, const OpcodeList &b)
        {
            if (a)
            {
                if (b)
                {
                    a.m_tail->next = b.m_head;
                    a.m_tail = b.m_tail;
                }
            }
            else
            {
                a = b;
            }
        }

        OpcodeList OpcodeList::Glue(std::initializer_list<const OpcodeList> blocks)
        {
            OpcodeList ret;
            for (auto &b : blocks)
                Glue(ret, b);
            return ret;
        }

        void OpcodeList::extract(Array<const StubOpcode *> &outOpcodes) const
        {
            for (auto ptr = m_head; ptr != nullptr; ptr = ptr->next)
                outOpcodes.pushBack(ptr);
        }

        //--

        OpcodeGenerator::OpcodeGenerator(mem::LinearAllocator &storageMem)
            : m_mem(storageMem)
            , m_emitBreakpoints(true)
        {
        }

        OpcodeList OpcodeGenerator::makeOpcode(const FunctionNode *source, Opcode op)
        {
            auto opcode = m_mem.create<StubOpcode>();
            opcode->location = source->location;
            opcode->op = op;
            return opcode;
        }

        OpcodeList OpcodeGenerator::makeOpcode(const StubLocation& location, Opcode op)
        {
            auto opcode = m_mem.create<StubOpcode>();
            opcode->location = location;
            opcode->op = op;
            return opcode;
        }

        OpcodeList OpcodeGenerator::generateOpcodes(IErrorHandler& err, const FunctionNode* node, Array<const FunctionScope*>& activeScopes)
        {
            return generateInnerOpcodes(err, node, activeScopes);
        }

        OpcodeList OpcodeGenerator::generateScopeVariableDestructors(IErrorHandler& err, const FunctionNode* node, const Array<const FunctionScope*>& activeScopes, FunctionScope* targetScope)
        {
            OpcodeList ret;

            // the target scope MUST be withing the list of active scopes... (ie. we can't destroy something that was not created)
            if (targetScope)
            {
                if (!activeScopes.contains(targetScope))
                {
                    reportError(err, node, TempString("Internal Compiler Error: target scope if not in the current list"));
                    return OpcodeList();
                }
            }

            // process the scopes until we reach the target one
            // NOTE: we don't call dtors for the target scope since we will end up in it
            for (int i=activeScopes.lastValidIndex(); i >=0; --i)
            {
                // we reached the target scope
                auto scopeNode  = activeScopes[i];
                if (scopeNode == targetScope)
                    break;

                // look at scope variables and destroy them
                for (auto var  : scopeNode->m_localVars.values())
                {
                    auto destroyOp = makeOpcode(node, Opcode::LocalDtor);
                    destroyOp->value.i = var->index;
                    destroyOp->value.name = var->name;
                    destroyOp->stub = var->type.type;
                    OpcodeList::Glue(ret, destroyOp);
                }
            }

            // now we have all the shit required for proper local variable destruction when leaving scope
            return ret;
        }

        OpcodeList OpcodeGenerator::generateInnerOpcodes(IErrorHandler& err, const FunctionNode* node, Array<const FunctionScope*>& activeScopes)
        {
            // empty node - no opcodes
            // TODO: maybe nop ?
            if (!node)
                return OpcodeList();

            // each node type generates different opcodes
            switch (node->op)
            {
                case FunctionNodeOp::Nop:
                    return OpcodeList();

                case FunctionNodeOp::Statement:
                {
                    if (false)
                    {
                        auto child = node->child(0);
                        auto breakpoint = makeOpcode(node, Opcode::Breakpoint);
                        auto inner = generateOpcodes(err, node->child(0), activeScopes); // statement is not interested in the value
                        OpcodeList::Glue(breakpoint, inner);
                        return breakpoint;
                    }
                    else
                    {
                        auto child = node->child(0);
                        return generateOpcodes(err, node->child(0), activeScopes); // statement is not interested in the value
                    }
                }

                case FunctionNodeOp::StatementList:
                {
                    OpcodeList ret;

                    bool hasExitStatement = false;
                    for (auto child : node->children)
                    {
                        if (child)
                        {
                            if (hasExitStatement)
                            {
                                reportWarning(err, child, "Unreachable code");
                                break;
                            }

                            auto childList = generateOpcodes(err, child, activeScopes);
                            OpcodeList::Glue(ret, childList);

                            if (childList.tail())
                            {
                                auto tailOp = childList.tail()->op;
                                if (tailOp == Opcode::Exit || tailOp == Opcode::Jump) // unconditional exit
                                    hasExitStatement = true;
                            }
                        }
                    }

                    return ret;
                }

                case FunctionNodeOp::Scope:
                {
                    OpcodeList ret;

                    // add to list of active scopes, when leaving via jump (return, break or continue) we will have to destroy local scope variables
                    auto scope  = node->scope;
                    activeScopes.pushBack(scope);

                    // initialize variables in this scope
                    for (auto var  : scope->m_localVars.values())
                    {
                        auto destroyOp = makeOpcode(node, Opcode::LocalCtor);
                        destroyOp->value.i = var->index;
                        destroyOp->value.name = var->name;
                        destroyOp->stub = var->type.type;
                        OpcodeList::Glue(ret, destroyOp);
                    }

                    // build
                    bool hasExitStatement = false;
                    for (auto child : node->children)
                    {
                        if (child)
                        {
                            if (hasExitStatement)
                            {
                                reportWarning(err, child, "Unreachable code");
                                break;
                            }

                            auto childList = generateOpcodes(err, child, activeScopes);
                            OpcodeList::Glue(ret, childList);

                            if (childList.tail())
                            {
                                auto tailOp = childList.tail()->op;
                                if (tailOp == Opcode::Exit || tailOp == Opcode::Jump) // unconditional exit
                                {
                                    hasExitStatement = true;
                                }
                            }
                        }
                    }

                    // destroy variables in this scope, unless we exit it in a definate way
                    if (!hasExitStatement)
                    {
                        for (auto var : scope->m_localVars.values())
                        {
                            auto destroyOp = makeOpcode(node, Opcode::LocalDtor);
                            destroyOp->value.i = var->index;
                            destroyOp->value.name = var->name;
                            destroyOp->stub = var->type.type;
                            OpcodeList::Glue(ret, destroyOp);
                        }
                    }

                    // leave scope
                    ASSERT(activeScopes.back() == scope);
                    activeScopes.popBack();

                    return ret;
                }

                case FunctionNodeOp::ExpressionList:
                {
                    OpcodeList ret;

                    for (auto child : node->children)
                        OpcodeList::Glue(ret, generateOpcodes(err, child, activeScopes));
                    return ret;
                }

                case FunctionNodeOp::IfThenElse:
                {
                    auto jumpToFalse = makeOpcode(node, Opcode::JumpIfFalse);
                    auto cond = generateOpcodes(err, node->child(0), activeScopes);
                    auto trueBlock = generateOpcodes(err, node->child(1), activeScopes);

                    if (node->child(2))
                    {
                        auto falseBlock = generateOpcodes(err, node->child(2), activeScopes);
                        auto jumpToEnd = makeOpcode(node, Opcode::Jump);
                        auto falseLabel = makeOpcode(node,  Opcode::Label);
                        jumpToFalse->target = *falseLabel;

                        auto endLabel = makeOpcode(node, Opcode::Label);
                        jumpToEnd->target = *endLabel;

                        return OpcodeList::Glue({jumpToFalse, cond, trueBlock, jumpToEnd, falseLabel, falseBlock, endLabel});
                    }
                    else
                    {
                        auto endLabel = makeOpcode(node, Opcode::Label);
                        jumpToFalse->target = *endLabel;

                        return OpcodeList::Glue({jumpToFalse, cond, trueBlock, endLabel});
                    }
                }

                case FunctionNodeOp::CallVirtual:
                case FunctionNodeOp::CallFinal:
                case FunctionNodeOp::CallStatic:
                {
                    // we must have valid function to call
                    auto funcToCall  = node->data.function;
                    if (funcToCall == nullptr)
                    {
                        reportError(err, node, "Internal Compiler Error: Missing function to call");
                        return OpcodeList();
                    }

                    // check argument count (1 more for the context)
                    // NOTE: context is extracted during cal
                    if ((1 + funcToCall->args.size()) != node->children.size())
                    {
                        reportError(err, node, TempString("Internal Compiler Error: Invalid argument count for building a function call opcode {} != {}", funcToCall->args.size() + 1, node->children.size()));
                        return OpcodeList();
                    }

                    // handle cases when function is actually implemented as opcode
                    if (funcToCall->opcodeName)
                    {
                        Opcode opcodeToCall = Opcode::Nop;
                        if (!FindOpcodeByName(funcToCall->opcodeName, opcodeToCall))
                        {
                            reportError(err, node, TempString("Internal Compiler Error: Function '{}' uses undefined opcode '{}', unable to translate.", funcToCall->name, funcToCall->opcodeName));
                            return OpcodeList();
                        }

                        if (opcodeToCall == Opcode::Nop)
                        {
                            if (node->children.size() != 2)
                            {
                                reportError(err, node, TempString("Internal Compiler Error: Function '{}' uses passthrough opcode Nop but has invalid number of arguments", funcToCall->name));
                                return OpcodeList();
                            }

                            auto passThroughArg = node->children[2];
                            return generateInnerOpcodes(err, passThroughArg, activeScopes);
                        }
                        // HACK: boolean operator short-circuit
                        else if (opcodeToCall == Opcode::LogicOr || opcodeToCall == Opcode::LogicAnd)
                        {
                            if (node->children.size() != 3)
                            {
                                reportError(err, node, TempString("Internal Compiler Error: Logical boolean operator should have 2 arguments"));
                                return OpcodeList();
                            }

                            auto callOp = makeOpcode(node, opcodeToCall);
                            auto firstArg = generateOpcodes(err, node->child(1), activeScopes);
                            auto secondArg = generateOpcodes(err, node->child(2), activeScopes);
                            auto endLabel = makeOpcode(node, Opcode::Label);
                            callOp->target = *endLabel;
                            return OpcodeList::Glue({callOp, firstArg, secondArg, endLabel});
                        }
                        else
                        {
                            auto callOp = makeOpcode(node, opcodeToCall);

                            for (uint32_t i = 1; i < node->children.size(); ++i)
                            {
                                // generate code for the argument
                                auto funcArgNode = node->children[i];
                                auto funcArgCode = generateOpcodes(err, funcArgNode, activeScopes);

                                // generate wrapper code and glue everything together
                                OpcodeList::Glue(callOp, funcArgCode);
                            }

                            return callOp;
                        }
                    }

                    // create function header
                    OpcodeList callOp;
                    if (node->op == FunctionNodeOp::CallStatic)
                        callOp = makeOpcode(node, Opcode::StaticFunc);
                    else if (node->op == FunctionNodeOp::CallVirtual)
                        callOp = makeOpcode(node, Opcode::VirtualFunc);
                    else if (node->op == FunctionNodeOp::CallFinal)
                        callOp = makeOpcode(node, Opcode::FinalFunc);
                    callOp->stub = funcToCall;

                    // generate the arguments
                    InplaceArray<FunctionParamMode, 16> paramEncodings;
                    for (uint32_t i = 1; i < node->children.size(); ++i)
                    {
                        // generate code for the argument
                        auto funcArg = funcToCall->args[i - 1];
                        auto funcArgNode = node->children[i];
                        auto funcArgCode = generateOpcodes(err,funcArgNode, activeScopes);

                        // get the source and target reference type
                        auto sourceRef = funcArgNode->type.reference;
                        auto targetRef = funcArg->flags.test(StubFlag::Ref) || funcArg->flags.test(StubFlag::Out);

                        // determine way to pass value to function
                        auto paramEncodingMode = FunctionParamMode::TypedValue;
                        if (targetRef && sourceRef)
                            paramEncodingMode = FunctionParamMode::Ref;
                        else if (targetRef && !sourceRef)
                            paramEncodingMode = FunctionParamMode::TypedValue; // we must pass a reference but we have a value on the input

                        // encode the way we should pass the param
                        paramEncodings.pushBack(paramEncodingMode);

                        // generate wrapper code and glue everything together
                        OpcodeList::Glue(callOp, funcArgCode);
                    }

                    // save calling encoding
                    {
                        FunctionCallingEncoding encoding;
                        for (int i = paramEncodings.lastValidIndex(); i >= 0; --i)
                            encoding.encode(paramEncodings[i]);
                        callOp->value.u = encoding.value; // stored
                    }

                    // generate the context object switch
                    if (node->op != FunctionNodeOp::CallStatic)
                    {
                        if (auto contextNode = node->child(0))
                        {
                            auto contextType = contextNode->type;
                            if (contextType.isAnyPtr())
                            {
                                auto postFunctionCall = makeOpcode(node, Opcode::Label);
                                OpcodeList::Glue(callOp, postFunctionCall);

                                auto contextOp = makeOpcode(node, contextType.reference ? Opcode::ContextFromPtrRef : Opcode::ContextFromPtr);
                                contextOp->target = *postFunctionCall;
                                contextOp->stub = funcToCall->returnTypeDecl;
                                OpcodeList::Glue(contextOp, generateOpcodes(err,contextNode, activeScopes));
                                OpcodeList::Glue(contextOp, callOp);
                                callOp = contextOp;
                            }
                            else if (contextType.isStruct() && contextType.reference)
                            {
                                auto postFunctionCall = makeOpcode(node, Opcode::Label);
                                OpcodeList::Glue(callOp, postFunctionCall);

                                auto contextOp = makeOpcode(node, Opcode::ContextFromRef);
                                contextOp->target = *postFunctionCall;
                                contextOp->stub = funcToCall->returnTypeDecl;
                                OpcodeList::Glue(contextOp, generateOpcodes(err,contextNode, activeScopes));
                                OpcodeList::Glue(contextOp, callOp);
                                callOp = contextOp;
                            }
                            else if (contextType.isStruct() && !contextType.reference)
                            {
                                auto postFunctionCall = makeOpcode(node, Opcode::Label);
                                OpcodeList::Glue(callOp, postFunctionCall);

                                auto contextOp = makeOpcode(node, Opcode::ContextFromValue);
                                contextOp->target = *postFunctionCall;
                                contextOp->stub = funcToCall->returnTypeDecl;
                                OpcodeList::Glue(contextOp, generateOpcodes(err,contextNode, activeScopes));
                                OpcodeList::Glue(contextOp, callOp);
                                callOp = contextOp;
                            }
                            else
                            {
                                reportError(err, node, TempString("Internal Compiler Error: Type '{}' is invalid context for a call", contextType));
                                return OpcodeList();
                            }
                        }
                    }

                    return callOp;
                }

                case FunctionNodeOp::Context:
                {
                    auto jumpLabel = makeOpcode(node, Opcode::Label);
                    auto contextOp = makeOpcode(node, Opcode::ContextFromPtr);
                    contextOp->target = *jumpLabel;
                    contextOp->stub = node->type.type;
                    if (!contextOp->stub)
                        reportError(err, node, TempString("Internal Compiler Error: Context without proper type"));
                    OpcodeList::Glue(contextOp, generateOpcodes(err,node->child(0), activeScopes));
                    OpcodeList::Glue(contextOp, generateOpcodes(err,node->child(1), activeScopes)); // TODO: create more opcodes
                    OpcodeList::Glue(contextOp, jumpLabel);
                    return contextOp;
                }

                case FunctionNodeOp::ContextRef:
                {
                    auto jumpLabel = makeOpcode(node, Opcode::Label);
                    auto contextOp = makeOpcode(node, Opcode::ContextFromPtrRef);
                    contextOp->target = *jumpLabel;
                    contextOp->stub = node->type.type;
                    if (!contextOp->stub)
                        reportError(err, node, TempString("Internal Compiler Error: Context without proper type"));
                    OpcodeList::Glue(contextOp, generateOpcodes(err,node->child(0), activeScopes));
                    OpcodeList::Glue(contextOp, generateOpcodes(err,node->child(1), activeScopes));
                    OpcodeList::Glue(contextOp, jumpLabel);
                    return contextOp;
                }

                case FunctionNodeOp::Const:
                {
                    if (node->type.isType<StringBuf>())
                    {
                        auto ret = makeOpcode(node, Opcode::StringConst);
                        ret->value.text = StringBuf(node->data.text);
                        return ret;
                    }
                    else if (node->type.isType<StringID>())
                    {
                        auto ret = makeOpcode(node, Opcode::NameConst);
                        ret->value.name = node->data.name;
                        return ret;
                    }
                    else if (node->type.isType<char>())
                    {
                        auto ret = makeOpcode(node, Opcode::IntConst1);
                        ret->value.i = node->data.number.asInteger();
                        return ret;
                    }
                    else if (node->type.isType<uint8_t>())
                    {
                        auto ret = makeOpcode(node, Opcode::UintConst1);
                        ret->value.u = node->data.number.asUnsigned();
                        return ret;
                    }
                    else if (node->type.isType<short>())
                    {
                        auto ret = makeOpcode(node, Opcode::IntConst2);
                        ret->value.i = node->data.number.asInteger();
                        return ret;
                    }
                    else if (node->type.isType<uint16_t>())
                    {
                        auto ret = makeOpcode(node, Opcode::UintConst2);
                        ret->value.u = node->data.number.asUnsigned();
                        return ret;
                    }
                    else if (node->type.isType<int>())
                    {
                        if (node->data.number.value.i == 0)
                        {
                            return makeOpcode(node, Opcode::IntZero);
                        }
                        else if (node->data.number.value.i == 1)
                        {
                            return makeOpcode(node, Opcode::IntOne);
                        }
                        else
                        {
                            auto ret = makeOpcode(node, Opcode::IntConst4);
                            ret->value.i = node->data.number.asInteger();
                            return ret;
                        }
                    }
                    else if (node->type.isType<uint32_t>())
                    {
                        if (node->data.number.value.u == 0)
                        {
                            return makeOpcode(node, Opcode::IntZero);
                        }
                        else if (node->data.number.value.u == 1)
                        {
                            return makeOpcode(node, Opcode::IntOne);
                        }
                        else
                        {
                            auto ret = makeOpcode(node, Opcode::UintConst4);
                            ret->value.u = node->data.number.asUnsigned();
                            return ret;
                        }
                    }
                    else if (node->type.isType<float>())
                    {
                        auto ret = makeOpcode(node, Opcode::FloatConst);
                        ret->value.f = node->data.number.asFloat();
                        ret->flags |= StubFlag::Const;
                        return ret;
                    }
                    else if (node->type.isType<double>())
                    {
                        auto ret = makeOpcode(node, Opcode::DoubleConst);
                        ret->value.d = node->data.number.asFloat();
                        ret->flags |= StubFlag::Const;
                        return ret;
                    }
                    else if (node->type.isType<int64_t>())
                    {
                        auto ret = makeOpcode(node, Opcode::IntConst8);
                        ret->value.u = node->data.number.asInteger();
                        return ret;
                    }
                    else if (node->type.isType<uint64_t>())
                    {
                        auto ret = makeOpcode(node, Opcode::UintConst8);
                        ret->value.u = node->data.number.asUnsigned();
                        ret->flags |= StubFlag::Cast;
                        return ret;
                    }
                    else if (node->type.isType<bool>())
                    {
                        if (node->data.number.value.u != 0)
                            return makeOpcode(node, Opcode::BoolTrue);
                        else
                            return makeOpcode(node, Opcode::BoolFalse);
                    }
                    else if (node->type.isClassType())
                    {
                        auto ret = makeOpcode(node, Opcode::ClassConst);
                        ret->stub = node->type.typeClass();
                        return ret;
                    }

                    reportError(err, node, TempString("Internal Compiler Error: Unsupported constant type '{}'", node->type));
                    return OpcodeList();
                }

                case FunctionNodeOp::EnumConst:
                {
                    auto ret = makeOpcode(node, Opcode::EnumConst);
                    ret->stub = node->data.enumStub;
                    ret->value.name = node->data.name;
                    if (!ret->stub)
                        reportError(err, node, TempString("Internal Compiler Error: Missing enum reference in enum constant"));
                    if (ret->value.name.empty())
                        reportError(err, node, TempString("Internal Compiler Error: Missing enum name in enum constant"));
                    return ret;
                }

                case FunctionNodeOp::This:
                    if (node->type.isStruct())
                        return makeOpcode(node, Opcode::ThisStruct);
                    else if (node->type.isSharedPtr())
                        return makeOpcode(node, Opcode::ThisObject);
                    else
                        reportError(err, node, TempString("Internal Compiler Error: Invalid type for this reference"));

                case FunctionNodeOp::Null:
                    return makeOpcode(node, Opcode::Null);

                case FunctionNodeOp::CastWeakToStrong:
                {
                    auto ret = makeOpcode(node, Opcode::WeakToStrong);
                    OpcodeList::Glue(ret, generateOpcodes(err,node->child(0), activeScopes));
                    return ret;
                }

                case FunctionNodeOp::CastStrongToWeak:
                {
                    auto ret = makeOpcode(node, Opcode::StrongToWeak);
                    OpcodeList::Glue(ret, generateOpcodes(err,node->child(0), activeScopes));
                    return ret;
                }

                case FunctionNodeOp::CastDownWeak:
                {
                    auto ret = makeOpcode(node, Opcode::DynamicWeakCast);
                    ret->stub = node->type.typeClass();
                    if (!ret->stub)
                        reportError(err, node, TempString("Internal Compiler Error: Missing class reference in dynamic cast"));
                    OpcodeList::Glue(ret, generateOpcodes(err,node->child(0), activeScopes));
                    return ret;
                }

                case FunctionNodeOp::CastDownStrong:
                {
                    auto ret = makeOpcode(node, Opcode::DynamicCast);
                    ret->stub = node->type.typeClass();
                    if (!ret->stub)
                        reportError(err, node, TempString("Internal Compiler Error: Missing class reference in dynamic cast"));
                    OpcodeList::Glue(ret, generateOpcodes(err,node->child(0), activeScopes));
                    return ret;
                }

                case FunctionNodeOp::CastClassMetaDownCast:
                {
                    auto ret = makeOpcode(node, Opcode::MetaCast);
                    ret->stub = node->type.typeClass();
                    if (!ret->stub)
                        reportError(err, node, TempString("Internal Compiler Error: Missing class reference in dynamic cast"));
                    OpcodeList::Glue(ret, generateOpcodes(err,node->child(0), activeScopes));
                    return ret;
                }

                case FunctionNodeOp::CastEnumToInt64:
                {
                    auto ret = makeOpcode(node, Opcode::EnumToInt64);
                    ret->stub = node->children[0]->type.typeEnum();
                    if (!ret->stub)
                        reportError(err, node, TempString("Internal Compiler Error: Missing enum reference in enum cast"));
                    OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                    return ret;
                }

                case FunctionNodeOp::CastInt64ToEnum:
                {
                    auto ret = makeOpcode(node, Opcode::Int64ToEnum);
                    ret->stub = node->type.typeEnum();
                    if (!ret->stub)
                        reportError(err, node, TempString("Internal Compiler Error: Missing enum reference in enum cast"));
                    OpcodeList::Glue(ret, generateOpcodes(err,node->child(0), activeScopes));
                    return ret;
                }

                case FunctionNodeOp::CastEnumToInt32:
                {
                    auto ret = makeOpcode(node, Opcode::EnumToInt32);
                    ret->stub = node->children[0]->type.typeEnum();
                    if (!ret->stub)
                        reportError(err, node, TempString("Internal Compiler Error: Missing enum reference in enum cast"));
                    OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                    return ret;
                }

                case FunctionNodeOp::CastInt32ToEnum:
                {
                    auto ret = makeOpcode(node, Opcode::Int32ToEnum);
                    ret->stub = node->type.typeEnum();
                    if (!ret->stub)
                        reportError(err, node, TempString("Internal Compiler Error: Missing enum reference in enum cast"));
                    OpcodeList::Glue(ret, generateOpcodes(err,node->child(0), activeScopes));
                    return ret;
                }

                case FunctionNodeOp::CastEnumToName:
                {
                    auto ret = makeOpcode(node, Opcode::EnumToName);
                    ret->stub = node->children[0]->type.typeEnum();
                    if (!ret->stub)
                        reportError(err, node, TempString("Internal Compiler Error: Missing enum reference in enum cast"));
                    OpcodeList::Glue(ret, generateOpcodes(err,node->child(0), activeScopes));
                    return ret;
                }

                case FunctionNodeOp::CastNameToEnum:
                {
                    auto ret = makeOpcode(node, Opcode::NameToEnum);
                    ret->stub = node->type.typeEnum();
                    if (!ret->stub)
                        reportError(err, node, TempString("Internal Compiler Error: Missing enum reference in enum cast"));
                    OpcodeList::Glue(ret, generateOpcodes(err,node->child(0), activeScopes));
                    return ret;
                }

                case FunctionNodeOp::CastEnumToString:
                {
                    auto ret = makeOpcode(node, Opcode::EnumToString);
                    ret->stub = node->children[0]->type.typeEnum();
                    if (!ret->stub)
                        reportError(err, node, TempString("Internal Compiler Error: Missing enum reference in enum cast"));
                    OpcodeList::Glue(ret, generateOpcodes(err,node->child(0), activeScopes));
                    return ret;
                }

                case FunctionNodeOp::CastClassToBool:
                {
                    auto ret = makeOpcode(node, Opcode::ClassToBool);
                    OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                    return ret;
                }

                case FunctionNodeOp::CastClassToName:
                {
                    auto ret = makeOpcode(node, Opcode::ClassToName);
                    OpcodeList::Glue(ret, generateOpcodes(err,node->child(0), activeScopes));
                    return ret;
                }

                case FunctionNodeOp::CastClassToString:
                {
                    auto ret = makeOpcode(node, Opcode::ClassToString);
                    OpcodeList::Glue(ret, generateOpcodes(err,node->child(0), activeScopes));
                    return ret;
                }

                case FunctionNodeOp::CastStrongPtrToBool:
                {
                    auto ret = makeOpcode(node, Opcode::StrongToBool);
                    OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                    return ret;
                }

                case FunctionNodeOp::CastWeakPtrToBool:
                {
                    auto ret = makeOpcode(node, Opcode::WeakToBool);
                    OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                    return ret;
                }

                case FunctionNodeOp::CastTypeToVariant:
                {
                    auto ret = makeOpcode(node, Opcode::CastToVariant);
                    ret->stub = node->type.type;
                    if (!ret->stub)
                        reportError(err, node, TempString("Internal Compiler Error: Missing type reference in variant cast"));
                    OpcodeList::Glue(ret, generateOpcodes(err,node->child(0), activeScopes));
                    return ret;
                }

                case FunctionNodeOp::CastVariantToType:
                {
                    auto ret = makeOpcode(node, Opcode::CastFromVariant);
                    ret->stub = node->type.type;
                    if (!ret->stub)
                        reportError(err, node, TempString("Internal Compiler Error: Missing type reference in variant cast"));
                    OpcodeList::Glue(ret, generateOpcodes(err,node->child(0), activeScopes));
                    return ret;
                }

                case FunctionNodeOp::GeneralEqual:
                {
                    auto ret = makeOpcode(node, Opcode::TestEqual);
                    ret->stub = node->child(0)->type.type;
                    if (!ret->stub)
                        reportError(err, node, TempString("Internal Compiler Error: Missing type reference in generic comparision"));
                    OpcodeList::Glue(ret, generateOpcodes(err,node->child(0), activeScopes));
                    OpcodeList::Glue(ret, generateOpcodes(err,node->child(1), activeScopes));
                    return ret;
                }

                case FunctionNodeOp::GeneralNotEqual:
                {
                    auto ret = makeOpcode(node, Opcode::TestNotEqual);
                    ret->stub = node->child(0)->type.type;
                    if (!ret->stub)
                        reportError(err, node, TempString("Internal Compiler Error: Missing type reference in generic comparision"));
                    OpcodeList::Glue(ret, generateOpcodes(err,node->child(0), activeScopes));
                    OpcodeList::Glue(ret, generateOpcodes(err,node->child(1), activeScopes));
                    return ret;
                }

                case FunctionNodeOp::MakeValueFromRef:
                {
                    if (node->type.isType<bool>() || node->type.isType<uint8_t>())
                    {
                        auto ret = makeOpcode(node, Opcode::LoadInt1);
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                        return ret;
                    }
                    else if (node->type.isType<char>())
                    {
                        auto ret = makeOpcode(node, Opcode::LoadUint1);
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                        return ret;
                    }
                    else if (node->type.isType<short>())
                    {
                        auto ret = makeOpcode(node, Opcode::LoadInt2);
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                        return ret;
                    }
                    else if (node->type.isType<uint16_t>())
                    {
                        auto ret = makeOpcode(node, Opcode::LoadUint2);
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                        return ret;
                    }
                    else if (node->type.isType<int>())
                    {
                        auto ret = makeOpcode(node, Opcode::LoadInt4);
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                        return ret;
                    }
                    else if (node->type.isType<uint32_t>())
                    {
                        auto ret = makeOpcode(node, Opcode::LoadUint4);
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                        return ret;
                    }
                    else if (node->type.isType<int64_t>())
                    {
                        auto ret = makeOpcode(node, Opcode::LoadInt8);
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                        return ret;
                    }
                    else if (node->type.isType<uint64_t>())
                    {
                        auto ret = makeOpcode(node, Opcode::LoadUint8);
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                        return ret;
                    }
                    else if (node->type.isType<float>())
                    {
                        auto ret = makeOpcode(node, Opcode::LoadFloat);
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                        return ret;
                    }
                    else if (node->type.isType<double>())
                    {
                        auto ret = makeOpcode(node, Opcode::LoadDouble);
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                        return ret;
                    }
                    else if (node->type.isClassType())
                    {
                        auto ret = makeOpcode(node, Opcode::LoadUint8); // generic pointer
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                        return ret;
                    }
                    else if (node->type.isSharedPtr())
                    {
                        auto ret = makeOpcode(node, Opcode::LoadStrongPtr);
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                        return ret;
                    }
                    else if (node->type.isWeakPtr())
                    {
                        auto ret = makeOpcode(node, Opcode::LoadWeakPtr);
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                        return ret;
                    }
                    else
                    {
                        auto ret = makeOpcode(node, Opcode::LoadAny);
                        ret->stub = node->type.type;
                        if (!ret->stub)
                            reportError(err, node, TempString("Internal Compiler Error: Missing type reference in generic comparision"));
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                        return ret;
                    }
                }

                case FunctionNodeOp::Return:
                {
                    // write the result
                    OpcodeList ret;
                    if (node->child(0) != nullptr)
                    {
                        auto childType = node->child(0)->type;

                        if (childType.reference)
                        {
                            if (node->type.isType<bool>() || node->type.isType<uint8_t>())
                            {
                                ret = makeOpcode(node, Opcode::ReturnLoad1);
                            }
                            else if (node->type.isType<char>())
                            {
                                ret = makeOpcode(node, Opcode::ReturnLoad1);
                            }
                            else if (node->type.isType<short>())
                            {
                                ret = makeOpcode(node, Opcode::ReturnLoad2);
                            }
                            else if (node->type.isType<uint16_t>())
                            {
                                ret = makeOpcode(node, Opcode::ReturnLoad2);
                            }
                            else if (node->type.isType<int>())
                            {
                                ret = makeOpcode(node, Opcode::ReturnLoad4);
                            }
                            else if (node->type.isType<uint32_t>())
                            {
                                ret = makeOpcode(node, Opcode::ReturnLoad4);
                            }
                            else if (node->type.isType<int64_t>())
                            {
                                ret = makeOpcode(node, Opcode::ReturnLoad8);
                            }
                            else if (node->type.isType<uint64_t>())
                            {
                                ret = makeOpcode(node, Opcode::ReturnLoad8);
                            }
                            else if (node->type.isType<float>())
                            {
                                ret = makeOpcode(node, Opcode::ReturnLoad4);
                            }
                            else if (node->type.isType<double>())
                            {
                                ret = makeOpcode(node, Opcode::ReturnLoad8);
                            }
                            else
                            {
                                ret = makeOpcode(node, Opcode::ReturnAny);
                            }
                        }
                        else
                        {
                            ret = makeOpcode(node, Opcode::ReturnDirect);
                        }

                        ret->stub = childType.type;

                        OpcodeList::Glue(ret, generateOpcodes(err,node->child(0), activeScopes));
                    }

                    // generate the function exit + the scope dtors
                    OpcodeList::Glue(ret, generateScopeVariableDestructors(err, node, activeScopes, nullptr));
                    OpcodeList::Glue(ret, makeOpcode(node, Opcode::Exit));
                    return ret;
                }

                case FunctionNodeOp::VarArg:
                {
                    auto ret = makeOpcode(node, Opcode::ParamVar);
                    if (!node->data.argVar)
                    {
                        reportError(err, node, TempString("Internal Compiler Error: Missing argument reference"));
                    }
                    else
                    {
                        ret->value.i = node->data.argVar->index;
                        ret->value.name = node->data.argVar->name;
                        if (ret->value.i < 0)
                            reportError(err, node, TempString("Internal Compiler Error: Invalid argument index"));
                    }

                    return ret;
                }

                case FunctionNodeOp::VarLocal:
                {
                    auto ret = makeOpcode(node, Opcode::LocalVar);
                    if (!node->data.localVar)
                    {
                        reportError(err, node, TempString("Internal Compiler Error: Missing local variable reference"));
                    }
                    else
                    {
                        if (!node->data.localVar->type.type)
                        {
                            reportError(err, node, TempString("Internal Compiler Error: Local variable '{}' has no type", node->data.localVar->name));
                        }
                        else
                        {
                            ret->value.i = node->data.localVar->index;
                            ret->value.name = node->data.localVar->name;
                            ret->stub = node->data.localVar->type.type;

                            if (ret->value.i < 0)
                                reportError(err, node, TempString("Internal Compiler Error: Invalid local variable index"));
                        }
                    }

                    return ret;
                }

                case FunctionNodeOp::VarClass:
                {
                    auto ret = makeOpcode(node, Opcode::ContextVar);
                    if (!node->data.classVar)
                    {
                        reportError(err, node, TempString("Internal Compiler Error: Missing class property reference"));
                    }
                    else
                    {
                        ret->stub = node->data.classVar;
                        ret->value.name = node->data.classVar->name;
                    }
                    return ret;
                }

                case FunctionNodeOp::Assign:
                {
                    auto type =  node->child(0)->type;
                    if (type.isType<char>())
                    {
                        auto ret = makeOpcode(node, Opcode::AssignInt1);
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(1), activeScopes));
                        return ret;
                    }
                    else if (type.isType<uint8_t>())
                    {
                        auto ret = makeOpcode(node, Opcode::AssignUint1);
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(1), activeScopes));
                        return ret;
                    }
                    else if (type.isType<short>())
                    {
                        auto ret = makeOpcode(node, Opcode::AssignInt2);
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(1), activeScopes));
                        return ret;
                    }
                    else if (type.isType<uint16_t>())
                    {
                        auto ret = makeOpcode(node, Opcode::AssignUint2);
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(1), activeScopes));
                        return ret;
                    }
                    else if (type.isType<int>())
                    {
                        auto ret = makeOpcode(node, Opcode::AssignInt4);
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(1), activeScopes));
                        return ret;
                    }
                    else if (type.isType<uint32_t>())
                    {
                        auto ret = makeOpcode(node, Opcode::AssignUint4);
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(1), activeScopes));
                        return ret;
                    }
                    else if (type.isType<int64_t>())
                    {
                        auto ret = makeOpcode(node, Opcode::AssignInt8);
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(1), activeScopes));
                        return ret;
                    }
                    else if (type.isType<uint64_t>())
                    {
                        auto ret = makeOpcode(node, Opcode::AssignUint8);
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(1), activeScopes));
                        return ret;
                    }
                    else if (type.isType<float>())
                    {
                        auto ret = makeOpcode(node, Opcode::AssignFloat);
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                        OpcodeList::Glue(ret, generateOpcodes(err,  node->child(1), activeScopes));
                        return ret;
                    }
                    else if (type.isType<double>())
                    {
                        auto ret = makeOpcode(node, Opcode::AssignDouble);
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                        OpcodeList::Glue(ret, generateOpcodes(err,  node->child(1), activeScopes));
                        return ret;
                    }
                    else
                    {
                        auto ret = makeOpcode(node, Opcode::AssignAny);
                        ret->stub = type.type;
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                        OpcodeList::Glue(ret, generateOpcodes(err, node->child(1), activeScopes));
                        return ret;
                    }
                }

                case FunctionNodeOp::For:
                {
                    // @Loop:
                    //  if (!CondBlock) jump @BreakLabel;
                    //  StmtBlock
                    //  @ContinueLabel:
                    //  IncrBlock
                    // jump @Loop
                    // @BreakLabel:

                    // generate labels
                    auto loopStart = makeOpcode(node, Opcode::Label);
                    auto loopBreak = makeOpcode(node, Opcode::Label);
                    auto loopContinue = makeOpcode(node, Opcode::Label);

                    // store the most important labels for "break" and "continue" functionality
                    node->loopBreak = *loopBreak;
                    node->loopContinue = *loopContinue;

                    // generate code for loop parts
                    auto condBlock = generateOpcodes(err, node->child(0), activeScopes);
                    auto incrBlock = generateOpcodes(err, node->child(1), activeScopes);
                    auto stmtBlock = generateOpcodes(err, node->child(2), activeScopes);

                    // create the argument check
                    OpcodeList conditionalJump;
                    if (condBlock)
                    {
                        conditionalJump = makeOpcode(node, Opcode::JumpIfFalse);
                        conditionalJump->target = *loopBreak;
                    }

                    // create the loop jump
                    OpcodeList loopJump = makeOpcode(node, Opcode::Jump);
                    loopJump->target = *loopStart;

                    // glue stuff
                    return OpcodeList::Glue({loopStart, conditionalJump, condBlock, stmtBlock, loopContinue, incrBlock, loopJump, loopBreak});
                }

                case FunctionNodeOp::MemberOffset:
                {
                    auto ret = makeOpcode(node, Opcode::StructMember);
                    ret->stub = node->data.classVar;
                    OpcodeList::Glue(ret, generateOpcodes(err,node->child(0), activeScopes));
                    return ret;
                }

                case FunctionNodeOp::MemberOffsetRef:
                {
                    auto ret = makeOpcode(node, Opcode::StructMemberRef);
                    ret->stub = node->data.classVar;
                    OpcodeList::Glue(ret, generateOpcodes(err, node->child(0), activeScopes));
                    return ret;
                }

                case FunctionNodeOp::Break:
                {
                    if (!node->contextNode || !node->contextNode->loopBreak)
                    {
                        reportError(err, node, TempString("Internal Compiler Error: Break without containing loop/switch"));
                        return makeOpcode(node, Opcode::Nop);
                    }
                    else
                    {
                        OpcodeList ret;
                        OpcodeList::Glue(ret, generateScopeVariableDestructors(err, node, activeScopes, node->contextNode->scope));

                        auto jump = makeOpcode(node, Opcode::Jump);
                        jump->target = node->contextNode->loopBreak;
                        OpcodeList::Glue(ret, jump);
                        return ret;
                    }
                }

                case FunctionNodeOp::Continue:
                {
                    if (!node->contextNode || !node->contextNode->loopContinue)
                    {
                        reportError(err, node, TempString("Internal Compiler Error: Continue without containing loop"));
                        return makeOpcode(node, Opcode::Nop);
                    }
                    else
                    {
                        OpcodeList ret;
                        OpcodeList::Glue(ret, generateScopeVariableDestructors(err, node, activeScopes, node->contextNode->scope));

                        auto jump = makeOpcode(node, Opcode::Jump);
                        jump->target = node->contextNode->loopContinue;
                        OpcodeList::Glue(ret, jump);
                        return ret;
                    }
                }

                case FunctionNodeOp::Construct:
                {
                    auto ret = makeOpcode(node, Opcode::Constructor);
                    ret->stub = node->type.type;
                    ret->value.u = node->children.size();

                    for (auto child  : node->children)
                        OpcodeList::Glue(ret, generateOpcodes(err,child, activeScopes));

                    return ret;
                }

                case FunctionNodeOp::New:
                {
                    if (!node->type.typeClass())
                    {
                        reportError(err, node, TempString("Internal Compiler Error: New without class type"));
                        return makeOpcode(node, Opcode::Nop);
                    }

                    auto ret = makeOpcode(node, Opcode::New);
                    ret->stub = node->type.typeClass();
                    OpcodeList::Glue(ret, generateOpcodes(err,node->child(0), activeScopes));
                    return ret;
                }

                case FunctionNodeOp::PointerEqual:
                case FunctionNodeOp::PointerNotEqual:
                case FunctionNodeOp::Switch:
                case FunctionNodeOp::Case:
                case FunctionNodeOp::DefaultCase:
                case FunctionNodeOp::While:
                case FunctionNodeOp::DoWhile:
                case FunctionNodeOp::Ident:
                case FunctionNodeOp::Type:
                case FunctionNodeOp::Conditional:
                    break;
            }

            // unrecognized opcode
            reportError(err, node, TempString("Internal Compiler Error: Node '{}' not valid for opcode generation", node->op));
            return OpcodeList();
        }

        bool OpcodeGenerator::reportError(IErrorHandler& err, const StubLocation& location, StringView txt)
        {
            err.reportError(location.file->absolutePath, location.line, txt);
            return false;
        }

        void OpcodeGenerator::reportWarning(IErrorHandler& err, const StubLocation& location, StringView txt)
        {
            err.reportWarning(location.file->absolutePath, location.line, txt);
        }

        bool OpcodeGenerator::reportError(IErrorHandler& err, const FunctionNode* node, StringView txt)
        {
            return reportError(err, node->location, txt);
        }

        void OpcodeGenerator::reportWarning(IErrorHandler& err, const FunctionNode* node, StringView txt)
        {
            reportWarning(err, node->location, txt);
        }

    } // script
} // base